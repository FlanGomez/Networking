
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>


#define PORT "8080"
#define MAX_QUEUE 100
#define MAX_TRY 20
#define DNS NULL

int client_group[100];
int client_count = 0;

void non_blocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}


int main(void){

    //===== Variables ====//
    int server_fd, client_fd;
    int status = 0, epoll_fd, ready_fds;
    char buffer[1024] = {0};

    struct sockaddr_storage client_storage;
    socklen_t len = sizeof(client_storage);
    struct addrinfo req,*access;
    //======== addrinfo ========//
    // ai_flags  => 4bytes
    // ai_family => 4bytes
    // ai_protocol => 4bytes
    // ai_socktype => 4bytes
    // ai_addrlen => 4byte
    // ai_addr  ==> 8byte
    // ai_canonname ==> 8byte
    // ai_next  ==> 8byte
    //==========================//

    memset(&req,0,sizeof(req));  // Set the addrinfo struct members values to zero and later assigned them
    req.ai_flags = AI_PASSIVE;
    req.ai_family = AF_UNSPEC;
    req.ai_protocol = 0;
    req.ai_socktype = SOCK_STREAM;

    
    //====== Explaintion =====//
    // This code converts the human readable lines to binary assigning post host and the whole addrinfo and also req data transfer to access
    
    for(int try = 0;try<MAX_TRY;try++){
        status = getaddrinfo(DNS,PORT,&req,&access);
        if(status == 0){
            printf("\nSTATUS CODE : 200");
            break;
        }

        else if(status == EAI_AGAIN){
            printf("\nTRYING AGAIN....");
            sleep(2);
            continue;
        }

        else if(status != 0){
            fprintf(stderr,"\ngetaddrinfo :%s\n",gai_strerror(status));
            return 1;
        }
    }

    
    server_fd = socket(access -> ai_family,access -> ai_socktype,access -> ai_protocol);
    if(server_fd == -1){
        perror("\nSocket Failed");
        return 1;
    }

    int enable = 1;
    if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(enable)) == -1){
        printf("\nSETSOCKOPT FAILED");
        //exit(EXIT_FAILURE);
        return 1;
    }
    

    if(bind(server_fd,access -> ai_addr,access -> ai_addrlen) == -1){
        if(errno == EADDRINUSE){
            printf("\nPORT IN USE....");
            return 1;
        }
        else if(errno == EBADF){
            printf("\nFD ERROR");
            return 1;
        }
        else{
            perror("\nBind Failed");
            return 1;
        }
    }
    freeaddrinfo(access);

    if(listen(server_fd,MAX_QUEUE) == -1){
        perror("\nListen Failed");
    }

    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
    {
        perror("\nEPOLL INSTANCE FAILED");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev , ready_list[100];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&ev) == -1)
    {
        perror("\nEPOLL EVENT FAILED");
        exit(EXIT_FAILURE);
    }
    non_blocking(server_fd);


    while(1)
    {
        ready_fds = epoll_wait(epoll_fd,ready_list,100,-1);
        if(ready_fds == -1){
            perror("\nEPOLL WAIT FAILED");
            exit(EXIT_FAILURE);
        }


        if(ready_fds > 0){
            for(int each_ID = 0;each_ID < ready_fds; each_ID++){
                int check_serverFD = ready_list[each_ID].data.fd;
                if(check_serverFD == server_fd){
                    client_fd = accept(server_fd,(struct sockaddr *)&client_storage,&len);
                    if(client_fd == -1)
                {
                    perror("\nACCEPT FAILED");
                    exit(EXIT_FAILURE);
                }
                
                struct epoll_event watch;
                watch.events = EPOLLIN;
                watch.data.fd = client_fd;
                epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&watch);
                non_blocking(client_fd);

                client_group[client_count] = client_fd;
                printf("\nCLIENT ADDED");
                client_count++;
            }

                else{
                    memset(buffer,0,sizeof(buffer));
                    int bytes = read(check_serverFD,buffer,sizeof(buffer));
                    if(bytes > 0){
                        for(int check_ID = 0;check_ID < client_count; check_ID++){
                            if(client_group[check_ID] != check_serverFD){
                                write(client_group[check_ID],buffer,bytes);
                            }
                        }
                        printf("\nClient : %s\n",buffer);
                    }
                }
            }
        }
    }
}






