#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <poll.h>
#include "conn.h"
#include "vector.h"

void die(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static void fd_set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
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

    Vector *fd2conn = vector_create(sizeof(Conn *));
    Vector *poll_args = vector_create(sizeof(struct pollfd));

    while (1)
    {
        vector_resize(poll_args, 0);
        struct pollfd listening_pfd = {fd, POLLIN, 0};
        vector_push_back(poll_args, &listening_pfd);

        for (size_t i = 0; i < vector_size(fd2conn); i++)
        {
            Conn **conn_ptr = (Conn **)vector_get(fd2conn, i);
            Conn *conn = *conn_ptr;

            if (!conn)
            {
                continue;
            }

            struct pollfd pfd = {conn->fd, POLLERR, 0};

            // Add POLLIN or POLLOUT based on the connection's intent
            if (conn->want_read)
            {
                pfd.events |= POLLIN;
            }
            if (conn->want_write)
            {
                pfd.events |= POLLOUT;
            }

            vector_push_back(poll_args, &pfd);
        }
    }
}
