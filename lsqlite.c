

#include "luaconf.h"
#include "sqlite3.h"
#include <lua.h>
#include <lauxlib.h> 
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>




#define Q_DB_ERROR(L,db,ret) \
        lua_pushfstring((L),"E%d %s, ",sqlite3_errcode((db)),sqlite3_errstr((ret)));\
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
    sqlite3 * db;
    int flags=luaL_optint(L, 2, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
    int ret=sqlite3_open_v2(db_name, &db, flags, NULL);
    Q_CHECK_OK(L,db,ret);
    sqlite3 ** pdb =  lua_newuserdata(L,sizeof(sqlite3*));
    if (pdb==NULL){
        sqlite3_close_v2(db);
        return luaL_error(L,"ENOMEM");
    }
    *pdb=db;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L,-2);
    return 1;
}





//================= DB
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
    Q_CHECK_OK(L,*pdb,sqlite3_close(*pdb));
    *pdb=NULL;
    return 0;
}

static int lsqlite_db_is_readonly(lua_State * L){
    sqlite3 ** pdb=lua_touserdata(L, 1);
    CHECK_DB(*pdb);
    const char * name=luaL_optstring(L, 2,"main");
    int ret=sqlite3_db_readonly(*pdb,name);
    if (ret==-1)
        return luaL_error(L,"no attached database named '%s'",name);
    lua_pushboolean(L, ret==1);
    return 1;
}

static int lsqlite_db_name(lua_State * L){
    sqlite3 ** pdb=lua_touserdata(L, 1);
    CHECK_DB(*pdb);
    int i=luaL_checkint(L, 2);
    const char * name = sqlite3_db_name(*pdb, i);
    if (!name) 
        return luaL_error(L, "out of range");
    lua_pushstring(L, name);
    return 1;
}

static int lsqlite_db_filename(lua_State * L){
    sqlite3 ** pdb=lua_touserdata(L, 1);
    CHECK_DB(*pdb);
    const char * name = luaL_optstring(L, 2, "main");
    const char * fname = sqlite3_db_filename(*pdb,name);
    if (!fname) 
        return luaL_error(L,"no attached database named '%s'",name);
    lua_pushstring(L, fname);
    return 1;
}





static int lsqlite_is_autocommit(lua_State * L){
    sqlite3 ** pdb=lua_touserdata(L, 1);
    CHECK_DB(*pdb);
    lua_pushboolean(L,sqlite3_get_autocommit(*pdb)==1);
    return 1;
}

static int lsqlite_db_changes(lua_State * L){
    sqlite3 ** pdb = lua_touserdata(L,1);
    CHECK_DB(*pdb);
    lua_pushinteger(L,sqlite3_changes(*pdb));
    return 1;
}


static int lsqlite_db_exec(lua_State * L){
    sqlite3 ** pdb = lua_touserdata(L,1);
    CHECK_DB(*pdb);
    const char *sql = luaL_checkstring(L, 2);
    Q_CHECK_OK(L,*pdb,sqlite3_exec(*pdb,sql,NULL,NULL,NULL));
    return 0;
}







static int lsqlite_db_prepare(lua_State * L){
    sqlite3 ** pdb = lua_touserdata(L,1);
    CHECK_DB(*pdb);
    size_t sql_len;
    const char *sql = luaL_checklstring(L, 2,&sql_len);
    sqlite3_stmt * stmt;
    int ret,is_persistant;
    is_persistant=lua_gettop(L)>3?lua_toboolean(L, 3):0;
    if (is_persistant){
        ret=sqlite3_prepare_v3(*pdb, sql,sql_len,SQLITE_PREPARE_PERSISTENT,&stmt,NULL);
    }else{
        ret=sqlite3_prepare_v2(*pdb, sql,sql_len,&stmt,NULL);
    }
    Q_CHECK_OK(L,*pdb,ret);
    sqlite3_stmt ** pstmt =  lua_newuserdata(L,sizeof(sqlite3_stmt*));
    if (pstmt==NULL){
        sqlite3_finalize(stmt);
        return luaL_error(L,"ENOMEM");
    }
    *pstmt=stmt;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L,-2);
    return 1;
}





static int lsqlite_db_id(lua_State * L){
    sqlite3 ** pdb = lua_touserdata(L,1);
    CHECK_DB(*pdb);
    int var=sqlite3_last_insert_rowid(*pdb);
    if (var==0) 
        return 0;
    lua_pushinteger(L,var);
    return 1;
}





//================= STMT

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
    CHECK_STMT(*pstmt);
    if ((*pstmt)==NULL)
        return luaL_error(L,"already finalized");
    Q_CHECK_STMT_OK(L,*pstmt, sqlite3_finalize(*pstmt));
    *pstmt=NULL;
    return 0;
}

static int lsqlite_stmt_is_readonly(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    CHECK_STMT(*pstmt);
    lua_pushboolean(L, sqlite3_stmt_readonly(*pstmt));
    return 1;
}



