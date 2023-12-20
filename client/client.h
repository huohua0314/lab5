#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "../sock.h"
#include <semaphore.h>
#include <pthread.h>
// 怎么实现主线程和子线程互发

sem_t bin_sem;
bool is_connect; // 这是为了不用申请socket也能查看是否connect
    
class client{
private:
    
    sock client_sock;
    
public:
    client(){};
    void c_connect(const char *serAddress,int port);
    pthread_t create_thread(void *(*thread_function)(void *));
    int get_clientsockid(){
        return client_sock.get_sockid();
    }
};
message m;
void *thread_receiver(void *arg);
void *thread_sender(void *arg);
int getMessage(message* m,int client_sockfd);
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
#endif