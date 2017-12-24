## description
这是一个基于Linux的简单的聊天室服务器
- 纯c语言编写
- 基于tcp
- 基于高性能epoll i/o复用模式
## install
```
make epoll_server.o
```
## usage
在终端开启多个telnet,然后多个telnet和server之间就建立起了聊天室
```
telnet 127.0.0.1 12345
```
