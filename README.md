

##Usage
```lua
local Q=require'lsqlite'
```

```lua
local db=Q.open('data.db','readonly')
print(Q.type(db)) --> database
print(db:is_readonly()) --> true
db:close()
print(Q.type(db)) --> closed database
```

```lua
local db=Q.open('data.db') -- default mode is create+readwrite 
-- Q.open('data.db') similar to Q.open('data.db','create','readwrite')
print(db:is_readonly()) --> false

db:exec'CREATE TABLE IF NOT EXISTS data (id INTEGER PRIMARY KEY AUTOINCREMENT,code INTEGER,name TEXT,image BLOB,price REAL);'
```
```lua
local binary=string.char(0x00, 0x01, 0x02, 0x03, 0x04) 

-- insert (bind map) 
local stmt=db:prepare('INSERT INTO data (code,name,image,price) VALUES (:code,:name,:image,:price);')
print(Q.type(stmt)) --> statement
for i=1,10 do 
    stmt:bind{
        code=i,
        name='name '..i,
        image=Q.blob(binary),
        price=Q.real(i*1.1)
    }
    stmt:step() 
end
stmt:finalize() 
print(Q.type(stmt)) --> finalized statement
```
```lua
-- simple insert (bind array)
local stmt=db:prepare('INSERT INTO data (code,name,image,price) VALUES (?,?,?,?);')
stmt:bind{0,'name 0',Q.blob(binary),Q.real(0)}
stmt:step() 
stmt:finalize() 
```

```lua
-- insert (bind array)
local stmt=db:prepare('INSERT INTO data (code,name,image,price) VALUES (?,?,?,?);')
for i=11,20 do 
    stmt:bind{i,'name '..i,Q.blob(binary),Q.real(i*1.1)}
    stmt:step() 
end
-- with missing fields  
for i=21,30 do 
    stmt:bind{[1]=i,[4]=Q.real(i*1.1)}
    stmt:step() -- missing fields are treated as nil (null in sqlite)
end
stmt:finalize() 
```

```lua
-- insert (bind table=map|array) 
local stmt=db:prepare('INSERT INTO data (code,name,image,price) VALUES (?,?,:image,:price);')
for i=31,40 do 
    stmt:bind{i,'name '..i,image=Q.blob(binary),price=Q.real(i*1.1)}
    stmt:step() 
end
stmt:finalize() 
```

```lua
-- insert (bind values)
local stmt=db:prepare('INSERT INTO data (code,name,image,price) VALUES (?,?,:image,:price);')
for i=41,50 do 
    stmt:bind(1,i)
    stmt:bind(2,'name '..i)
    stmt:bind('image',Q.blob(binary))
    stmt:bind('price',Q.real(i*1.1))
    stmt:step() 
end
-- with missing fields  
for i=51,60 do 
    -- prevent reusing previous values (is not required when binding table) 
    stmt:clear() 
    if i % 2== 0 then
        stmt:bind(2,'name '..i)
        stmt:bind('price',Q.real(i*1.1))
    else
        stmt:bind(1,i)
        stmt:bind('image',Q.blob(binary))
    end
    stmt:step() 
end
-- with shared value
for i=51,60 do 
    if i==51 then
        stmt:bind('image',Q.blob(binary))
    end
    -- we dont need clear (shared image)
    stmt:bind(1,i)
    stmt:bind(2,'name '..i)
    stmt:bind('price',Q.real(i*1.1))
    stmt:step() 
end
stmt:finalize() 
```

```lua
-- insert (bind same table with shared fields)
local stmt=db:prepare('INSERT INTO data (code,name,image,price) VALUES (:code,:name,:image,:price);')
local row={image=Q.blob(binary)}
for i=31,40 do 
    row.id=1
    row.name='name '..i
    row.price=Q.real(i*1.1)
    -- same image is used for all inserts
    stmt:bind(row)
    stmt:step() 
end
stmt:finalize() 
```

```lua
-- insert (bind with same fieds)
local stmt=db:prepare('INSERT INTO data (code,name,image,price) VALUES (:code,:name,:image,:price);')
local row={image=Q.blob(binary)}
for i=31,40 do 
    row.id=1
    row.name='name '..i
    row.price=Q.real(i*1.1)
    -- same image is used for all inserts
    stmt:step() 
end
stmt:finalize() 
```