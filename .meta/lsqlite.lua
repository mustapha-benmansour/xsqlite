

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
---@alias lsqlite.stmt-row {[string|integer]:nil|string|integer|number}



function M_stmt:finalize() end

---@return boolean
function M_stmt:is_readonly() end

---@param key lsqlite.stmt-bind-key
---@param value lsqlite.stmt-bind-val
---@param alt true? # tell the function to use the correspondant alternative type of value's type (true for string to use blob)(true for number to use integer)
---@overload fun(self:lsqlite.stmt*,obj:lsqlite.stmt-bind-obj)
function M_stmt:bind(key,value,alt) end


---@generic T:table
---@param row T? # optional table to fill with row data if available.
---@return (true|T)? # `true` if expected result is obtained, `false` if no more rows (only when `row` is given).
function M_stmt:next(row) end


function M_stmt:reset() end


function M_stmt:clear() end

return M