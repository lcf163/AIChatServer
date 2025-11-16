# HttpServer

## 项目介绍
本项目是基于C++实现自定义的 HttpServer 框架，该项目包括 HTTP/HTTPS 支持、动态路由处理、会话管理等功能。
### HttpServer 目录结构
```
HttpServer/
├── include/
│   ├── http/
│   │   ├── HttpContext.h
│   │   ├── HttpRequest.h
│   │   ├── HttpResponse.h
│   │   └── HttpServer.h
│   ├── router/
│   │   ├── Router.h
│   │   └── RouterHandler.h
│   ├── middleware/
|   |   ├── MiddlewareChain.h
|   |   ├── Middleware.h
|   |   └── cors/
|   |       ├── CorsConfig.h
|   |       └── CorsMiddleware.h
│   ├── session/                
│   │   ├── Session.h
│   │   ├── SessionManager.h
│   │   └── SessionStorage.h
│   ├── ssl/
│   │   ├── SslContext.h
│   │   ├── SslConfig.h
│   │   ├── SslConnection.h
│   │   └── SslTypes.h
│   └── utils/
│       ├── FileUtil.h
│       ├── JsonUtil.h
│       ├── MysqlUtil.h
│       └── db/
│           ├── DbConnection.h
│           ├── DbConnectionPool.h
│           └── DbException.h
└── src/
    ├── http/
    │   ├── HttpContext.cpp
    │   ├── HttpRequest.cpp
    │   ├── HttpResponse.cpp
    │   └── HttpServer.cpp
    ├── router/
    │   └── Router.cpp
    ├── middleware/
    │   ├── MiddlewareChain.cpp
    │   └── cors/
    │       └── CorsMiddleware.cpp
    ├── session/                
    │   ├── Session.cpp
    │   ├── SessionManager.cpp
    │   └── SessionStorage.cpp
    ├── ssl/
    │   ├── SslContext.cpp
    │   ├── SslConfig.cpp
    │   └── SslConnection.cpp
    └── utils/
        ├── FileUtil.cpp
        └── db/
            ├── DbConnection.cpp
            └── DbConnectionPool.cpp
```
### HttpServer 模块
- **网络模块**：基于Muduo网络库实现，Muduo网络库提供了基于Reactor模式的高性能网络编程框架，支持多线程和事件驱动的I/O多路复用，简化了C++网络应用的开发。
- **HTTP模块**：用于处理HTTP请求和响应，包括请求的解析、响应的生成和发送。
- **路由模块**：用于管理HTTP请求的路由，根据请求路径和方法将其路由到适当的处理器。支持动态路由和静态路由。
- **中间件模块**：处理 HTTP 请求和响应的函数或组件，它在客户端请求到达服务器处理逻辑之前、或者服务器响应返回客户端之前执行
- **会话管理模块**：基于Session实现，Session是一种用于管理用户会话状态的技术，它可以在多个请求之间保持用户状态的一致性。
- **数据库模块**：数据库连接池通过复用数据库连接来提高应用程序的性能和资源利用效率，减少连接创建和销毁的开销。
- **SSL模块**：用于处理HTTPS请求和响应，包括请求的解析、响应的生成和发送。

## 项目环境
### 环境依赖
- 操作系统：Ubuntu 22.04
- 数据库：mysql 8.0
- boost_1_69_0
- muduo
- nlohmann/json
- libmysqlcppconn-dev
- openssl

### 配置环境
编译环境通过linux命令安装，需要时自行上网搜索相应命令。
#### 编译器相关
```sh
# 更新包管理器
sudo apt-get update

# 安装基础开发工具
sudo apt-get install -y build-essential cmake git
```
#### MySQL 安装
```sh
sudo apt-get install -y mysql-server
sudo systemctl start mysql
sudo systemctl enable mysql
# 创建数据库
# 导入数据库结构
```
#### 第三方库
```sh
# boost
wget https://archives.boost.io/release/1.69.0/source/boost_1_69_0.tar.gz
tar -zxvf boost_1_69_0
cd boost_1_69_0
./bootstrap.sh
./b2
sudo ./b2 insta11
cd ..

# muduo
git clone https://github.com/chenshuo/muduo.git
cd muduo
./build.sh -j4
sudo cp -r build/release-cpp11/lib/* /usr/local/lib/
sudo cp -r muduo/base/* /usr/local/include/
sudo cp -r muduo/net/* /usr/local/include/
cd ..

# nlohmann/json
sudo apt upgrade
sudo apt install nlohmann-json3-dev

# mysqlcppconn
sudo apt install libmysqlcppconn-dev

# OpenSSL
sudo apt install libssl-dev
```

## 项目编译
```sh
# 在项目根目录下创建build目录，并进入该目录
mkdir build
cd build

# 执行 cmake 命令
cmake ..

# 编译生成可执行文件
make
```  

## 项目运行
默认运行在 8080 端口，可以加上 -p 端口号来指定。
```sh
sudo ./http_server
```  
服务启动后，浏览器访问自己对应的 ip:端口号  

## 总结
- HttpServer 是一个基于 C++ 的高性能 HTTP 服务器框架，简化 Web 应用的开发。
- 结合 Reactor 模型、OpenSSL 和多线程技术，HttpServer 提供了高效的请求处理、安全通信和并发支持。
- 支持动态路由、中间件和会话管理，开发者专注于业务逻辑的实现。
