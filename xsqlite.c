

#include "luaconf.h"
#include "sqlite3.h"
#include <lua.h>
#include <lauxlib.h> 
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>




//================= shared

static void xsqlite__ferr(lua_State * L,sqlite3 * db,int rc){
    if (db){
        lua_pushfstring(L,"E:%d,EE:%d,O:%d,%s,",rc,sqlite3_extended_errcode(db),sqlite3_error_offset(db),sqlite3_errstr(rc));
        lua_pushstring(L, sqlite3_errmsg(db));
        lua_concat(L, 2);
    }else
        lua_pushfstring((L),"E%d:%s",(rc),sqlite3_errstr((rc)));
} 

#define xsqlite__check_rc(L,db,rc) do {\
        if ((rc)!=SQLITE_OK) {\
            xsqlite__ferr((L),(db),(rc));\
            lua_error((L));\
        }\
    } while(0)
#define xsqlite__stmt_check_rc(L,stmt,rc) do {\
        if ((rc)!=SQLITE_OK) {\
            sqlite3 * db=sqlite3_db_handle((stmt));\
            xsqlite__ferr((L),db,(rc));\
            lua_error((L));\
        }\
    } while(0)

//================= SQLITE



static int xsqlite_version(lua_State * L){
    lua_pushstring(L,sqlite3_libversion());
    return 1;
}

//upvalue 1 is real's mt
//upvalue 2 is blob's mt
//upvalue 3 is stmt's mt
//upvalue 4 is db's mt
static int xsqlite_type(lua_State * L){
    int t = lua_type(L, 1);
    luaL_argcheck(L, t != LUA_TNONE, 1, "value expected");
    if (t==LUA_TUSERDATA){
        void ** p=lua_touserdata(L, 1);
        if (p && lua_getmetatable(L, 1)){
            if (lua_equal(L, -1, lua_upvalueindex(1))){
                lua_pushliteral(L, "real");
                return 1;
            }
            if (lua_equal(L, -1, lua_upvalueindex(2))){
                lua_pushliteral(L, "blob");
                return 1;
            }
            if (lua_equal(L, -1, lua_upvalueindex(3))){
                if (*p) lua_pushliteral(L, "statement");
                else lua_pushliteral(L, "finalized statement");
                return 1;
            }
            if (lua_equal(L, -1, lua_upvalueindex(4))){
                if (*p) lua_pushliteral(L, "database");
                else lua_pushliteral(L, "closed database");
                return 1;
            } 
        }
    }
    lua_pushstring(L, lua_typename(L, t));
    return 1;
}

//upvalue 1 is real's mt
static int xsqlite_real(lua_State * L){
    if (lua_isnoneornil(L, 1)){
        lua_pushnil(L);
        return 1;
    }
    lua_Number real=luaL_checknumber(L, 1);
    lua_Number * preal =  lua_newuserdata(L,sizeof(lua_Number));
    if (preal==NULL){
        return luaL_error(L,"no enough memory");
    }
    *preal=real;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L,-2);
    return 1;
}

//upvalue 1 is blob's mt
static int xsqlite_blob(lua_State * L){
    if (lua_isnoneornil(L, 1)){
        lua_pushnil(L);
        return 1;
    }
    luaL_checktype(L,1, LUA_TSTRING);
    int * pref =  lua_newuserdata(L,sizeof(int));
    if (pref==NULL){
        return luaL_error(L,"no enough memory");
    }
    lua_pushvalue(L, 1);
    *pref=luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L,-2);
    return 1;
}



