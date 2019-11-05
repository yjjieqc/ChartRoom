#include <iostream>
#include "Server.h"
 
using namespace std;

// 服务器端成员函数

// 服务器类构造函数

Server::Server(){
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    listener = 0;

    epfd = 0;
}

void Server::Init(){
    cout << "Init Server ..." <<endl;

    listener = socket(PF_INET, SOCK_STREAM, 0);
    if(listener < 0){
        perror("listener error.\n");
        exit(-1);
    }

    if(bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
        perror("bind error.\n");
        exit(-1);
    }

    cout << "Start to listen: " << SERVER_IP <<endl;

    epfd = epoll_create(EPOLL_SIZE);

    if(epfd < 0){
        perror("epoll error.\n");
        exit(-1);
    }
    
    addfd(epfd, listener, true);
}

void Server::Close(){
    close(listener);
    close(epfd);
}

int Server::SendBroadcastMessage(int clientfd){
    char recv_buf[BUF_SIZE];
    char send_buf[BUF_SIZE];
    Msg msg;
    bzero(recv_buf, BUF_SIZE);

    cout << "read from client(clientID = " << clientfd << ")" <<endl;
    int len = recv(clientfd, recv_buf, BUF_SIZE, 0);

    memset(&msg, 0, sizeof(msg));
    memcpy(&msg, recv_buf, sizeof(msg));
    msg.fromID = clientfd;
    if(msg.content[0] == '\\' && isdigit(msg.content[1])){
        msg.type = 1;
        msg.toID = msg.content[1] - '0';
        memcpy(msg.content, msg.content+2, sizeof(msg.content));
    }
    else
    {
        msg.type = 0;
    }

    if(len == 0){
        close(clientfd);

        clients_list.remove(clientfd);
        cout << "ClientID = "<< clientfd
        <<"closed.\n now there are "
        << clients_list.size()<< " client in the room."
        <<endl;
    }
    else
    {
        if(clients_list.size()== 1){
            memcpy(&msg.content, CAUTION, sizeof(msg.content));
            bzeor(send_buf, BUF_SIZE);
            memcpy(send_buf, &msg, sizeof(msg));
            send(clientfd, send_buf, sizeof(send_buf), 0);
            return len;
        }

        char format_message[BUF_SIZE];

        if(msg.type == 0){
            sprintf(format_message, SERVER_MESSAGE, clientfd, msg.content);
            memcpy(msg.content, format_message, BUF_SIZE);
            list<int>::iterator it;
            for(it = clients_list.begin(); it!= clients_list.end(); ++it){
                if(*it != clientfd){
                    bzero(send_buf, sizeof(msg));
                    memcpy(send_buf, &msg, sizeof(msg));
                    if(send(*it, send_buf, sizeof(send_buf), 0) < 0){
                        return -1;
                    }
                }
            }
        }

        if(msg.type ==1){
            bool private_offline = true;
            sprintf(format_message, SERVER_PRIVATE_MESSAGE, clientfd, msg.content);
            memcpy(msg.content, format_message, BUF_SIZE);

            list<int>::iterator it;
            for(it = clients_list.begin(); it != clients_list.end(); ++it){
                if(*it== msg.toID){
                    private_offline = false;
                    bzero(send_buf, BUF_SIZE);
                    memcpy(send_buf, &msg, sizeof(msg));
                    if(send(*it, send_buf, sizeof(send_buf), 0) < 0){
                        return -1;
                    }
                }
            }

            if(private_offline){
                sprintf(format_message, SERVER_PRIVATE_ERROR_MESSAGE, msg.toID);
                memcpy(msg.content, format_message, BUF_SIZE);
                bzero(send_buf, BUF_SIZE);
                memcpy(send_buf, &msg, sizeof(msg));
            }
        }
    }      
    return len;
}


void Server::Start() {
 
    // epoll 事件队列
    static struct epoll_event events[EPOLL_SIZE]; 
 
    // 初始化服务端
    Init();
 
    //主循环
    while(1)
    {
        //epoll_events_count表示就绪事件的数目
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
 
        if(epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }
 
        cout << "epoll_events_count =\n" << epoll_events_count << endl;
 
        //处理这epoll_events_count个就绪事件
        for(int i = 0; i < epoll_events_count; ++i)
        {
            int sockfd = events[i].data.fd;
            //新用户连接
            if(sockfd == listener)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept( listener, ( struct sockaddr* )&client_address, &client_addrLength );
 
                cout << "client connection from: "
                     << inet_ntoa(client_address.sin_addr) << ":"
                     << ntohs(client_address.sin_port) << ", clientfd = "
                     << clientfd << endl;
 
                addfd(epfd, clientfd, true);
 
                // 服务端用list保存用户连接
                clients_list.push_back(clientfd);
                cout << "Add new clientfd = " << clientfd << " to epoll" << endl;
                cout << "Now there are " << clients_list.size() << " clients int the chat room" << endl;
 
                // 服务端发送欢迎信息  
                cout << "welcome message" << endl;                
                char message[BUF_SIZE];
                bzero(message, BUF_SIZE);
                sprintf(message, SERVER_WELCOME, clientfd);
                int ret = send(clientfd, message, BUF_SIZE, 0);
                if(ret < 0) {
                    perror("send error");
                    Close();
                    exit(-1);
                }
            }
            //处理用户发来的消息，并广播，使其他用户收到信息
            else {   
                int ret = SendBroadcastMessage(sockfd);
                if(ret < 0) {
                    perror("error");
                    Close();
                    exit(-1);
                }
            }
        }
    }
 
    // 关闭服务
    Close();
}