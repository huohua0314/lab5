#ifndef __SERVER__H
#define __SERVER__H

#include "../sock.h"
#include "time.h"
#include <map>
#include <pthread.h>
#include <mutex>

using namespace std;

#define port 3156
typedef struct client client;

map<int ,client> clients;
mutex mapMutex;

struct client{
    int sock_fd;
    struct sockaddr_in addr;
};

class server{
private:
    
    sock server_sock;
    
public:
    server();
    void accept();
    
};

void *client_process(void * ptr);

#endif
