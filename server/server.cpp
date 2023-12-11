#include "server.h"

char* name = "my_server";
server::server() : server_sock{}{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    server_sock.sock_bind((struct sockaddr *) &addr, sizeof(addr));
    server_sock.sock_listen();
}

void server::accept(){
    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addrlen = sizeof(client_addr);

        int new_socket = server_sock.sock_accept((struct sockaddr *)&client_addr, &client_addrlen);

        mapMutex.lock();
        clients[new_socket] = client{new_socket,client_addr};
        mapMutex.unlock(); // provent map be manapulated by two threads


        pthread_t new_client;
        client client_ = client{new_socket, client_addr};
        if(pthread_create(&new_client,nullptr, client_process , & client_ ) != 0)
        {
            std::cout << "thread create error" << std::endl;
            exit(-1);
        }

    }
}

void *client_process(void * ptr){
    client * new_client =  (client *)ptr;
    int client_fd = new_client->sock_fd;
    Message m;

    char buf[MAXLEN];
    int bytegot;

    std::cout << "connect established:" << std::endl  << "client_sock:" <<  client_fd <<std::endl;
    while(1)
    {
        bytegot = recv(client_fd, buf, MAXLEN, 0);
        if(bytegot <= 0 )
        {
            std::cout << "something wrong" << std::endl;
            exit(-1);
        }

        char client_type = buf[0] ;


        map<int, client> temp;

        if(client_type == 't')
        {
            std::cout << "tiem request from " << client_fd << std::endl;
            m.type = 't';
            time_t currentTime;
            time(&currentTime);
            memcpy(m.info,&currentTime, sizeof(currentTime));
            send(client_fd, &m, 1 + sizeof(currentTime), 0);
        }
        else if(client_type == 'n')
        {
            std::cout << "name request from " << client_fd << std::endl;
            m.type = 'n';
            memcpy(m.info, name, strlen(name));
            send(client_fd, &m, 1 + strlen(name),0);
        }
        else if(client_type == 'l')
        {
            std::cout << "list request from " << client_fd << std::endl;
            m.type = 'l';
            int  pos = 0;
            
            mapMutex.lock();
            temp = clients;
            mapMutex.unlock();

            for(auto it : temp)
            {
                memcpy(m.info+pos, &it.first, sizeof(int));
                pos += 4;
            }
            send(client_fd, &m, 1 + pos ,0);

        }
        else if(client_type == 's')
        {
            int next_fd = *(int *) buf+1;
            bool find = false;

            m.type = 's'; //transfer message
            
            mapMutex.lock();
            auto it = clients.find(next_fd);
            find = it != clients.end();
            mapMutex.unlock();

            if(find = false)
                std::cout << "client not found" << std::endl;
            else
                send(next_fd,buf + 1 + sizeof(int),bytegot - 1- sizeof(int),0);

        }
        else if(client_type == 'd')
        {
            close(client_fd);

            std::cout<< "client_server " << client_fd << " closed" << std::endl;

            pthread_exit(NULL);
        }
        else{
            std::cout << "message type error" << std::endl;
        }
    }
}

int main()
{
    server myserver;
    myserver.accept();
}