static int xsqlite_open(lua_State * L){
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
        xsqlite__ferr(L, db, rc);
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

//================= BLOB
static int xsqlite_blob_gc(lua_State * L){
    int * pref=lua_touserdata(L, 1);
    luaL_unref(L, LUA_REGISTRYINDEX, *pref);
    return 0;
}




//================= DB

static inline sqlite3 * xsqlite__todb(lua_State * L){
    sqlite3 ** pdb=lua_touserdata(L, 1);
    if (!(*pdb)) luaL_error(L, "attempt to use a closed database");
    return (*pdb);
}

static int xsqlite_db_gc(lua_State * L){
    sqlite3 ** pdb=lua_touserdata(L, 1);
    if ((*pdb)!=NULL){
        sqlite3_close_v2(*pdb);
        *pdb=NULL;
    }
    return 0;
}

static int xsqlite_db_close(lua_State * L){
    sqlite3 ** pdb=lua_touserdata(L, 1);
    if ((*pdb)==NULL)
        return luaL_error(L, "database is already closed");
    int rc=sqlite3_close(*pdb);
    xsqlite__check_rc(L,*pdb,rc);
    *pdb=NULL;
    return 0;
}


static int xsqlite_db_is_readonly(lua_State * L){
    sqlite3 * db=xsqlite__todb(L);
    const char * name=luaL_optstring(L, 2,"main");
    int rc=sqlite3_db_readonly(db,name);
    if (rc==-1)
        return luaL_error(L,"no attached database named '%s'",name);
    lua_pushboolean(L, rc==1);
    return 1;
}

static int xsqlite_db_name(lua_State * L){
    sqlite3 * db=xsqlite__todb(L);
    int i=luaL_checkint(L, 2);
    const char * name = sqlite3_db_name(db, i);
    if (!name) 
        return luaL_error(L, "out of range");
    lua_pushstring(L, name);
    return 1;
}

static int xsqlite_db_filename(lua_State * L){
    sqlite3 * db=xsqlite__todb(L);
    const char * name = luaL_optstring(L, 2, "main");
    const char * fname = sqlite3_db_filename(db,name);
    if (!fname) 
        return luaL_error(L,"no attached database named '%s'",name);
    lua_pushstring(L, fname);
    return 1;
}





static int xsqlite_is_autocommit(lua_State * L){
    sqlite3 * db=xsqlite__todb(L);
    lua_pushboolean(L,sqlite3_get_autocommit(db)==1);
    return 1;
}

static int xsqlite_db_id(lua_State * L){
    sqlite3 * db=xsqlite__todb(L);
    int id=sqlite3_last_insert_rowid(db);
    if (id==0) 
        return 0;
    lua_pushinteger(L,id);
    return 1;
}


static int xsqlite_db_changes(lua_State * L){
    sqlite3 * db=xsqlite__todb(L);
    lua_pushinteger(L,sqlite3_changes(db));
    return 1;
}


static int xsqlite_db_exec(lua_State * L){
    sqlite3 * db=xsqlite__todb(L);
    const char *sql = luaL_checkstring(L, 2);
    int rc=sqlite3_exec(db,sql,NULL,NULL,NULL);
    xsqlite__check_rc(L,db,rc);
    return 0;
}



static int xsqlite_db_prepare(lua_State * L){
    sqlite3 * db=xsqlite__todb(L);
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
    xsqlite__check_rc(L,db,rc);
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

static inline sqlite3_stmt * xsqlite__tostmt(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    if (!(*pstmt)) luaL_error(L, "attempt to use a finalized statement");
    return (*pstmt);
}


static int xsqlite_stmt_gc(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    if ((*pstmt)!=NULL){
        //printf("stmt_gc %d\n",
        sqlite3_finalize(*pstmt);//);
        *pstmt=NULL;
    }
    return 0;
}

static int xsqlite_stmt_finalize(lua_State * L){
    sqlite3_stmt ** pstmt=lua_touserdata(L, 1);
    if ((*pstmt)==NULL)
        return luaL_error(L,"already finalized");
    sqlite3 * db=sqlite3_db_handle(*pstmt);
    int rc=sqlite3_finalize(*pstmt);
    *pstmt=NULL;
    xsqlite__check_rc(L,db,rc);
    
    return 0;
}

static int xsqlite_stmt_is_readonly(lua_State * L){
    sqlite3_stmt * stmt=xsqlite__tostmt(L);
    lua_pushboolean(L, sqlite3_stmt_readonly(stmt));
    return 1;
}



static void xsqlite__stmt_bind(lua_State * L,sqlite3_stmt * stmt,int q_index,int l_value_idex){
    int rc;
    switch(lua_type(L,l_value_idex)){
        case LUA_TNIL:rc= sqlite3_bind_null(stmt,q_index);break;
        case LUA_TNUMBER:{
                lua_Number n = lua_tonumber(L, l_value_idex);
                lua_Integer i = lua_tointeger(L, l_value_idex);
                if (i == n)
                    rc=sqlite3_bind_int64(stmt,q_index,lua_tointeger(L,l_value_idex));
                else
                    rc=sqlite3_bind_double(stmt, q_index, lua_tonumber(L,l_value_idex));
                break;
            }
        case LUA_TSTRING:{
                size_t sz;
                const char * value=lua_tolstring(L,l_value_idex,&sz);
                rc=sqlite3_bind_text(stmt,q_index,value,sz,SQLITE_TRANSIENT);  
                break;
            }
        case LUA_TBOOLEAN:rc= sqlite3_bind_int(stmt,q_index,lua_toboolean(L,l_value_idex));break;
        /*case LUA_TTABLE:{
            lua_rawgeti(L, l_value_idex, 1);
            rc=lua_type(L, -1);
            if (rc==LUA_TNUMBER){
                lua_Number n = lua_tonumber(L, -1);
                rc=sqlite3_bind_double(stmt, q_index, n);
                break;
            }else if (rc==LUA_TSTRING) {
                size_t sz;
                const char * blob=lua_tolstring(L,-1,&sz);
                rc=sqlite3_bind_blob(stmt,q_index,blob,sz,SQLITE_TRANSIENT);
            }else if (rc==LUA_TNIL){
                rc=sqlite3_bind_null(stmt,q_index);
            }else{
                luaL_error(L,"invalid type");return ;
            }
            lua_pop(L, 1);
            break;
        }*/
        case LUA_TUSERDATA:{
            void * p=lua_touserdata(L, l_value_idex);
            if (p && lua_getmetatable(L, l_value_idex)){
                if (lua_equal(L, -1, lua_upvalueindex(1))){
                    lua_Number real=*((lua_Number *)p);
                    /*
                        we dont need any check
                        number was initialized once with final value
                        and it is valid until __gc
                    */
                    rc=sqlite3_bind_double(stmt, q_index, real);
                    lua_pop(L, 1);
                    break;
                }
                if (lua_equal(L, -1, lua_upvalueindex(2))){
                    int ivar=*((int *)p);
                    /*
                        we dont need to check (ivar & type)
                        string was initialized once with final ref
                        and it is valid until __gc
                    */
                    lua_rawgeti(L, LUA_REGISTRYINDEX, ivar);
                    size_t sz;
                    const char * blob = lua_tolstring(L, -1, &sz);// we dont need to check (string was initialized once and will never gone until __gc)
                    rc=sqlite3_bind_blob(stmt,q_index,blob,sz,SQLITE_TRANSIENT);
                    lua_pop(L, 2);
                    break;
                }
                lua_pop(L, 1);
            }
        }
        default: luaL_error(L,"invalid type");return ;
    } 
    xsqlite__stmt_check_rc(L, stmt, rc);
}

static inline void xsqlite__stmt_bind_all(lua_State * L,sqlite3_stmt * stmt){
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
        xsqlite__stmt_bind(L,stmt,i,-1);
    }
}
// upvalue 1 is real's mt
// upvalue 2 is blob's mt
static int xsqlite_stmt_bind(lua_State * L){
    sqlite3_stmt * stmt=xsqlite__tostmt(L);
    int type=lua_type(L,2);
    if (type==LUA_TTABLE)
        xsqlite__stmt_bind_all(L,stmt);
    else{
        int q_index;
        if (type==LUA_TSTRING){
            q_index=sqlite3_bind_parameter_index(stmt,lua_tostring(L, 2));
        }else
            q_index=lua_tointeger(L,2);
        xsqlite__stmt_bind(L,stmt,q_index,3);
    }
    return 0;
}



static void xsqlite__stmt_col(lua_State * L,sqlite3_stmt * stmt,int q_index){
    int t=sqlite3_column_type(stmt,q_index);
    if (t==SQLITE_INTEGER){
        sqlite_int64 i64 = sqlite3_column_int64(stmt,q_index);
        lua_Integer i = (lua_Integer )i64;
        if (i == i64) {
            lua_pushinteger(L, i);
            return;
        }
        lua_Number n = (lua_Number)i64;
        if (n == i64) {
            lua_pushnumber(L, n);
            return;
        }
        t=SQLITE_TEXT;
    }
    switch (t) {
        case SQLITE_TEXT:lua_pushlstring(L,(const char *)sqlite3_column_text(stmt,q_index),sqlite3_column_bytes(stmt,q_index));break;
        case SQLITE_NULL:lua_pushnil(L);break;
        case SQLITE_FLOAT:lua_pushnumber(L,sqlite3_column_double(stmt,q_index));break;
        case SQLITE_BLOB:lua_pushlstring(L,sqlite3_column_blob(stmt,q_index),sqlite3_column_bytes(stmt,q_index));break;
        default:lua_pushnil(L);
    }
}

static int xsqlite_stmt_step(lua_State * L){
    sqlite3_stmt * stmt=xsqlite__tostmt(L);
    int rc=sqlite3_step(stmt);
    if (rc==SQLITE_DONE){
        return 0;
    }
    if (rc==SQLITE_ROW){
        lua_pushboolean(L, 1);
        return 1;
    }
    xsqlite__stmt_check_rc(L, stmt, rc);
    return luaL_error(L, "unexpected rc %d",rc);
}


static const char *sqlite_type_name[] = {
    [SQLITE_INTEGER] = "integer",
    [SQLITE_FLOAT]   = "real",
    [SQLITE_TEXT]    = "text",
    [SQLITE_BLOB]    = "blob",
    [SQLITE_NULL]    = "null"
};


static int xsqlite_stmt_meta(lua_State * L){
    sqlite3_stmt * stmt=xsqlite__tostmt(L);
    int count=sqlite3_column_count(stmt);
    lua_createtable(L, count, count);
    for(int i=0;i<count;i++){
        int type=sqlite3_column_type(stmt,i);
        lua_pushinteger(L, i);
        lua_pushstring(L, sqlite_type_name[type]);
        lua_rawset(L, -3);
        const char * colname = sqlite3_column_name(stmt, i);
        if (colname){
            lua_pushstring(L,colname);
            lua_pushinteger(L, i);
            lua_rawset(L, -3);
        }
    }
    return 1;
}


static int xsqlite_stmt_col(lua_State * L){
    sqlite3_stmt * stmt=xsqlite__tostmt(L);
    int icol=-1;
    int count;
    if (lua_type(L, 2)==LUA_TSTRING){
        const char * name=lua_tostring(L, 2);
        count=sqlite3_column_count(stmt);
        for(int i=0;i<count;i++){
            const char * colname = sqlite3_column_name(stmt, i);
            if (colname && strcmp(name,colname)==0){
                icol=i;
                break;
            }
        }
        if (icol==-1) 
            return luaL_error(L,"col with name '%s' is not found",name);
    }else{
        icol=luaL_checkint(L, 2);
        count=sqlite3_column_count(stmt);
        if (icol<0 || icol>=count)
            return luaL_error(L,"col index %d is out of range [0 - %d]",icol,count-1);
    }
    xsqlite__stmt_col(L,stmt,icol);
    return 1;
}


static int xsqlite_stmt_irow(lua_State * L){
    sqlite3_stmt * stmt=xsqlite__tostmt(L);
    int count=sqlite3_column_count(stmt);
    lua_createtable(L, count, 0);
    for(int i=0;i<count;){
        xsqlite__stmt_col(L,stmt,i);
        lua_rawseti(L,-2,++i);
    }
    return 1;
}

static int xsqlite_stmt_row(lua_State * L){
    sqlite3_stmt * stmt=xsqlite__tostmt(L);
    int count=sqlite3_column_count(stmt);
    lua_createtable(L, 0, count);
    for(int i=0;i<count;i++){
        const char * name = sqlite3_column_name(stmt, i );
        if (name){
            lua_pushstring(L, name);
            xsqlite__stmt_col(L,stmt,i);
            lua_rawset(L, -3);
        }
    }
    return 1;
}

static int xsqlite_stmt_reset(lua_State * L){
    sqlite3_stmt * stmt=xsqlite__tostmt(L);
    int rc=sqlite3_reset(stmt);
    xsqlite__stmt_check_rc(L, stmt, rc);
    lua_settop(L, 1);
    return 1;
}

static int xsqlite_stmt_clear(lua_State * L){
    sqlite3_stmt * stmt=xsqlite__tostmt(L);
    int rc=sqlite3_clear_bindings(stmt);
    xsqlite__stmt_check_rc(L, stmt, rc);
    lua_settop(L, 1);
    return 1;
}






static const struct luaL_Reg xsqlite_stmt_mt[] = {
    //{"bind",xsqlite_stmt_bind},
    {"step",xsqlite_stmt_step},
    {"meta",xsqlite_stmt_meta},
    {"col",xsqlite_stmt_col},
    {"row",xsqlite_stmt_row},
    {"irow",xsqlite_stmt_irow},
    {"reset",xsqlite_stmt_reset},
    {"clear",xsqlite_stmt_clear},
    {"finalize",xsqlite_stmt_finalize},  
    {"is_readonly",xsqlite_stmt_is_readonly},
    {"__gc",xsqlite_stmt_gc},
    {NULL,NULL}
};



static const struct luaL_Reg xsqlite_db_mt[] = {
    {"exec",xsqlite_db_exec},
    {"close",xsqlite_db_close},
    {"is_readonly",xsqlite_db_is_readonly},
    {"name",xsqlite_db_name},
    {"filename",xsqlite_db_filename},
    {"changes",xsqlite_db_changes},
    {"is_autocommit",xsqlite_is_autocommit},
    {"id",xsqlite_db_id},
    {"__gc",xsqlite_db_gc},
    {NULL,NULL}
};



static const struct luaL_Reg xsqlite_fn[] = {
    {"version",xsqlite_version},
    {NULL,NULL}
};





LUA_API int luaopen_xsqlite(lua_State * L){

    //lsqite_real_mt
    lua_newtable(L);/*no need for gc (primitve)*/
    int real_mt=lua_gettop(L);

    //xsqlite_blob_mt
    lua_newtable(L);
    int blob_mt=lua_gettop(L);
    lua_pushcfunction(L,xsqlite_blob_gc);
    lua_setfield(L, -2, "__gc");

    //xsqlite_stmt_mt
    lua_newtable(L);
    int stmt_mt=lua_gettop(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushvalue(L,real_mt);//real_mt
    lua_pushvalue(L,blob_mt);//blob_mt
    lua_pushcclosure(L,xsqlite_stmt_bind,2);
    lua_setfield(L, -2, "bind");
    luaL_setfuncs(L,xsqlite_stmt_mt, 0);   
    
    
    //xsqlite_db_mt
    lua_newtable(L);
    int db_mt=lua_gettop(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushvalue(L, stmt_mt);
    lua_pushcclosure(L, xsqlite_db_prepare, 1);
    lua_setfield(L, -2, "prepare");
    luaL_setfuncs(L,xsqlite_db_mt, 0); 
    

    //xsqlite
    lua_newtable(L);
    lua_pushvalue(L, real_mt);
    lua_pushcclosure(L, xsqlite_real, 1);
    lua_setfield(L, -2, "real");
    lua_pushvalue(L, blob_mt);
    lua_pushcclosure(L, xsqlite_blob, 1);
    lua_setfield(L, -2, "blob");
    lua_pushvalue(L, real_mt);
    lua_pushvalue(L, blob_mt);
    lua_pushvalue(L, stmt_mt);
    lua_pushvalue(L, db_mt);
    lua_pushcclosure(L, xsqlite_type, 4);
    lua_setfield(L, -2, "type");
    lua_pushvalue(L, db_mt);
    lua_pushcclosure(L, xsqlite_open, 1);
    lua_setfield(L, -2, "open");
    luaL_setfuncs(L,xsqlite_fn, 0);
    
    return 1;
}

