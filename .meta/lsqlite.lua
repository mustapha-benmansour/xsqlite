

---@meta lsqlite
local M={}



---@class lsqlite.real*

---@class lsqlite.blob*



---@return string
function M.version() end

---@alias lsqlite.type
---|'real'
---|'blob'
---|'database'
---|'closed database'
---|'statement'
---|'finalized statement'

---@param obj any|lsqlite.db*|lsqlite.stmt*|lsqlite.real*|lsqlite.blob*
---@return lsqlite.type|type
function M.type(obj) end


---@param number number?
---@return lsqlite.real*
function M.real(number) end

---@param blob string?
---@return lsqlite.blob*
function M.blob(blob) end



---@alias lsqlite-open-flag 
---|'readonly'
---|>'readwrite'
---|>'create'

---@param file string
---@param ... lsqlite-open-flag? # mode
---@return lsqlite.db*
function M.open(file,...) end




---@class lsqlite.db*
local M_db={}


function M_db:close() end


---@param query string
function M_db:exec(query) end

---@return integer
function M_db:changes() end

--- return true if the given database connection is in autocommit mode
--[[
Autocommit mode is on by default. <br>
Autocommit mode is disabled by a [BEGIN] statement. <br>
Autocommit mode is re-enabled by a [COMMIT] or [ROLLBACK]. <br>
If certain kinds of errors occur on a statement within a multi-statement <br>
transaction (errors including [SQLITE_FULL], [SQLITE_IOERR], <br>
[SQLITE_NOMEM], [SQLITE_BUSY], and [SQLITE_INTERRUPT]) then the <br>
transaction might be rolled back automatically. The only way to <br>
find out whether SQLite automatically rolled back the transaction after <br>
an error is to use this function
]]
---@return boolean
function M_db:is_autocommit() end


---@param name string? # scheme/database name default is 'main'
---@return boolean
function M_db:is_readonly(name) end

--- returns name of attached database/conn at index i
---@param i integer # 0:main 1:temp +2: attached
---@return string name
function M_db:name(i) end

---@param name string? # schema/database name default is 'main'
---@return string name
function M_db:filename(name) end


--- returns last inserted rowid
---@return integer?
function M_db:id() end

---@param query string
---@return lsqlite.stmt*
function M_db:prepare(query) end


---@class lsqlite.stmt*
local M_stmt={}

---@alias lsqlite.stmt-bind-key string|integer
---@alias lsqlite.stmt-bind-val nil|string|integer|boolean|lsqlite.real*|lsqlite.blob* 
---@alias lsqlite.stmt-bind-obj {[lsqlite.stmt-bind-key]:lsqlite.stmt-bind-val}
---@alias lsqlite.stmt-col-key string|integer
---@alias lsqlite.stmt-col-val nil|string|number
---@alias lsqlite.stmt-row {[lsqlite.stmt-col-key]:lsqlite.stmt-col-val}


function M_stmt:finalize() end

---@return boolean
function M_stmt:is_readonly() end

---@param key lsqlite.stmt-bind-key
---@param value lsqlite.stmt-bind-val
---@overload fun(self:lsqlite.stmt*,obj:lsqlite.stmt-bind-obj)
function M_stmt:bind(key,value) end


---@return true?  # true means a new row of data is ready for processing
function M_stmt:step() end

---usage: 
---```lua
--- local stmt=db:prepare'SELECT id,name,image,price FROM tb' -- image may be null
--- assert(stmt:step())
--- local meta=stmt:meta() 
--- local id_index=meta.id 
--- print(id_index) --> 0
--- local name_index=meta.name 
--- print(name_index) --> 1
--- local id_type = meta[id_index]
--- print(id_type) --> 'integer'
--- print(meta[name_index]) --> 'text'
--- print(meta[meta.image]) --> 'blob' or 'null'
--- print(meta[meta.price]) --> 'real'
---```
-- make sure that `s:step()` returned true before calling this function.
---@return {[integer]:'integer'|'real'|'text'|'blob'|'null'}|{[string]:integer} # 
function M_stmt:meta() end

-- make sure that `s:step()` returned true before calling this function.
---@param mode 'i'|'n'|'*'?
---@return lsqlite.stmt-row|table # we use table to ignore lsp warnings
function M_stmt:row(mode) end

-- make sure that `s:step()` returned true before calling this function.
---@param n string|integer # 0 based index or name
---@return lsqlite.stmt-col-val value
function M_stmt:col(n) end


function M_stmt:reset() end


function M_stmt:clear() end

return M