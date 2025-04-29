

#include "luaconf.h"
#include "sqlite3.h"
#include <lua.h>
#include <lauxlib.h> 
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>




#define Q_DB_ERROR(L,db,rc) \
        lua_pushfstring((L),"E%d EE%d %s, ",rc,sqlite3_extended_errcode((db)),sqlite3_errstr((ret)));\
        lua_pushstring((L), sqlite3_errmsg((db)));\
        lua_concat((L), 2); \
        return lua_error((L));

#define Q_CHECK_OK(L,db,ret) \
    if ((ret) != SQLITE_OK) { \
        Q_DB_ERROR((L),(db),(ret))\
    } 

#define Q_CHECK_STMT_OK(L,stmt,ret) \
    if ((ret) != SQLITE_OK) { \
        sqlite3 * db=sqlite3_db_handle(stmt);\
        Q_DB_ERROR((L),(db),(ret))\
    } 


#define CHECK_DB(db) do { if (!(db)) luaL_error(L, "attempt to use a closed database"); } while(0)
#define CHECK_STMT(stmt) do { if (!(stmt)) luaL_error(L, "attempt to use a finalized statement"); } while(0)



//================= shared

static void lsqlite__ferr(lua_State * L,sqlite3 * db,int rc){
    if (db){
        lua_pushfstring(L,"E:%d,EE:%d,O:%d,%s,",rc,sqlite3_extended_errcode(db),sqlite3_error_offset(db),sqlite3_errstr(rc));
        lua_pushstring(L, sqlite3_errmsg(db));
        lua_concat(L, 2);
    }else
        lua_pushfstring((L),"E%d:%s",(rc),sqlite3_errstr((rc)));
} 

#define lsqlite__check_rc(L,db,rc) do {\
        if ((rc)!=SQLITE_OK) {\
            lsqlite__ferr((L),(db),(rc));\
            lua_error((L));\
        }\
    } while(0)
#define lsqlite__stmt_check_rc(L,stmt,rc) do {\
        if ((rc)!=SQLITE_OK) {\
            sqlite3 * db=sqlite3_db_handle((stmt));\
            lsqlite__ferr((L),db,(rc));\
            lua_error((L));\
        }\
    } while(0)

//================= SQLITE



static int lsqlite_version(lua_State * L){
    lua_pushstring(L,sqlite3_libversion());
    return 1;
}

static int lsqlite_type(lua_State * L){
    luaL_checkany(L, 1);
    if (lua_type(L, 1)==LUA_TUSERDATA){
        void ** p=lua_touserdata(L, 1);
        if (p && lua_getmetatable(L, 1)){
            if (lua_equal(L, -1, lua_upvalueindex(1))){
                if (*p) lua_pushliteral(L, "statement");
                else lua_pushliteral(L, "finalized statement");
                return 1;
            }
            if (lua_equal(L, -1, lua_upvalueindex(2))){
                if (*p) lua_pushliteral(L, "database");
                else lua_pushliteral(L, "closed database");
                return 1;
            } 
        }
    }
    lua_pushnil(L);
    return 1;
}




static int lsqlite_open(lua_State * L){
    const char * db_name=luaL_checkstring(L, 1);
    sqlite3 * db=NULL;
    int flags=0;
    int top=lua_gettop(L);
    if (top>1){
        static const int mode[] = {SQLITE_OPEN_READONLY,SQLITE_OPEN_READWRITE,SQLITE_OPEN_CREATE};
        static const char *const modenames[] = {"readonly", "readwrite", "create", NULL};
        for (int i=2; i<=top; i++) {
            flags|=mode[luaL_checkoption(L, i, NULL, modenames)];
        }
    }else{
        flags=SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    }
    int rc=sqlite3_open_v2(db_name, &db, flags, NULL);
    if (rc!=SQLITE_OK){
        lsqlite__ferr(L, db, rc);
        if (db) sqlite3_close(db);
        return lua_error(L);
    }
    sqlite3 ** pdb =  lua_newuserdata(L,sizeof(sqlite3*));
    if (pdb==NULL){
        sqlite3_close(db);
        return luaL_error(L,"no enough memory");
    }
    *pdb=db;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L,-2);
    return 1;
}





//================= DB

