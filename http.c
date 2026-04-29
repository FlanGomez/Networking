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

#define PORT "8080"
#define SIZE_OF_CLIENTS 999
#define DNS NULL
#define BUFFER 4096
#define LOGIN_HTML                                                       \
    "<html><body>"                                                       \
    "<form method='POST' action='/login'>"                               \
    "<input type='text' name='username' placeholder='Username'><br>"     \
    "<input type='password' name='password' placeholder='Password'><br>" \
    "<button type='submit'>Login</button>"                               \
    "</form></body></html>"

// ==== NON BLOCKING FUNC ==== //
void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main()
{
    int sock_fd, client_fd, epoll_fd, status;
    int bind_check, ready_fds;
    char message[BUFFER];

    struct addrinfo req, *access;
    //======= ADDRINFO ====== //
    // ai_flags
    // ai_protocol
    // ai_socktype
    // ai_family
    // ai_addrlen
    // ai_next
    // ai_addr
    // ai_canonname
    //=========================

    memset(&req, 0, sizeof(req));
    req.ai_family = AF_UNSPEC;
    req.ai_protocol = 0;
    req.ai_socktype = SOCK_STREAM;
    req.ai_flags = AI_PASSIVE;

    struct sockaddr_storage client_storage;
    socklen_t len = sizeof(client_storage);

    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1){
        perror("EPOLL INSTANCE FAILED");
        return 1;
    }
    struct epoll_event ev, ready_fd_list[SIZE_OF_CLIENTS];

    status = getaddrinfo(DNS, PORT, &req, &access);
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo : %s\n", gai_strerror(status)); // gai_strerror used to convert the number based error into human readable form
        return 1;
    }

    sock_fd = socket(access->ai_family,access->ai_socktype,access->ai_protocol);
    if (sock_fd == -1)
    {
        perror("Socket Failed");
        return 1;
    }

    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bind_check = bind(sock_fd, access->ai_addr, access->ai_addrlen);
    if (bind_check == -1)
    {
        perror("Bind Failed");
        return 1;
    }
    freeaddrinfo(access);

    if (listen(sock_fd, 10) == -1)
    {
        perror("Listen Failed");
        return 1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = sock_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev);
    set_nonblocking(sock_fd);

    while (1)
    {
        ready_fds = epoll_wait(epoll_fd, ready_fd_list, SIZE_OF_CLIENTS, -1);

        if (ready_fds > 0)
        {
            for (int each_fd = 0; each_fd < ready_fds; each_fd++)
            {
                if (ready_fd_list[each_fd].data.fd == sock_fd)
                {
                    client_fd = accept(sock_fd, (struct sockaddr *)&client_storage, &len);
                    if (client_fd == -1)
                    {
                        perror("Client Fd Invalid");
                        continue;
                    }
                    set_nonblocking(client_fd);

                    struct epoll_event ev2;
                    ev2.events = EPOLLIN;
                    ev2.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev2);
                }
                else
                {
                    int bytes = recv(ready_fd_list[each_fd].data.fd, message, sizeof(message) - 1, 0);

                    if (bytes <= 0) {  // 0 = disconnect, -1 = error
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ready_fd_list[each_fd].data.fd, NULL);
                        close(ready_fd_list[each_fd].data.fd);
                        continue;

                    if (bytes > 0)
                    {
                        message[bytes] = '\0';

                        if (strncmp(message, "POST", 4) == 0)
                        {
                            const char *reply = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 2\r\n\r\nOK";
                            send(ready_fd_list[each_fd].data.fd, reply, strlen(reply), 0);
                        }
                        else
                        {
                            char response[4096];
                            snprintf(response, sizeof(response),
                                    "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/html\r\n"
                                    "Content-Length: %zu\r\n\r\n%s",
                                    strlen(LOGIN_HTML), LOGIN_HTML);
                            send(ready_fd_list[each_fd].data.fd, response, strlen(response), 0);
                        }
                    }
                }
            }
        }
    }
}
}