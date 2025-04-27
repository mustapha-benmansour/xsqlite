local N=100000

do
    local os_clock=os.clock()
    local Q=require'lsqlite'
    --print(Q.version())
    local db=Q.open('test.db')

    local time=os.time()
    db:exec('PRAGMA page_size = 4096')
    db:exec('PRAGMA cache_size = 1000000')
    db:exec('PRAGMA synchronous = OFF;')
    db:exec('PRAGMA journal_mode = OFF;')
    db:exec('PRAGMA locking_mode = EXCLUSIVE;')
    db:exec('PRAGMA encoding="UTF-8";')
    db:exec('PRAGMA foreign_keys=OFF;')
    db:exec('PRAGMA ignore_check_constraints = ON;')
    db:exec('PRAGMA temp_store = MEMORY;')
    db:prepare(string.format('create table if not exists _%s (id integer primary key autoincrement,code integer,name text,data blob,price real)',time)):done()
    local stmt =db:prepare(string.format('insert into _%s (code,name,data,price) values(?,?,?,?)',time))
    local binary_data = string.char(0x00, 0x01, 0x02, 0x03, 0x04)
    for i = 1, N do
        --stmt:bind_all('args',i,'name'..i)
        --stmt:bind(1,i)
        --stmt:bind(2,'name'..i)
        --stmt:bind(3,{binary_data});
        --stmt:bind(4,{i});
        stmt:bind_all{1,'name'..i,{binary_data},{i}}
        stmt:done()
    end
    local stmt=db:prepare(string.format('select * from _%s',time))
    for row in stmt:rows() do
        local id,code,name,data,price=unpack(row)
        if id==(N/2) then
            print('ROW ',id,code,name,data,price)
        end
    end   
    print(string.format('took %.3fs',(os.clock()-os_clock)))
end