static inline sqlite3 * lsqlite__todb(lua_State * L){
    sqlite3 ** pdb=lua_touserdata(L, 1);
    if (!(*pdb)) luaL_error(L, "attempt to use a closed database");
    return (*pdb);
}

static int lsqlite_db_gc(lua_State * L){
    sqlite3 ** pdb=lua_touserdata(L, 1);
    if ((*pdb)!=NULL){
        sqlite3_close_v2(*pdb);
        *pdb=NULL;
    }
    return 0;
}

static int lsqlite_db_close(lua_State * L){
    sqlite3 ** pdb=lua_touserdata(L, 1);
    if ((*pdb)==NULL)
        return luaL_error(L, "database is already closed");
    int rc=sqlite3_close(*pdb);
    lsqlite__check_rc(L,*pdb,rc);
    *pdb=NULL;
    return 0;
}


static int lsqlite_db_is_readonly(lua_State * L){
    sqlite3 * db=lsqlite__todb(L);
    const char * name=luaL_optstring(L, 2,"main");
    int rc=sqlite3_db_readonly(db,name);
    if (rc==-1)
        return luaL_error(L,"no attached database named '%s'",name);
    lua_pushboolean(L, rc==1);
    return 1;
}

static int lsqlite_db_name(lua_State * L){
    sqlite3 * db=lsqlite__todb(L);
    int i=luaL_checkint(L, 2);
    const char * name = sqlite3_db_name(db, i);
    if (!name) 
        return luaL_error(L, "out of range");
    lua_pushstring(L, name);
    return 1;
}

static int lsqlite_db_filename(lua_State * L){
    sqlite3 * db=lsqlite__todb(L);
    const char * name = luaL_optstring(L, 2, "main");
    const char * fname = sqlite3_db_filename(db,name);
    if (!fname) 
        return luaL_error(L,"no attached database named '%s'",name);
    lua_pushstring(L, fname);
    return 1;
}





static int lsqlite_is_autocommit(lua_State * L){
    sqlite3 * db=lsqlite__todb(L);
    lua_pushboolean(L,sqlite3_get_autocommit(db)==1);
    return 1;
}

static int lsqlite_db_id(lua_State * L){
    sqlite3 * db=lsqlite__todb(L);
    int id=sqlite3_last_insert_rowid(db);
    if (id==0) 
        return 0;
    lua_pushinteger(L,id);
    return 1;
}


static int lsqlite_db_changes(lua_State * L){
    sqlite3 * db=lsqlite__todb(L);
    lua_pushinteger(L,sqlite3_changes(db));
    return 1;
}


static int lsqlite_db_exec(lua_State * L){
    sqlite3 * db=lsqlite__todb(L);
    const char *sql = luaL_checkstring(L, 2);
    int rc=sqlite3_exec(db,sql,NULL,NULL,NULL);
    lsqlite__check_rc(L,db,rc);
    return 0;
}



static int lsqlite_db_prepare(lua_State * L){
    sqlite3 * db=lsqlite__todb(L);
    size_t sql_len;
    const char *sql = luaL_checklstring(L, 2,&sql_len);
    sqlite3_stmt * stmt;
    int rc,top;
    top=lua_gettop(L);
    if (top>2){
        int flags=0;
        static const int mode[] = {SQLITE_PREPARE_PERSISTENT,SQLITE_PREPARE_NORMALIZE,SQLITE_PREPARE_NO_VTAB};
        static const char *const modenames[] = {"persistent", "normalize", "no_vtab", NULL};
        for (int i=3; i<=top; i++) {
            flags|=mode[luaL_checkoption(L, i, NULL, modenames)];
        }
        rc=sqlite3_prepare_v3(db, sql,sql_len,flags,&stmt,NULL);
    }else
        rc=sqlite3_prepare_v2(db, sql,sql_len,&stmt,NULL);
    lsqlite__check_rc(L,db,rc);
    sqlite3_stmt ** pstmt =  lua_newuserdata(L,sizeof(sqlite3_stmt*));
    if (pstmt==NULL){
        sqlite3_finalize(stmt);
        return luaL_error(L,"no enough memory");
    }
    *pstmt=stmt;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L,-2);
    return 1;
}










//================= STMT

static inline sqlite3_stmt * lsqlite__tostmt(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    if (!(*pstmt)) luaL_error(L, "attempt to use a finalized statement");
    return (*pstmt);
}


