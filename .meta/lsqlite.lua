

---@meta lsqlite
local M={}




---@return string
function M.version() end

---@param obj lsqlite.db*|lsqlite.stmt*
---@return 'database'|'closed database'|'statement'|'finalized statement'|nil
function M.type(obj) end


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


---@param name string? # schema/database name default is 'main'
---@return boolean
function M_db:is_readonly(name) end

--- returns name of attached database/conn at index i
---@param i integer # 0:main 1:temp +2: attached
---@return string
function M_db:name(i) end

---@param name string? # schema/database name default is 'main'
function M:filename(name) end


--- returns last inserted rowid
---@return integer?
function M_db:id() end

---@param query string
---@return lsqlite.stmt*
function M_db:prepare(query) end


---@class lsqlite.stmt*
local M_stmt={}

---@alias lsqlite.stmt-bind-key string|integer
---@alias lsqlite.stmt-bind-val nil|string|integer|boolean|{[1]:number}|{[1]:string} 
---@alias lsqlite.stmt-bind-obj {[lsqlite.stmt-bind-key]:lsqlite.stmt-bind-val}
---@alias lsqlite.stmt-row {[string|integer]:nil|string|integer|boolean|number}

function M_stmt:finalize() end

---@return boolean
function M_stmt:is_readonly() end

---@param key lsqlite.stmt-bind-key
---@param value lsqlite.stmt-bind-val
function M_stmt:bind(key,value) end


---@param obj lsqlite.stmt-bind-obj
function M_stmt:bind_all(obj) end


function M_stmt:done() end

---@return lsqlite.stmt-row
function M_stmt:row() end

---@return fun():lsqlite.stmt-row
function M_stmt:rows() end


return M