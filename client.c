#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

int main(void){

    int client_server;
    char message[1024];
    struct addrinfo req,*access_struct;
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

    memset(&req,0,sizeof(req));
    req.ai_family = AF_UNSPEC;  // IPv4 or IPv6
    req.ai_socktype = SOCK_STREAM; // TCP

    int status = getaddrinfo("127.0.0.1", "8000", &req, &access_struct);
    // here getaddrinfo is set a domain of the server 127.0.0.1 and port 8000 and the struct variable req and ai_addr heap pointer variable access_struct
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status)); // gai_strerror converts numbers in human readable codes
        return 1;
    }

    // Socket(server , IP , SOCKTYPE , PROTOCOL)
    client_server = socket(access_struct -> ai_family,access_struct -> ai_socktype, access_struct ->ai_protocol);
    if(client_server == -1){
        printf("\nServer Failed");
        close(client_server);
    }

    // Connect( server , sockaddr_in , sizeof(sockaddr_in)) or ( server , sockaddr , sizeof(sockaddr))
    if(connect(client_server,access_struct -> ai_addr, access_struct -> ai_addrlen) == -1){
        printf("Connect Failed");
        close(client_server);
    }

    freeaddrinfo(access_struct); // Freed the heap

    while (1)
    {

        read(client_server,message,sizeof(message));
        printf("\nClient : %s\n",message);

        memset(&message,0,sizeof(message));
        printf("\nMessage : ");
        fgets(message,sizeof(message),stdin);
        write(client_server,message,strlen(message));

    }

}