static int lsqlite_stmt_gc(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    if ((*pstmt)!=NULL){
        //printf("stmt_gc %d\n",
        sqlite3_finalize(*pstmt);//);
        *pstmt=NULL;
    }
    return 0;
}

static int lsqlite_stmt_finalize(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    if ((*pstmt)==NULL)
        return luaL_error(L,"already finalized");
    sqlite3 * db=sqlite3_db_handle(*pstmt);
    int rc=sqlite3_finalize(*pstmt);
    *pstmt=NULL;
    lsqlite__check_rc(L,db,rc);
    
    return 0;
}

static int lsqlite_stmt_is_readonly(lua_State * L){
    sqlite3_stmt * stmt=lsqlite__tostmt(L);
    lua_pushboolean(L, sqlite3_stmt_readonly(stmt));
    return 1;
}



static void lsqlite__stmt_bind(lua_State * L,sqlite3_stmt * stmt,int q_index,int l_value_idex,int q_extand_index){
    int rc;
    switch(lua_type(L,l_value_idex)){
        case LUA_TNIL:rc= sqlite3_bind_null(stmt,q_index);break;
        case LUA_TNUMBER:{
                if (q_extand_index && lua_type(L, q_extand_index)==LUA_TBOOLEAN){
                    if (lua_toboolean(L, q_extand_index))
                        rc=sqlite3_bind_int64(stmt,q_index,lua_tointeger(L,l_value_idex));
                    else
                        rc=sqlite3_bind_double(stmt, q_index, lua_tonumber(L,l_value_idex));
                }else{
                    lua_Number n = lua_tonumber(L, q_index);
                    lua_Integer i = lua_tointeger(L, q_index);
                    if (i == n)
                        rc=sqlite3_bind_int64(stmt,q_index,lua_tointeger(L,l_value_idex));
                    else
                        rc=sqlite3_bind_double(stmt, q_index, lua_tonumber(L,l_value_idex));
                }
            }
            break;
        case LUA_TSTRING:{
                size_t sz;
                const char * value=lua_tolstring(L,l_value_idex,&sz);
                if (q_extand_index && lua_type(L, q_extand_index)==LUA_TBOOLEAN){
                    if (lua_toboolean(L, q_extand_index))
                        rc=sqlite3_bind_blob(stmt,q_index,value,sz,SQLITE_TRANSIENT);
                    else
                        rc=sqlite3_bind_text(stmt,q_index,value,sz,SQLITE_TRANSIENT);
                }else
                    rc=sqlite3_bind_text(stmt,q_index,value,sz,SQLITE_TRANSIENT);
            }
            break;
        case LUA_TBOOLEAN:rc= sqlite3_bind_int(stmt,q_index,lua_toboolean(L,l_value_idex));break;
        default: luaL_error(L,"invalid type");return ;
    } 
    lsqlite__stmt_check_rc(L, stmt, rc);
}

static inline void lsqlite__stmt_bind_all(lua_State * L,sqlite3_stmt * stmt){
    int count=sqlite3_bind_parameter_count(stmt);
    int i;
    const char * name;
    for(i=1;i<=count;i++){
        name = sqlite3_bind_parameter_name(stmt, i );
        if (name && (name[0]=='$' || name[0]=='@' || name[0]==':')){
            lua_pushstring(L, ++name);
            //lua_gettable(L,2);
            lua_rawget(L, 2);
        }else{
            lua_pushinteger(L, i);
            //lua_gettable(L,2);
            lua_rawget(L, 2);
        }
        lsqlite__stmt_bind(L,stmt,i,-1,0);
    }
}

static int lsqlite_stmt_bind(lua_State * L){
    sqlite3_stmt * stmt=lsqlite__tostmt(L);
    int type=lua_type(L,2);
    if (type==LUA_TTABLE)
        lsqlite__stmt_bind_all(L,stmt);
    else{
        int q_index;
        if (type==LUA_TSTRING){
            q_index=sqlite3_bind_parameter_index(stmt,lua_tostring(L, 2));
        }else
            q_index=lua_tointeger(L,2);
        lsqlite__stmt_bind(L,stmt,q_index,3,4);
    }
    return 0;
}



