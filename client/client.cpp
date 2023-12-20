#include "client.h"

// client需要用户输入IP地址和port
void client::c_connect(const char *serAddress,int port){
    struct sockaddr_in addr;  // 存储服务器的地址信息
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(serAddress); // 绑定指定服务器地址
    addr.sin_port = htons(port); // 将端口号从主机字节序转换为网络字节序
    is_connect = false;
    // 这里直接用connect，因为客户端不需要绑定,而sock_connect只有在绑定之后才能使用

    std::cout<<"client socket id: "<<client_sock.get_sockid()<<std::endl;
    if(connect(client_sock.get_sockid(),(struct sockaddr *) &addr, sizeof(addr)) < 0)
        {
            std::cout << "connect fail" << std::endl;
            exit(EXIT_FAILURE);
        }
        printf("connected\n");
    is_connect = true;
}

pthread_t client::create_thread(void *(*thread_function)(void *))
{
    int result;
    pthread_t thread;
    long sock_id = client_sock.get_sockid();

    result = pthread_create(&thread,nullptr,thread_function,(void *)sock_id);
    if(result)
    {
        std::cout<<"client thread create failed\n";
        exit(-1);
    }
    return thread;
}

int main(int argc,char * argv[]) // argv[0]是文件名，argv[1]是第一个参数
{
    client myclient;
    int choose,res;
    int sock_id = myclient.get_clientsockid();
    pthread_t sender;
    pthread_t receiver;
    void * thread_result;
    while(1)
    {
        std::string server_ip;
        int server_port;
        std::cout<<"the client start:\n 0-> exit \n 1-> connect \n 2-> disconnect \n 3-> get Time \n 4-> get Name \n 5-> send Message to other client\n 6-> get all clients\n";
        std::cin>>choose;
        switch(choose)
        {
            case 0:
            {
                if(is_connect)
                    std::cout<<"Please disconnect first\n";
                else
                    exit(0);
            }    
            case 1:
            {
                if(is_connect)
                {
                    std::cout<<"Already connected\n";
                    continue;
                }

                std::cout << "Please enter server ip: ";
                std::cin >> server_ip;
                std::cout << "Please enter server port: ";
                std::cin >> server_port;
                myclient.c_connect(server_ip.c_str(),server_port);
                // m.type = 'c';
                sender = myclient.create_thread(thread_sender); // 每次receive之后会send
                receiver = myclient.create_thread(thread_receiver);

                res = sem_init(&bin_sem,0,0);
                if(res)
                {
                    std::cout<<"Semaphore initialization failed\n";
                    close(sock_id);
                    exit(-1);
                }
                break;
            }
            case 2:  //"Read head wrong"是getMessage的问题，之后肯定要修改 //disconnect
            {
                if(!is_connect)
                {
                    std::cout<<"Hasn't been connected yet\n";
                    continue;
                }
                   
                m.type = 'd';
                m.info[0] = 0;
                sem_post(&bin_sem);
                res = pthread_join(receiver,&thread_result);
                if(res)
                {
                    std::cout<<"Thread join failed\n";
                    exit(-1);
                }
                res = pthread_join(sender,&thread_result);
                if(res)
                {
                    std::cout<<"Thread join failed\n";
                    exit(-1);
                }
                sem_destroy(&bin_sem);
                close(sock_id);
                exit(0);
                break;
            }
            case 3:  // time 问题：不会从服务器端返回结果，返回之后无法跳出
            {
                if(!is_connect)
                {
                    std::cout<<"Hasn't been connected yet\n";
                    continue;
                }
                m.type = 't';
                m.info[0] = 0;
                sem_post(&bin_sem);
                break;
            }
            case 4: // name
            {
                if(!is_connect)
                {
                    std::cout<<"Hasn't been connected yet\n";
                    continue;
                }
                m.type = 'n';
                m.info[0] = 0;
                sem_post(&bin_sem);
                break;
            }
            case 5:  // send message   IP|message
            {
                if(!is_connect)
                {
                    std::cout<<"Hasn't been connected yet\n";
                    continue;
                }

                printf("enter the client id you want to send to:  ");
                int client_id;
                std::cin>>client_id;
                std::cout<<"Please enter message:\n";
                scanf("\n");
                fgets(m.info,MAXLEN,stdin);  // 先输入目标client id ，再输入des
                snprintf(m.info,MAXLEN,"%d%s",client_id,m.info);
                m.type = 's';
                sem_post(&bin_sem);
                break;
            }
            case 6:  // list
            {
                if(!is_connect)
                {
                    std::cout<<"Hasn't been connected yet\n";
                    continue;
                }
                m.type='l';
                m.info[0]=0;
                sem_post(&bin_sem);
            }
            default: // case 6变成wrong enter?
            {
                std::cout<<"Wrong enter, please enter again\n";
                break;
            }    
        }
    }
}
 
