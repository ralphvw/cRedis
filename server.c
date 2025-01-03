#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

void die(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

const size_t k_max_msg = 4096;

static int32_t one_request(int connfd)
{
    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    int errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err)
    {
        if (errno == 0)
        {
            msg("EOF");
        }
        else
        {
            msg("read() error");
        }
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    if (len > k_max_msg)
    {
        msg("too long");
        return -1;
    }
    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err)
    {
        msg("read() error");
        return err;
    }

    rbuf[4 + len] = '\0';
    printf("client says: %s\n", &rbuf[4]);

    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

static int32_t read_full(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0)
        {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }

    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0)
        {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
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

        while (1)
        {
            int32_t err = one_request(connfd);
            if (err)
            {
                break;
            }
        }

        do_something(connfd);
        close(connfd);
    }
}