static int _lsqlite_stmt_bind(lua_State * L,sqlite3_stmt * stmt,int q_index,int l_value_idex){
    int var;
    switch(lua_type(L,l_value_idex)){
        case LUA_TNIL:var= sqlite3_bind_null(stmt,q_index);break;
        case LUA_TNUMBER:var= sqlite3_bind_int(stmt,q_index,lua_tointeger(L,l_value_idex));break;
        case LUA_TSTRING:{
            size_t sz;
            const char * value=lua_tolstring(L,l_value_idex,&sz);
            var= sqlite3_bind_text(stmt,q_index,value,sz,SQLITE_TRANSIENT);
        }
        break;
        case LUA_TBOOLEAN:var= sqlite3_bind_int(stmt,q_index,lua_toboolean(L,l_value_idex));break;
        case LUA_TTABLE:{
            lua_rawgeti(L, l_value_idex,1);
            var=lua_type(L,-1);
            if (var==LUA_TNUMBER){
                var=sqlite3_bind_double(stmt, q_index, lua_tonumber(L,-1));
                break;
            }
            if (var==LUA_TSTRING){
                size_t sz;
                const char * value=lua_tolstring(L,-1,&sz);
                var=sqlite3_bind_blob(stmt,q_index,value,sz,SQLITE_TRANSIENT);
                break;
            }
            luaL_error(L,"invalid type");return 0;
        }
        default: luaL_error(L,"invalid type");return 0;
    } 
    Q_CHECK_STMT_OK(L,stmt,var);
    return 0;
}

static int lsqlite_stmt_bind(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    CHECK_STMT(*pstmt);
    int q_index;
    if (lua_type(L,2)==LUA_TSTRING){
        q_index=sqlite3_bind_parameter_index(*pstmt,lua_tostring(L, 2));
    }else
        q_index=lua_tointeger(L,2);
    _lsqlite_stmt_bind(L,*pstmt,q_index,3);
    lua_settop(L,1);
    return 1;
}

static int lsqlite_stmt_bind_all(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    CHECK_STMT(*pstmt);
    int count=sqlite3_bind_parameter_count(*pstmt);
    int i;
    const char * name;
    for(i=1;i<=count;i++){
        name = sqlite3_bind_parameter_name(*pstmt, i );
        if (name && (name[0]=='$' || name[0]=='@' || name[0]==':')){
            lua_pushstring(L, ++name);
            //lua_gettable(L,2);
            lua_rawget(L, 2);
        }else{
            lua_pushinteger(L, i);
            //lua_gettable(L,2);
            lua_rawget(L, 2);
        }
        _lsqlite_stmt_bind(L,*pstmt,i,-1);
    }
    lua_settop(L,1);
    return 1;
}


static int lsqlite_stmt_done(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    CHECK_STMT(*pstmt);
    int var=sqlite3_step(*pstmt);
    if (var==SQLITE_DONE || var==SQLITE_ROW){
        var=sqlite3_reset(*pstmt);
    }        
    Q_CHECK_STMT_OK(L,*pstmt,var);
    return 0;
}

static int _lsqlite_stmt_col(lua_State * L,sqlite3_stmt * stmt,int q_index){
    int type=sqlite3_column_type(stmt,q_index);
    switch (type) {
        case SQLITE_INTEGER:lua_pushinteger(L,sqlite3_column_int(stmt,q_index));break;
        case SQLITE_TEXT:lua_pushlstring(L,(const char *)sqlite3_column_text(stmt,q_index),sqlite3_column_bytes(stmt,q_index));break;
        case SQLITE_NULL:lua_pushnil(L);break;
        case SQLITE_FLOAT:lua_pushnumber(L,sqlite3_column_double(stmt,q_index));break;
        case SQLITE_BLOB:lua_pushlstring(L,sqlite3_column_blob(stmt,q_index),sqlite3_column_bytes(stmt,q_index));break;
        default:lua_pushnil(L);
    }
    return 1;
}


static int _lsqlite_stmt_row(lua_State * L,sqlite3_stmt * stmt){
    int var=sqlite3_step(stmt);
    if (var==SQLITE_ROW){
        int count=sqlite3_column_count(stmt);
        int i;
        const char * name;
        lua_createtable(L, count, count);
        for(i=0;i<count;){
            _lsqlite_stmt_col(L,stmt,i);
            name = sqlite3_column_name(stmt, i );
            if (name){
                lua_pushvalue(L, -1);
                lua_setfield(L,-3,name);
            }
            lua_rawseti(L,-2,++i);
        }
        return 1;
    }
    if (var==SQLITE_DONE){
        var=sqlite3_reset(stmt);
    }
    Q_CHECK_STMT_OK(L,stmt,var);
    return 0;
}

static int lsqlite_stmt_row(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    CHECK_STMT(*pstmt);
    return _lsqlite_stmt_row(L,*pstmt);
}

static int lsqlite_stmt_rows(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    CHECK_STMT(*pstmt);
    int var=lua_gettop(L);
    if (var>1) lua_settop(L, 1);
    lua_pushcfunction(L, lsqlite_stmt_row);
    lua_insert(L,-2);
    return 2;
}





static const struct luaL_Reg lsqlite_stmt_mt[] = {
    {"bind_all",lsqlite_stmt_bind_all},
    {"bind",lsqlite_stmt_bind},
    //{"col",lsqlite_stmt_col},
    {"row",lsqlite_stmt_row},
    {"rows",lsqlite_stmt_rows},
    {"done",lsqlite_stmt_done},
    {"finalize",lsqlite_stmt_finalize},  
    {"is_readonly",lsqlite_stmt_is_readonly},
    {"__gc",lsqlite_stmt_gc},
    {NULL,NULL}
};


static const struct luaL_Reg lsqlite_db_mt[] = {
    {"exec",lsqlite_db_exec},
    {"prepare",lsqlite_db_prepare},
    {"close",lsqlite_db_close},
    {"is_readonly",lsqlite_db_is_readonly},
    {"name",lsqlite_db_name},
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

