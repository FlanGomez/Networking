#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define PORT "8000"

void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void)
{
    int server_fd, client_fd;
    char message[1024];
    struct addrinfo req, *access;
    //======== addrinfo =======//
    // ai_flags   ==> 4byte
    // ai_family  ==> 4byte
    // ai_socktype ==> 4byte
    // ai_protocol ==> 4byte
    // ai_addrlen ==> 4byte
    // ai_addr   ==> 8byte  (an struct which has 8byte in it and is allocated through heap (SOCKADDR_IN))
    // ai_canonname ==> 8byte (for DNS naming)
    // ai_next  ==> 8byte
    // Padding  ==> 4byte
    //======== addrinfo =======//
    struct sockaddr_storage client_storage; // 128 byte storage
    socklen_t len = sizeof(client_storage);

    memset(&req, 0, sizeof(req));
    req.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    req.ai_socktype = SOCK_STREAM; // TCP CONNECTION
    req.ai_flags = AI_PASSIVE;     // LIKE INNADDR_ANY

    int epoll_fd = epoll_create1(0);
    struct epoll_event ev, ready_list[100];

    int status = getaddrinfo(0, PORT, &req, &access); // also keeps preferences over here autofilling will be done in the server() and bind()
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo : %s\n", gai_strerror(status)); // gai_strerror used to convert the number based error into human readable form
        return 1;
    }

    // socket(IP , TCP , PROTOCOL)
    server_fd = socket(access->ai_family, access->ai_socktype, access->ai_protocol);
    if (server_fd == -1)
    {
        perror("Socket Failed");
        close(server_fd);
    }

    // bind(server, sockaddr_in(16byte), sizeof(sockaddr_in))
    if (bind(server_fd, access->ai_addr, access->ai_addrlen) == -1)
    {
        perror("Bind Failed");
        close(server_fd);
    }

    freeaddrinfo(access); // Free the heap after initalizing socket and bind it

    if (listen(server_fd, 3) == -1)
    {
        perror("Listen Failed");
        close(server_fd);
    }

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);
    set_nonblocking(server_fd);

    while (1)
    {
        int ready_fds = epoll_wait(epoll_fd, ready_list, 100, -1);
        if (ready_fds > 0)
        {
            for (int each_ID = 0; each_ID < ready_fds; each_ID++)
            {
                if (ready_list[each_ID].data.fd == server_fd)
                {
                    client_fd = accept(server_fd, (struct sockaddr *)&client_storage, &len);
                    if (client_fd == -1)
                    {
                        printf("\nACCEPT FAILED");
                        continue;
                    }
                    struct epoll_event watch;
                    watch.events = EPOLLIN;
                    watch.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &watch);
                    set_nonblocking(client_fd);
                }

                else if (ready_list[each_ID].data.fd != server_fd)
                {
                    int bytes = recv(ready_list[each_ID].data.fd, message, sizeof(message), 0);
                    if (bytes > 0)
                    {
                        message[bytes] = '\0';
                        printf("Buffer : %s\n", message);
                    }

                    if (strncmp(message, "POST", 4) == 0)
                    {
                        printf("%s\n", message);

                        const char *clients_reply =
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 2\r\n"
                            "\r\n"
                            "OK";
                        send(ready_list[each_ID].data.fd, clients_reply, strlen(clients_reply), 0);
                    }
                    else
                    {
                        const char *body =
                            "<html><body>"
                            "<form method='POST' action='/login'>"
                            "<input type='text' name='username' placeholder='Username'><br>"
                            "<input type='password' name='password' placeholder='Password'><br>"
                            "<button type='submit'>Login</button>"
                            "</form>"
                            "</body></html>";

                        char response[1024];
                        snprintf(response, sizeof(response),
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/html\r\n"
                                "Content-Length: %zu\r\n"
                                "\r\n"
                                "%s",
                                strlen(body),
                                body);

                        send(ready_list[each_ID].data.fd, response, strlen(response), 0);
                    }
                }
            }
        }
    }
}