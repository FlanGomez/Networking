#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <pthread.h>



int main(void){
    int client_server;
    pthread_t thread;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    char message[1024];

    struct addrinfo req,*access;
    //======== addrinfo =======//
    //ai_flags   ==> 4byte
    //ai_family  ==> 4byte
    //ai_socktype ==> 4byte
    //ai_protocol ==> 4byte
    //ai_addrlen ==> 4byte
    //ai_addr   ==> 8byte  (an struct which has 8byte in it and is allocated through heap (SOCKADDR_IN))
    //ai_canonname ==> 8byte (for DNS naming)
    //ai_next  ==> 8byte
    // Padding  ==> 4byte
    //======== addrinfo =======//

    memset(&req,0,sizeof(req));
    req.ai_family = AF_UNSPEC;
    req.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo("127.0.0.1","8000",&req,&access); // also keeps preferences over here autofilling will be done in the server() and bind()
    if(status !=0){
        fprintf(stderr,"getaddrinfo : %s\n",gai_strerror(status));
        return 1;
    }

    client_server = socket(access -> ai_family,access -> ai_socktype, access -> ai_protocol);
    if(client_server == -1){
        perror("Server Failed");
        close(client_server);
    }

    if(connect(client_server,access -> ai_addr, access -> ai_addrlen) == -1){
        perror("Connect Failed");
        close(client_server);
    }

    freeaddrinfo(access);

    while (1)
    {

        memset(message, 0, sizeof(message));
        read(client_server,message,sizeof(message));
        printf("\nClient : %s\n",message);

        memset(message,0,sizeof(message));
        printf("\nMessage : ");
        fgets(message,sizeof(message),stdin);
        write(client_server,message,strlen(message));


    }
    

}