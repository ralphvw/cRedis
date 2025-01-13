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

#define K_MAX_MSG 1024

void die(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

void handle_write(Conn *conn)
{
    assert(vector_size(conn->outgoing) > 0);

    ssize_t rv = write(conn->fd, conn->outgoing->data, vector_size(conn->outgoing));
    if (rv < 0)
    {
        conn->want_close = true;
        return;
    }

    // Remove written data from `outgoing`
    buf_consume(conn->outgoing, (size_t)rv);

    if (vector_size(conn->outgoing) == 0)
    {
        conn->want_read = true;
        conn->want_write = false;
    }
}

void handle_read(Conn *conn)
{
    uint8_t buf[64 * 1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));
    if (rv <= 0)
    {
        conn->want_close = true;
        return;
    }

    buf_append(conn->incoming, buf, (size_t)rv);

    while (try_one_request(conn))
    {
        // Keep processing as long as there's a complete message
    }

    if (vector_size(conn->outgoing) > 0)
    {
        conn->want_read = false;
        conn->want_write = true;
    }
    else
    {
        conn->want_read = true;
        conn->want_write = false;
    }
}

static void fd_set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

bool try_one_request(Conn *conn)
{
    // Ensure there is enough data for the header
    if (vector_size(conn->incoming) < 4)
    {
        return false;
    }

    uint32_t len = 0;
    memcpy(&len, conn->incoming->data, 4);
    if (len > K_MAX_MSG)
    {
        conn->want_close = true;
        return false;
    }

    if (4 + len > vector_size(conn->incoming))
    {
        return false;
    }

    const uint8_t *request = (const uint8_t *)conn->incoming->data + 4;

    buf_append(conn->outgoing, (const uint8_t *)&len, 4); // Add the header
    buf_append(conn->outgoing, request, len);

    buf_consume(conn->incoming, 4 + len);

    return true;
}

void handle_connection_sockets(Vector *poll_args, Vector *fd2conn)
{
    for (size_t i = 1; i < vector_size(poll_args); ++i)
    {
        struct pollfd *pfd = (struct pollfd *)vector_get(poll_args, i);
        uint32_t ready = pfd->revents;

        Conn **conn_ptr = (Conn **)vector_get(fd2conn, pfd->fd);
        Conn *conn = *conn_ptr;

        if (!conn)
        {
            continue;
        }

        if (ready & POLLIN)
        {
            handle_read(conn);
        }

        if (ready & POLLOUT)
        {
            handle_write(conn);
        }

        if ((ready && POLL_ERR) || conn->want_close)
        {
            close(conn->fd);

            conn_free(conn);
            *conn_ptr = NULL;
        }
    }
}

int main()
{
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

        int rv = poll((struct pollfd *)poll_args->data, (nfds_t)vector_size(poll_args), -1);
        if (rv < 0 && errno == EINTR)
        {
            continue;
        }
        if (rv < 0)
        {
            die("poll()");
        }

        if (((struct pollfd *)poll_args->data)[0].revents)
        {
            Conn *conn = handle_accept(fd);
            if (conn)
            {
                if (vector_size(fd2conn) <= (size_t)conn->fd)
                {
                    vector_resize(fd2conn, conn->fd + 1);
                }

                Conn **slot = (Conn **)vector_get(fd2conn, conn->fd);
                *slot = conn;
            }
        }

        handle_connection_sockets(poll_args, fd2conn);
    }
}
