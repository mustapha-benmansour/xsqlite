

---@meta lsqlite
local M={}




---@return string
function M.version() end

---@param obj lsqlite.db*|lsqlite.stmt*
---@return 'database'|'closed database'|'statement'|'finalized statement'|nil
function M.type(obj) end

---@param file string
---@param mode integer?
---@return lsqlite.db*
function M.open(file,mode) end




---@class lsqlite.db*
local M_db={}


function M_db:close() end


---@param query string
function M_db:exec(query) end

---@return integer
function M_db:changes() end

---@return boolean
function M_db:get_autocommit() end

---@return integer?
function M_db:id() end

---@param query string
---@return lsqlite.stmt*
function M_db:prepare(query) end


---@class lsqlite.stmt*
local M_stmt={}

---@alias lsqlite.stmt-bind-val nil|string|integer|boolean|{[1]:number}|{[1]:string} 
---@alias lsqlite.stmt-row-val nil|string|integer|boolean|number

function M_stmt:finalize() end

---@param key string|integer 
---@param value lsqlite.stmt-bind-val
function M_stmt:bind(key,value) end


---@param values {[integer]:lsqlite.stmt-bind-val,[string]:lsqlite.stmt-bind-val}
function M_stmt:bind_all(values) end


function M_stmt:done() end

---@return {[integer]:lsqlite.stmt-row-val,[string]:lsqlite.stmt-row-val}
function M_stmt:row() end

---@return fun():{[integer]:lsqlite.stmt-row-val,[string]:lsqlite.stmt-row-val}
function M_stmt:rows() end


return M