#include "conn.h"
#include <stdlib.h> // For malloc, free
#include <string.h> // For memcpy

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