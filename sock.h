#ifndef __sock__H
#define __sock__H

#include "message.h"
#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
class sock{
private:
    sock() : connected{false}, binded{false}
    {
        socket_id = socket(AF_INET, SOCK_STREAM, 0);
        if(socket_id < 0)
            std::cout << "socket created failed" << std::endl;
    }    

    bool connected;
    bool binded;
    int socket_id;

public:
    void sock_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
    {
        if(!binded)
        {
            std::cout << "please bind the socket" << std::endl;
            return ;
        }
        if(connect(socket_id, addr, addrlen) < 0)
        {
            std::cout << "connect fail" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    void sock_close()
    {
        if(close(socket_id) == -1);
        {
            std::cout << "close fail" << std::endl;
            exit(EXIT_FAILURE);
        }
        connected = false;
    }
    void sock_bind(const struct sockaddr *addr, socklen_t addrlen)
    {
        if(bind(socket_id, addr, addrlen) < 0);
        {
            std::cout << "bind fail" << std::endl;
            return ;
        }
        binded = true;
    }
    ssize_t sock_send( message m, int flags = 0)
    {
        if(!connected)
        {
            std::cout << "please connected" << std::endl;
            return -1;
        }
        send(socket_id, &m, m.len + 1, flags); //type + info
    }
    message sock_recv(   int flags = 0)
    {
        char buf[MAXLEN];
        int len;
        message got;
    
        if(!connected)
        {
            std::cout << "please connected" << std::endl;
            got.type = -1;
            return got;
        } 

        len = recv(socket_id, buf, MAXLEN , flags);
        got.type = buf[0];
        memcpy(got.info, buf + 1, len - 1);
        got.len = len - 1;
    }
};  

#endif