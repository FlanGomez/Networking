#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>


int main(void){

    int server,client;
    char message[1024];

    struct addrinfo req, *accessing_struct;
    // ============ addrinfo ============ //
    // addrinfo is a 48 byte in-build struct to hold 8 in-built variables
    // 3 variables are on heap and rest 6 are on stack and we use only 1 variable from the 3 variables which are on heap which is ai_addr(which is sockaddr_in)
    //
    // ------ 8 variables ------
    // ai_flags  => 4 byte
    // ai_socktype => 4byte
    // ai_protocol => 4byte
    // ai_family  => 4byte
    // ai_addrlen => 4byte
    // struct ai_addr( The Heap allocation) => 8byte
    // ai_canonname (Heap allocation) => 8byte
    // ai_next (Heap allocation) => 8byte
    // paddint  => 4 byte
    //
    // Total = 48 bytes
    //
    // ============ addrinfo ============ //

    struct sockaddr_storage client_storage;  // This hold 128 bytes of storage to hold any IPv4 or IPv6

    socklen_t memory_space_of_client_storage = sizeof(client_storage);

    memset(&req,0,sizeof(req)); // Set each variable in the struct as zero so example ai_flags = 0; because they are set to garbage values so we set them to zero
    req.ai_family = AF_UNSPEC;  // (saying : I want any IPv4 or IPv6) IP
    req.ai_socktype = SOCK_STREAM; // TCP or UDP
    req.ai_flags = AI_PASSIVE;    // Receives any address WIFI ETHERNET OR ANY ADDRESS


    // getaddrinfo is used to set the address and let the code be ready with all address and domains and ports
    int status = getaddrinfo(0,"8000",&req,&accessing_struct); // ( host   port   stack_variable  heap_variable)
    // dns is 0 cause this is a server
    // port is set to 8000
    // &req is the struct variable which has all the access to internal varibale of addrinfo and getaddrinfo automatically fills the all addresses by itself based on the IP SOCKTYPE n all etc
    //&accessing_struct is used to create a heap by internal addrinfo struct 's variable ai_addr which create a heap to store the auto filled address and let it be used further
    //ai_addr is sockaddr_in which is of interally 16 bytes which has IP, port, address of the server 127.0.0.1 and padding but in here we dont include padding so it is 8 byte

    // Q1. Why &accessing_struct used in below code ?
    // Because we need the sockaddr_in to do the further code which is just 16 byte packet which holds IP PORT ADDRESS
    
    if(status != 0){
        // if status any other number then 0 perform this
        fprintf(stderr,"getaddrinfo : %s\n",gai_strerror(status)); // gai_strerror converts numbers to human readable forms
        return 1;
    }

    // Socket(server , IP, TCP/USP , PROTOCOL)
    server = socket( accessing_struct -> ai_family, accessing_struct -> ai_socktype, accessing_struct -> ai_protocol);
    if(server == -1){
        printf("\nSocket Failed");
        close(server);
    }

    // Bind(server , sockaddr_in , sizeof(sockaddr_in)) or Bind(server , sockaddr , sizeof(sockaddr))
    if(bind(server,accessing_struct -> ai_addr,accessing_struct -> ai_addrlen) == -1){
        printf("\nBind Failed");
        close(server);
    }

    // Freed the heap after making the server and binding the addresses
    freeaddrinfo(accessing_struct);

    if(listen(server,3) == -1){
        printf("\nListen Failed");
    }

    // Client( server , sockaddr , &client_storage, sizeof(client_storage))
    client = accept(server,(struct sockaddr *)&client_storage,&memory_space_of_client_storage);
    if(client == -1){
        printf("\nAccept Failed");
        close(server);
    }

    while (1)
    {
        memset(&message,0,sizeof(message));
        printf("\nMessage : ");
        fgets(message,sizeof(message),stdin);
        write(client,message,strlen(message));

        read(client,message,sizeof(message));
        printf("\nClient : %s\n",message);
    }

    close(server);

}