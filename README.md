# HTTP throughput cache 
## _written in C, with PostgreSQL and Tarantool_

[![](https://fit-m.org/public/img/speakers/ptcodatacolor.png)](https://picodata.io/)


## Features

- HTTP API, can be used via Postman or Thunderbird;
- Uses in-memory cache to improve throughput;
- PostgreSQL - the best database management system;
- ANSI C;
- Cmake config to build project by yourself;

## HTTP data schema
| Request           | HTTP Method | Request body   | Asnwer                |
|----------------|-------------|----------------|---------------------  |
| /data          | PUT         | {data: string "any string data"} | id: string uuid          |
|/data?id=my_id  | GET         |                | {id: string my_id,  data: string my_data}     |

## Launch guide
### Pre-requisites
Before we proceed:
1. Install the PostreSQL (author uses Ubuntu)
    ```sh
    sudo apt install postgresql postgresql-contrib
    ```
2. Install [Tarantool](https://www.tarantool.io/en/download/os-installation/ubuntu/)
3. Download and install [C client library](https://github.com/tarantool/tarantool-c) and [msgpuck library](https://github.com/tarantool/msgpuck)
### Initialize PostgreSQL
0. If you cant login into Posgress, then follow this [guide](https://gist.github.com/AtulKsol/4470d377b448e56468baef85af7fd614)
1. Start PostreSQL service
    ```sh
    sudo sudo service postgresql start
    ```
2. Login into psql
    ```sh
    sudo -u postgres psql
    ```
3. Create new user
    ```sql
    CREATE USER pico WITH PASSWORD 'pico';
    ```
4. Create a new database:
    ```sql
    CREATE DATABASE picodb;
    ```
5. Grant the user permission to access the database:
    ```sql
    GRANT ALL PRIVILEGES ON DATABASE picodb TO pico;
    ALTER USER pico CREATEDB;
    GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO pico;
    ```
6. Exit from PostgreSQL 
    ```sql
    \q
    ```
7. Login into PostgreSQL as user and enter password:
    ```sh
    psql -U pico -d picodb
    ```
8. Add extension for generating UUIDs
    ```sql
    CREATE EXTENSION "uuid-ossp";
    ```
> **_NOTE:_**  
**Default** PostgreSQL **port** is **8080**. 
It can be configured in **pg_hba.conf** file

### Initialize Tarantool
1. Run Tarantool:
    ```sh
    tarantool
    ```
2. Set listening port:
    ```lua
    box.cfg{ listen = '8081' } 
    ```
> **_NOTE:_**  
**Remember** this **port**.
It will be used to communicate with Tarantool.
Can be confugured in config.txt (located in project folder)

3. Change password for admin:
    ```lua
    box.schema.user.passwd('pass')
    ``` 
4. Create space - Tarantool table:
    ```lua
    s = box.schema.space.create('pico_tarantool')
    s:format({{name = 'id', type = 'string'},{name = 'data', type = 'string'}})---
    s:create_index('primary',{type = 'hash', parts = {'id'}})
    ``` 
5. Get space id
    ```lua
    box.space.pico_tarantool.id
    ``` 
> **_NOTE:_**  
**Remember** this **id**.
It will be used to store data in it.
Can be confugured in config.txt (located in project folder)


### (C)Make source files
1. Project main folder
-src  
    main.c  
    ...  
CMakeLists.txt  
config.txt  
Readme.md  

2. Create build folder
    ```sh
    mkdir build
    ``` 
3. Configure cmake
    ```sh
    cmake -B ./build -S .
    ``` 
> **_NOTE:_**  
**Execute** this **command** in folder, where **CMakeLists.txt** is **located**

4. Change dir to build
    ```sh
    cd ./build
    ``` 
5. Run cmake
    ```sh
    cmake ../CMakeLists.txt
    ``` 
6. Run make
    ```sh
    make
    ```
7. Run compiled file pico with flag -f /path/to/config.txt
    ```sh
    ./pico -f ../config.txt
    ```
### Sending requests
1. Just use Postaman or Thunder Client. Create and send requests, guiding by HTTP data shema provided before.
2. Enjoy :)

## Known issues
1. ~~PUT request: JSON with spaces and  will not work for Tarantool, while in PSQL stores correctly.~~ Fixed
2. ~~If Tarantool goes offline while server is on -  unexpected behavior.~~ Fixed.
3. If PostgreSQL goes offline while server is on - unexpected behavior. Note: Server responses with PSQL error. 
4. If you face any bugs and glitches - be welcome to make Pull Request!

## TO DO
- [x] Proof of concept
- [x] MVP
- [ ] Add  HTTP validation
- [ ] Cover MVP with test
- [ ] Make better query parameters handler
- [ ] Write func, that creates space in Tarantool, if space intially doesn't exist
- [ ] Add pthread support
- [ ] Check connection to PostgreSQL and Tarantool in separate threads
- [ ] Make cache asynchronous
- [ ] Cover code with test
- [ ] More stuff to do
