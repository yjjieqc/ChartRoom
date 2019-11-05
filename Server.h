#ifndef CHATROOM_SERVER_H
#define CHATROOM_SERVER_H

#include <string>
#include "Common.h"

using namespace std;

// 服务端类，用来处理客户端请求
class Server
{
private:
    //发送广播消息
    int SendBroadcastMessage(int clientfd);
    // 服务器serverAddress信息
    struct sockaddr_in serverAddr;
    // 创建监听socket
    int listener;
    // epoll_create创建后的返回值
    int epfd;
    // 客户端列表
    list<int> clients_list;
public:
    // 无参构造函数
    Server();
    // 析构函数
    ~Server();
    // 初始化服务器设置
    void Init();
    // 关闭服务器
    void Close();
    // 启动服务器
    void Start();
};

#endif