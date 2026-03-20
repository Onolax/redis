#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const int k_max_msg = 4096;

// Helper to print error and exit
static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = recv(fd, buf, n, 0);
        if (rv <= 0) {
            return -1;  // Error or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // Error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t query(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // Length prefix
    memcpy(&wbuf[4], text, len);
    if (write_all(fd, wbuf, 4 + len)) {
        return -1;
    }

    char rbuf[4 + k_max_msg];
    // Read the 4-byte header
    if (read_full(fd, rbuf, 4)) {
        return -1;
    }

    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) {
        fprintf(stderr, "too long\n");
        return -1;
    }

    // Read the body
    if (read_full(fd, &rbuf[4], len)) {
        return -1;
    }

    printf("server says: %.*s\n", len, &rbuf[4]);
    return 0;
}

int main() {
    // 1. Create the socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // 2. Configure the server address (localhost:1234)
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(9008);                    // Port number
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1

    // 3. Connect to the server
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0) {
        die("connect()");
    }

    // 4. Send multiple requests as per your snippet
    int32_t err = query(fd, "hello1");
    if (err) {
        goto L_DONE;
    }

    err = query(fd, "hello2");
    if (err) {
        goto L_DONE;
    }

    err = query(fd, "hello3");
    if (err) {
        goto L_DONE;
    }

L_DONE:
    close(fd);
    return 0;
}