void *thread_receiver(void *arg) {
    int sockfd = (long)arg;

    while(1){
        message m1;
        // std::cout<<"hahaha"<<std::endl;

        if(getMessage(&m1,sockfd) == 0)   // 将sockfd对应的message放入m1
            exit(-1);
        if(m1.type=='d')
            pthread_exit(NULL);
        
    }
}

/* the thread to send message */
void *thread_sender(void *arg) {
    int sockfd = (long)arg;
    // int value=1;
    // sem_getvalue(&bin_sem,&value);
    // std::cout<<"out:"<<value<<std::endl;
    sem_wait(&bin_sem);
    while(1){
        // std::cout<<"hehehe"<<std::endl;
        // sem_getvalue(&bin_sem,&value);
        // std::cout<<"in: "<<value<<std::endl;
        if(m.type=='s')
        {
            send(sockfd, &m, 1,0);
        }
        else
            send(sockfd, &m, 1,0);  // 修改
        if(m.type=='d')
            pthread_exit(NULL);
        sem_wait(&bin_sem);  
    }
}
// 需要修改消息的格式，但是目前没啥头绪
int getMessage(message* m,int client_sockfd){
    /*  根据以下内容分类讨论完成信息：
        t time t|time
        n name n|name
        l list, l|ID|IP|port|ID|IP|port
        s send message successfully, s|message
        u send message unsuccessfully, u|error message
    */
    char buf[MAXLEN] = {'\0'};
    std::string inform;
    int bytegot;
    bytegot = read(client_sockfd, buf, sizeof(buf) - 1);
    // bytegot = recv(client_sockfd, buf, MAXLEN, 0);  // 返回长度
    if(bytegot <= 0 )
    {
        std::cout << "something wrong" << std::endl;  // disconnect 有问题
        exit(-1);
    }
    char client_type = buf[0] ;
    std::cout<<"client_type: "<<client_type<<std::endl;
    inform = buf;
    m->type = client_type;
    if(client_type == 't')  //  t time t|time  
    {
        std::cout << "time: " << inform.substr(1,bytegot-1) << std::endl;
    }
    else if(client_type == 'n')  // n name n|name
    {
        std::cout << "name: " << inform.substr(1,bytegot-1) << std::endl;
    }
    else if(client_type == 'l') // l list, l|ID|IP|port|ID|IP|port
    {
        std::cout << "list:" << inform.substr(1,bytegot-1) << std::endl;

    }
    else if(client_type == 's' || client_type == 'u')  // s|message
    {
        std::cout<<"send result: "<<inform.substr(1,bytegot-1)<<std::endl;
    }
    else if(client_type == 'd') // l list, l|ID|IP|port|ID|IP|port
    {
        std::cout << "Disconnect acknowledgement from server"<<std::endl;

    }
    else{
        std::cout << "message type error" << std::endl;
        return 0;
    }
    return 1;
}
