## 一：项目内容

本项目使用C++实现一个具备服务器端和客户端即时通信且具有私聊功能的聊天室。

目的是学习C++网络开发的基本概念，同时也可以熟悉下Linux下的C++程序编译和简单MakeFile编写

## 二：需求分析

这个聊天室主要有两个程序：

1.服务端：能够接受新的客户连接，并将每个客户端发来的信息，广播给对应的目标客户端。

2.客户端：能够连接服务器，并向服务器发送消息，同时可以接收服务器发来的消息。

即最简单的C/S模型。

## 三：抽象与细化

服务端类需要支持：

1.支持多个客户端接入，实现聊天室基本功能。

2.启动服务，建立监听端口等待客户端连接。

3.使用epoll机制实现并发，增加效率。

4.客户端连接时，发送欢迎消息，并存储连接记录。

5.客户端发送消息时，根据消息类型，广播给所有用户(群聊)或者指定用户(私聊)。

6.客户端请求退出时，对相应连接信息进行清理。

客户端类需要支持：

1.连接服务器。

2.支持用户输入消息，发送给服务端。

3.接受并显示服务端发来的消息。

4.退出连接。

涉及两个事情，一个写，一个读。所以客户端需要两个进程分别支持以下功能。

子进程：

1.等待用户输入信息。

2.将聊天信息写入管道(pipe),并发送给父进程。

父进程：

1.使用epoll机制接受服务端发来的消息，并显示给用户，使用户看到其他用户的信息。

2.将子进程发送的聊天信息从管道(pipe)中读取出来，并发送给客户端。

## 四：C/S模型
TCP服务端通信常规步骤：                                                                                                    

1.socket()创建TCP套接字                                                                              

2.bind()将创建的套接字绑定到一个本地地址和端口上                                        

3.listen()，将套接字设为监听模式，准备接受客户请求                                        

4.accept()等用户请求到来时接受，返回一个对应此连接新套接字   

5.用accept()返回的套接字和客户端进行通信，recv()/send() 接受/发送信息。                               

6.返回，等待另一个客户请求。

7.关闭套接字

TCP客户端通信常规步骤：

1.socket()创建TCP套接字。

2.connect()建立到达服务器的连接。

3.与客户端进行通信，recv()/send()接受/发送信息，write()/read() 子进程写入管道，父进程从管道中读取信息然后send给客户端

5. close() 关闭客户连接。

## 五：相关技术介绍

1.socket 阻塞与非阻塞。

阻塞与非阻塞关注的是程序在等待调用结果时(消息，返回值)的状态。

阻塞调用是指在调用结果返回前，当前线程会被挂起，调用线程只有在得到调用结果之后才会返回。

非阻塞调用是指在不能立刻得到结果之前，该调用不会阻塞当前线程。

eg. 你打电话问书店老板有没有《网络编程》这本书，老板去书架上找，如果是阻塞式调用，你就会把自己一直挂起，守在电话边上，直到得到这本书有或者没有的答案。如果是非阻塞式调用，你可以干别的事情去，隔一段时间来看一下老板有没有告诉你结果。

同步异步是对书店老板而言(同步老板不会提醒你找到结果了，异步老板会打电话告诉你)，阻塞和非阻塞是对你而言。

更多可以参考博客：socket阻塞与非阻塞

socket()函数创建套接字时，默认的套接字都是阻塞的，非阻塞设置方式代码：

//将文件描述符设置为非阻塞方式（利用fcntl函数）
fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)| O_NONBLOCK);
2. epoll

当服务端的人数越来越多，会导致资源吃紧，I/O效率越来越低，这时就应该考虑epoll，epoll是Linux内核为处理大量句柄而改进的poll，是linux特有的I/O函数。其特点如下：

1）epoll是Linux下多路复用IO接口select/poll的增强版本，其实现和使用方式与select/poll大有不同，epoll通过一组函数来完成有关任务，而不是一个函数。

2）epoll之所以高效，是因为epoll将用户关心的文件描述符放到内核里的一个事件列表中，而不是像select/poll每次调用都需要重复传入文件描述符集或事件集（大量拷贝开销），比如一个事件发生，epoll无需遍历整个被监听的描述符集，而只需要遍历哪些被内核IO事件异步唤醒而加入就绪队列的描述符集合即可。

3)epoll有两种工作方式，LT(Level triggered) 水平触发 、ET(Edge triggered)边沿触发。LT是select/poll的工作方式，比较低效，而ET是epoll具有的高速工作方式。更多epoll之ET LT

Epoll 用法（三步曲）：

第一步：int epoll_create(int size)系统调用，创建一个epoll句柄，参数size用来告诉内核监听的数目，size为epoll支持的最大句柄数。

第二步:int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)  事件注册函数

参数 epfd为epoll的句柄。参数op 表示动作 三个宏来表示：EPOLL_CTL_ADD注册新fd到epfd 、EPOLL_CTL_MOD 修改已经注册的fd的监听事件、EPOLL_CTL_DEL从epfd句柄中删除fd。参数fd为需要监听的标识符。参数结构体epoll_event告诉内核需要监听的事件。

第三步：int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout) 等待事件的产生，通过调用收集在epoll监控中已经发生的事件。参数struct epoll_event 是事件队列 把就绪的事件放进去。

eg. 服务端使用epoll的时候步骤如下：

1.调用epoll_create()在linux内核中创建一个事件表。

2.然后将文件描述符(监听套接字listener)添加到事件表中

3.在主循环中，调用epoll_wait()等待返回就绪的文件描述符集合。

4.分别处理就绪的事件集合，本项目中一共有两类事件：新用户连接事件和用户发来消息事件。

## 六：代码结构

src:
-   Common.h
-   Client.h
-   Client.cpp
-   Server.h
-   Server.cpp
-   ClientMain.cpp
-   ServerMain.cpp


每个文件的作用：

1.Common.h：公共头文件，包括所有需要的宏以及socket网络编程头文件，以及消息结构体（用来表示消息类别等）

2.Client.h Client.cpp :客户端类的实现

3.Server.h Server.cpp : 服务端类的实现

4.ClientMain.cpp ServerMain.cpp 客户端及服务端的主函数。

————————————————

版权声明：本文为CSDN博主「CoderWill_Hunting」的原创文章，遵循 CC 4.0 BY-SA 版权协议，转载请附上原文出处链接及本声明。

原文链接：https://blog.csdn.net/lewis1993_cpapa/article/details/80589717