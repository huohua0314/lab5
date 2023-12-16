#include "client.h"

// client需要用户输入IP地址和port
client::client(char *serAddress,int port) : client_sock{}{
    struct sockaddr_in addr;  // 存储服务器的地址信息
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(serAddress); // 绑定指定服务器地址
    addr.sin_port = htons(port); // 将端口号从主机字节序转换为网络字节序
    is_connect = false;
    client_sock.sock_connect(client_sock.get_sockid(),(struct sockaddr *) &addr, sizeof(addr));
    is_connect = true;
}

pthread_t client::create_thread(void *(*thread_function)(void *))
{
    int result;
    pthread_t thread;

    result = pthread_create(&thread,nullptr,thread_function,(void *)client_sock.get_sockid());
    if(result)
    {
        std::cout<<"client thread create failed\n";
        exit(-1);
    }
    return thread;
}

int main(int argc,char * argv[]) // argv[0]是文件名，argv[1]是第一个参数
{
    if(argc!=3) 
    {
        std::cout<<"Please enter an IP address and a port"<<std::endl;
        exit(0);
    }
    
    int choose,res;
    int sock_id;
    pthread_t sender;
    pthread_t receiver;
    void * thread_result;
    while(1)
    {
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
                m.type = 'c';
                client myclient(argv[1],(int)argv[2]); // 只能初始化一次，怎么办？
                sock_id = myclient.get_clientsockid();
                receiver = myclient.create_thread(thread_receiver);
                sender = myclient.create_thread(thread_sender);
                res = sem_init(&bin_sem,0,0);
                if(res)
                {
                    std::cout<<"Semaphore initialization failed\n";
                    exit(-1);
                }
                break;
            }
            case 2:
            {
                if(!is_connect)
                    continue;
                m.type = 'd';
                m.info[0] = 0;
                sem_post(&bin_sem);
                res = pthread_join(receiver,&thread_result);
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
            case 3:  // time
            {
                if(!is_connect)
                    continue;
                m.type = 't';
                m.info[0] = 0;
                sem_post(&bin_sem);
                break;
            }
            case 4: // name
            {
                if(!is_connect)
                    continue;
                m.type = 'n';
                m.info[0] = 0;
                sem_post(&bin_sem);
                break;
            }
            case 5:  // send message   IP|message
            {
                if(!is_connect)
                    continue;

                printf("enter the client id you want to send to:  ");
                int client_id;
                std::cin>>client_id;
                std::cout<<"Please enter message:\n";
                scanf("\n");
                fgets(m.info,MAXLEN,stdin);  // 县输入目标client id ，再输入des
                snprintf(m.info,MAXLEN,"%d%s",client_id,m.info);
                m.type = 's';
                sem_post(&bin_sem);
                break;
            }
            case 6:
            {
                if(!is_connect)
                    continue;
                m.type='l';
                m.info[0]=0;
                sem_post(&bin_sem);
            }
            default:
            {
                std::cout<<"Wrong enter, please enter again\n";
                break;
            }    
        }
    }
}
 
void *thread_receiver(void *arg) {
    int sockfd = (int)arg;

    while(1){
        message m1;
        if(!getMessage(&m1,sockfd))
            exit(-1);
        if(m1.type=='d')
            pthread_exit(NULL);
        
    }
}

/* the thread to send message */
void *thread_sender(void *arg) {
    int sockfd = (int)arg;

    sem_wait(&bin_sem);
    while(1){
        send(sockfd, &m, 1 + strlen(m.info),0);  // 这里消息也太长了
        if(m.type=='d')
            pthread_exit(NULL);
        sem_wait(&bin_sem);
    }
}

int getMessage(message* m,int client_sockfd){
    int n=rio_readn(client_sockfd, m, sizeof(int)*3);
    if(n!=sizeof(int)*3){
        close(client_sockfd);
        std::cout<<"Read head wrong\n";
        return 0;
    }

    n += rio_readn(client_sockfd, m->info ,  sizeof(m->info)-sizeof(int)*3);

    if(n >= MAXLEN){
        close(client_sockfd);
        perror("Read content wrong\n");
        return 0;
    }
    m->info[n-sizeof(int)*3]=0;

    std::cout<<"receive message:\n";
    std::cout<<m->info<<std::endl;
    return 1;
}


ssize_t rio_readn(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = (char*)usrbuf;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* Interrupted by sig handler return */
		nread = 0;      /* and call read() again */
	    else
		return -1;      /* errno set by read() */ 
	} 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - Robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = (char*)usrbuf;

    while (nleft > 0) {
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR)  /* Interrupted by sig handler return */
		nwritten = 0;    /* and call write() again */
	    else
		return -1;       /* errno set by write() */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}