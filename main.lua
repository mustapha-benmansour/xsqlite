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
    local stmt=db:prepare(string.format('create table if not exists _%s (id integer primary key autoincrement,code integer,name text,data blob,price real)',time))
    stmt:step()

    stmt:finalize()
    stmt =db:prepare(string.format('insert into _%s (code,name,data,price) values(:code,:name,:data,:price)',time))
    print('stmt type',Q.type(stmt))
    local blob = Q.blob(string.char(0x00, 0x01, 0x02, 0x03, 0x04))
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
        stmt:bind(':data',blob)
        stmt:bind(':price',Q.real(i))
        stmt:step()
        stmt:reset()
    end
    stmt:finalize()
    print('stmt type',Q.type(stmt))
    print(string.format('insert took %.3fs',(os.clock()-os_clock)))
    local C=0
    ::again::
    C=C+1
    os_clock=os.clock()
    stmt=db:prepare(string.format('select * from _%s',time))
    print('stmt type',Q.type(stmt))
    while stmt:step() do
        local row=stmt:row()
        local id,code,name,data,price=unpack(row)
        if id==(N/2) then
            local meta=stmt:meta()
            for key, value in pairs(meta) do
                print('meta : ',key,value)
            end
            print('ROW ',id,code,name,data,price)
            print('COL id',stmt:col('id'))
            print('COL code',stmt:col('code'))
            print('COL name',stmt:col('name'))
            print('COL data',stmt:col('data'))
            print('COL price',stmt:col('price'))
            print('COL 0',stmt:col(0))
            print('COL 1',stmt:col(1))
            print('COL 2',stmt:col(2))
            print('COL 3',stmt:col(3))
            print('COL 4',stmt:col(4))
        end
    end   
    stmt:finalize()
    db:close()
    print('stmt type',Q.type(stmt))
    print(string.format('select took %.3fs',(os.clock()-os_clock)))
    if C==1 then
        db=Q.open('test.db','readonly')
        goto again
    end
end