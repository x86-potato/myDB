create table users (
    uid=int,
    name=char32,
    state=char8,
    age=int);
create table products (
    pid =int,
    name=char32,
    price=int,
    stock=int);
create table orders (oid=int, uid=int, pid=int, total=int);

create index on orders (uid);
create index on orders (pid);

load "users.csv" into users;
load "products.csv" into products;