static int lsqlite__stmt_col(lua_State * L,sqlite3_stmt * stmt,int q_index){
    int type=sqlite3_column_type(stmt,q_index);
    switch (type) {
        case SQLITE_INTEGER:lua_pushinteger(L,sqlite3_column_int64(stmt,q_index));break;
        case SQLITE_TEXT:lua_pushlstring(L,(const char *)sqlite3_column_text(stmt,q_index),sqlite3_column_bytes(stmt,q_index));break;
        case SQLITE_NULL:lua_pushnil(L);break;
        case SQLITE_FLOAT:lua_pushnumber(L,sqlite3_column_double(stmt,q_index));break;
        case SQLITE_BLOB:lua_pushlstring(L,sqlite3_column_blob(stmt,q_index),sqlite3_column_bytes(stmt,q_index));break;
        default:lua_pushnil(L);
    }
    return 1;
}


static int lsqlite_stmt_next(lua_State * L){
    sqlite3_stmt * stmt=lsqlite__tostmt(L);
    int rc=sqlite3_step(stmt);
    if (rc==SQLITE_ROW){
        int count=sqlite3_column_count(stmt);
        int i;
        const char * name;
        lua_createtable(L, count, count);
        for(i=0;i<count;){
            lsqlite__stmt_col(L,stmt,i);
            name = sqlite3_column_name(stmt, i );
            if (name){
                lua_pushvalue(L, -1);
                lua_setfield(L,-3,name);
            }
            lua_rawseti(L,-2,++i);
        }
        return 1;
    }
    if (rc==SQLITE_DONE){
        lua_pushboolean(L, 1);
        return 1;
    }
    lsqlite__stmt_check_rc(L, stmt, rc);
    return luaL_error(L, "unexpected rc %d",rc);
}

static int lsqlite_stmt_reset(lua_State * L){
    sqlite3_stmt * stmt=lsqlite__tostmt(L);
    int rc=sqlite3_reset(stmt);
    lsqlite__stmt_check_rc(L, stmt, rc);
    lua_settop(L, 1);
    return 1;
}

static int lsqlite_stmt_clear(lua_State * L){
    sqlite3_stmt * stmt=lsqlite__tostmt(L);
    int rc=sqlite3_clear_bindings(stmt);
    lsqlite__stmt_check_rc(L, stmt, rc);
    lua_settop(L, 1);
    return 1;
}






static const struct luaL_Reg lsqlite_stmt_mt[] = {
    {"bind",lsqlite_stmt_bind},
    {"next",lsqlite_stmt_next},
    {"reset",lsqlite_stmt_reset},
    {"clear",lsqlite_stmt_clear},
    {"finalize",lsqlite_stmt_finalize},  
    {"is_readonly",lsqlite_stmt_is_readonly},
    {"__gc",lsqlite_stmt_gc},
    {NULL,NULL}
};



static const struct luaL_Reg lsqlite_db_mt[] = {
    {"exec",lsqlite_db_exec},
    {"close",lsqlite_db_close},
    {"is_readonly",lsqlite_db_is_readonly},
    {"name",lsqlite_db_name},
    {"filename",lsqlite_db_filename},
    {"changes",lsqlite_db_changes},
    {"is_autocommit",lsqlite_is_autocommit},
    {"id",lsqlite_db_id},
    {"__gc",lsqlite_db_gc},
    {NULL,NULL}
};



static const struct luaL_Reg lsqlite_fn[] = {
    {"version",lsqlite_version},
    {NULL,NULL}
};



LUA_API int luaopen_lsqlite(lua_State * L){

    //lsqlite_stmt_mt
    lua_newtable(L);
    luaL_setfuncs(L,lsqlite_stmt_mt, 0);   
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
     
    
    //lsqlite_db_mt
    lua_newtable(L);
    luaL_setfuncs(L,lsqlite_db_mt, 0);  
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushvalue(L, -2);
    lua_pushcclosure(L, lsqlite_db_prepare, 1);lua_setfield(L, -2, "prepare");
    

    //lsqlite
    lua_newtable(L);
    luaL_setfuncs(L,lsqlite_fn, 0);
    lua_pushvalue(L, -3);
    lua_pushvalue(L, -3);
    lua_pushcclosure(L, lsqlite_type, 2);lua_setfield(L, -2, "type");
    lua_pushvalue(L, -2);
    lua_pushcclosure(L, lsqlite_open, 1);lua_setfield(L, -2, "open");
    
    return 1;
}

