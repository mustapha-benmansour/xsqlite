

---@meta xsqlite
local M={}



---@class xsqlite.real*

---@class xsqlite.blob*



---@return string
function M.version() end

---@alias xsqlite.type
---|'real'
---|'blob'
---|'database'
---|'closed database'
---|'statement'
---|'finalized statement'

---@param obj any|xsqlite.db*|xsqlite.stmt*|xsqlite.real*|xsqlite.blob*
---@return xsqlite.type|type
function M.type(obj) end


---@param number number?
---@return xsqlite.real*
function M.real(number) end

---@param blob string?
---@return xsqlite.blob*
function M.blob(blob) end



---@alias xsqlite-open-flag 
---|'readonly'
---|>'readwrite'
---|>'create'

---@param file string
---@param ... xsqlite-open-flag? # mode
---@return xsqlite.db*
function M.open(file,...) end




---@class xsqlite.db*
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
---@return xsqlite.stmt*
function M_db:prepare(query) end


---@class xsqlite.stmt*
local M_stmt={}

---@alias xsqlite.stmt-bind-key string|integer
---@alias xsqlite.stmt-bind-val nil|string|integer|boolean|xsqlite.real*|xsqlite.blob* 
---@alias xsqlite.stmt-bind-obj {[xsqlite.stmt-bind-key]:xsqlite.stmt-bind-val}


function M_stmt:finalize() end

---@return boolean
function M_stmt:is_readonly() end

---@param key xsqlite.stmt-bind-key
---@param value xsqlite.stmt-bind-val
---@overload fun(self:xsqlite.stmt*,obj:xsqlite.stmt-bind-obj)
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
---@return {[string]:nil|string|number}|table # we use table to ignore lsp warnings
function M_stmt:row() end

-- make sure that `s:step()` returned true before calling this function.
---@return {[integer]:nil|string|number}|table # we use table to ignore lsp warnings
function M_stmt:irow() end

-- make sure that `s:step()` returned true before calling this function.
---@param n string|integer # 0 based index or name
---@return nil|string|number value
function M_stmt:col(n) end


function M_stmt:reset() end


function M_stmt:clear() end

return M