local N=100000

do
    local os_clock=os.clock()
    local Q=require'lsqlite'
    --print(Q.version())
    local db=Q.open('test.db')
    print('db type',Q.type(db))
    db:close()
    print('db type',Q.type(db))
    db=Q.open('test.db')

    local time=os.time()
    db:exec[[
    --PRAGMA page_size = 4096;
    --PRAGMA cache_size = 1000000;
    PRAGMA synchronous = OFF;
    PRAGMA journal_mode = OFF;
    PRAGMA locking_mode = EXCLUSIVE;
    PRAGMA encoding="UTF-8";
    --PRAGMA foreign_keys=OFF;
    --PRAGMA ignore_check_constraints = ON;
    --PRAGMA temp_store = MEMORY;
    ]]
    db:prepare(string.format('create table if not exists _%s (id integer primary key autoincrement,code integer,name text,data blob,price real)',time)):done()
    local stmt =db:prepare(string.format('insert into _%s (code,name,data,price) values(:code,:name,:data,:price)',time))
    print('stmt type',Q.type(stmt))
    local binary_data = string.char(0x00, 0x01, 0x02, 0x03, 0x04)
    --local bind={data={binary_data},price={}}
    for i = 1, N do
        --stmt:bind_all('args',i,'name'..i)
        --stmt:bind(1,i)
        --stmt:bind(2,'name'..i)
        --stmt:bind(3,{binary_data});
        --stmt:bind(4,{i});
        --bind.code=1
        --bind.name='name'..i
        --bind.data[1]=binary_data
        --bind.price[1]=i
        --stmt:bind_all{code=1,name='name'..i,data={binary_data},price={i}}
        --stmt:bind_all(bind)
        stmt:bind(':code',1)
        stmt:bind(':name','name'..i)
        stmt:bind(':data',{binary_data})
        stmt:bind(':price',{i})
        stmt:done()
    end
    stmt:finalize()
    print('stmt type',Q.type(stmt))
    print(string.format('insert took %.3fs',(os.clock()-os_clock)))
    os_clock=os.clock()
    local stmt=db:prepare(string.format('select * from _%s',time))
    print('stmt type',Q.type(stmt))
    for row in stmt:rows() do
        local id,code,name,data,price=unpack(row)
        if id==(N/2) then
            print('ROW ',id,code,name,data,price)
        end
    end   
    stmt:finalize()
    print('stmt type',Q.type(stmt))
    print(string.format('select took %.3fs',(os.clock()-os_clock)))
end