#include "conn.h"
#include <stdlib.h> // For malloc, free
#include <string.h> // For memcpy
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Create a new Conn instance
Conn *conn_create()
{
    Conn *conn = (Conn *)malloc(sizeof(Conn));
    if (!conn)
    {
        perror("Failed to allocate Conn");
        exit(EXIT_FAILURE);
    }

    conn->fd = -1;
    conn->want_read = false;
    conn->want_write = false;
    conn->want_close = false;
    conn->incoming = vector_create(sizeof(uint8_t)); // Initialize incoming buffer
    conn->outgoing = vector_create(sizeof(uint8_t)); // Initialize outgoing buffer

    return conn;
}

void fd_set_nb(int fd)
{
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) < 0)
    {
        perror("fcntl(F_SETFL, O_NONBLOCK)");
        exit(EXIT_FAILURE);
    }
}

// Accept a new connection
Conn *handle_accept(int fd)
{
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);

    // Accept the new connection
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0)
    {
        perror("accept");
        return NULL;
    }

    // Set the new connection's file descriptor to non-blocking mode
    fd_set_nb(connfd);

    // Create a new Conn instance
    Conn *conn = conn_create();
    conn->fd = connfd;
    conn->want_read = true; // Read the first request

    return conn;
}

// Free a Conn instance
void conn_free(Conn *conn)
{
    if (conn)
    {
        vector_free(conn->incoming); // Free incoming buffer
        vector_free(conn->outgoing); // Free outgoing buffer
        free(conn);
    }
}

// Add data to the incoming buffer
void conn_add_incoming(Conn *conn, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        vector_push_back(conn->incoming, &data[i]);
    }
}

// Add data to the outgoing buffer
void conn_add_outgoing(Conn *conn, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        vector_push_back(conn->outgoing, &data[i]);
    }
}

// Clear both incoming and outgoing buffers
void conn_clear_buffers(Conn *conn)
{
    vector_resize(conn->incoming, 0); // Clear incoming buffer
    vector_resize(conn->outgoing, 0); // Clear outgoing buffer
}