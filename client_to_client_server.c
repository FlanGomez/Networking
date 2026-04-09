#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <pthread.h>


int group[5];
int client_count = 0;
pthread_t thread;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *handle_clients(void* ID){      // Created a void function and it should have void parameter
    int ClientID = *(int *)ID;       // ID was void datatype and it didnt had any datatype so here it is assigned to an Int datatype
    free(ID);                        // free the space for next ID
    char message[1024];
    while (1)
    {

        int check = read(ClientID,message,sizeof(message));
        if(check <= 0){
            printf("Client Disconnected : %d\n",ClientID);
            close(ClientID);
            break;
        }
            printf("\nClient : %s\n",message);
        
        
        // Here server just write what the client will type using for loop to get the data what the client types for they all are in the group[] and its accessed by the for loop
        for(int each_ID = 0;each_ID < 5;each_ID++){
            pthread_mutex_lock(&lock); // lock the code and dont do anything else
            if(group[each_ID]!=0 && group[each_ID]!= ClientID) // if each_ID of client not equal to zero and it doesnt write the message for himeself pass the code further
            {
                write(group[each_ID],message,strlen(message));
            }
            pthread_mutex_unlock(&lock); // unlock the code and move forward
        }

    }

    return NULL;
}


int main(void){
    int server,client;
    pthread_t thread;

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
    struct sockaddr_storage client_storage; // 128 byte storage
    socklen_t len = sizeof(client_storage);

    memset(&req,0,sizeof(req));
    req.ai_family = AF_UNSPEC;    // IPv4 or IPv6
    req.ai_socktype = SOCK_STREAM; // TCP CONNECTION
    req.ai_flags = AI_PASSIVE;    // LIKE INNADDR_ANY

    int status = getaddrinfo(0,"8000",&req,&access); // also keeps preferences over here autofilling will be done in the server() and bind()
    if(status !=0){
        fprintf(stderr,"getaddrinfo : %s\n",gai_strerror(status)); // gai_strerror used to convert the number based error into human readable form
        return 1;
    }

    // socket(IP , TCP , PROTOCOL)
    server = socket(access -> ai_family, access -> ai_socktype, access -> ai_protocol);
    if(server == -1){
        perror("Socket Failed");
        close(server);
    }

    // bind(server, sockaddr_in(16byte), sizeof(sockaddr_in))
    if(bind(server,access -> ai_addr,access -> ai_addrlen) == -1){
        perror("Bind Failed");
        close(server);
    }

    freeaddrinfo(access);  // Free the heap after initalizing socket and bind it

    if(listen(server,3) == -1){
        perror("Listen Failed");
        close(server);
    }

    while (1)
    {
        // client is a integer where number for each client is assigned
        client = accept(server,(struct sockaddr *)&client_storage,&len);
        if(client == -1){
            perror("Accept Failed");
            close(server);
        }
        printf("\nCliend Id : %d\n",client);

        // Lock n stop the code
            pthread_mutex_lock(&lock);
            group[client_count] = client;// pushed the integer client ID into the group
            client_count++;             // incremented the count
            pthread_mutex_unlock(&lock);// unlock the code and move ahead with the process

            int *ptr_client = malloc(sizeof(int)); // pointer variable create a 4 byte space for each client in the heap
            *ptr_client = client;   // the 4 byte memory space is used by client which is 4byte memory space is filled with clientID 5

            pthread_create(&thread,NULL,handle_clients,ptr_client);  // Created a pthread process where (thread , defualt settings , func ,args) are passed to create the pthread
            pthread_detach(thread);// pthread_detach is freeed after creating to create a new one for the next client

    }
}