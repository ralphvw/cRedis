#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

void die(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static void do_something(int connfd)
{
    // Read buffer
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);

    if (n < 0)
    {
        printf("Message read error");
        return;
    }

    printf("Client says: %s", rbuf);

    // Write buffer
    char wbuf[] = "World";
    write(connfd, wbuf, strlen(wbuf));
}

int main()
{
    // Server setup
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);

    int rv = bind(fd, (struct sockaddr *)&addr, sizeof(addr));

    if (rv)
    {
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);

    if (rv)
    {
        die("listen()");
    }

    while (1)
    {
        // Accept client connections
        struct sockaddr_in client_addr = {};
        socklen_t addr_len = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&addr, &addr_len);

        if (connfd < 0)
        {
            continue;
        }

        do_something(connfd);
        close(connfd);
    }
}