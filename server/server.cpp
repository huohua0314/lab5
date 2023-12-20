#include "server.h"

char* name = "my_server";
server::server() : server_sock{}{
    struct sockaddr_in addr;  // 存储服务器的地址信息
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.53"); // 修改为本机IP
    addr.sin_port = htons(port); // 将端口号从主机字节序转换为网络字节序
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
        mapMutex.unlock(); // prevent map be manipulated by two threads


        // creat a new thread
        pthread_t new_client;
        client client_ = client{new_socket, client_addr};
        if(pthread_create(&new_client,nullptr, client_process , & client_ ) != 0) // &client_: 这是传递给 client_process 函数的参数
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
    Message tm; // message transfer to another client

    char buf[MAXLEN];
    int bytegot;

    std::cout << "connect established:" << std::endl  << "client_sock:" <<  client_fd <<std::endl;
    while(1)
    {
        memset(buf,0,MAXLEN);
        memset(&m,0,sizeof(m));
        std::cout<<"server read again"<<std::endl;
        
        bytegot = recv(client_fd, buf, MAXLEN, 0);  // 返回长度
        if(bytegot <= 0 )
        {
            std::cout << "something wrong" << std::endl;
            exit(-1);
        }

        char client_type = buf[0] ;

        std::cout<<"type:" <<client_type << std::endl;
        std::cout<<"byte:" <<bytegot << std::endl;
        map<int, client> temp;

        if(client_type == 't')
        {
            std::cout << "time request from " << client_fd << std::endl;
            m.type = 't';
            time_t currentTime;
            char* response;
            time(&currentTime);
            response = ctime(&currentTime);
            
            strcpy(m.info,response);
            send(client_fd, &m, 1 + strlen(response), 0);
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
            int offset;
            
            mapMutex.lock();
            temp = clients;
            mapMutex.unlock();

            for(auto it : temp)
            {
                in_port_t client_port = ntohs(it.second.addr.sin_port);
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,&(it.second.addr.sin_addr),ip_str,INET_ADDRSTRLEN);
                offset = sprintf(m.info+pos,"SOCKET_ID: %d | IP: %s | Port: %u\n", it.first, ip_str, client_port );
                pos += offset;
            }

            // pos = 0;
            // for(auto it:temp)
            // {
            //     std::cout<<*(m.info+pos)<<std::endl;
            //     pos+=4;
            // }
            std::cout<<m.info<<std::endl;
            send(client_fd, &m, 1 + pos ,0);

        }
        else if(client_type == 's')
        {
            int next_fd = *(int * ) (buf+1);  // 把buf+1及之后的整数取出来
            int test = *(int * ) (buf+2);
            bool find = false;
            std::cout << "##sdsdsdss##" <<buf[4] <<std::endl;
            std::cout << "send to :"<< next_fd <<std::endl;
            std::cout << "send to :"<< test <<std::endl;

            m.type = 's'; //transfer message
            // tm.type = 'm';

            mapMutex.lock();
            auto it = clients.find(next_fd);
            find = it != clients.end();
            mapMutex.unlock();

            if(find = false)
            {
                std::cout << "client not found" << std::endl;
                m.type = 'u';
                strncpy(m.info, "send unsuccessfully", MAXLEN);
                send(client_fd, &m, 1 + strlen("send unsuccessfully"),0);
            } 
            else // ID|IP|message
            {
                send(next_fd,buf + 1 + sizeof(int),bytegot - 1- sizeof(int),0);
                // char addr_str[INET_ADDRSTRLEN];
                // inet_ntop(AF_INET, &(new_client->addr.sin_addr), addr_str, INET_ADDRSTRLEN);
                // snprintf(tm.info, sizeof(tm.info), "%d%s", client_fd,addr_str);
                // strncat(tm.info, buf + 1 + sizeof(int), bytegot - 1- sizeof(int));
                // if(send(next_fd,&m,strlen(m.info),0)>0)// 这里信息会不会太长？
                // {
                //     strncpy(m.info, "send successfully", MAXLEN);
                //     send(client_fd, &m, 1 + strlen("send successfully"),0);
                // }
            } 
        }
        else if(client_type == 'd')
        {
            m.type = 'd';
            send(client_fd, &m, 1,0);

            close(client_fd);

            std::cout<< "client_server " << client_fd << "closed" << std::endl;

            // 从client中删除关闭了的client
            map<int ,client>::iterator it;
            for(it=clients.begin();it!=clients.end();it++)
            {
                if ((*it).first==client_fd)
                {
                    break;
                }
            }
            mapMutex.lock();
            clients.erase(it);
            mapMutex.unlock();
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
