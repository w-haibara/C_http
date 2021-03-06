#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

int main(void) {
    int mysocket;
    struct sockaddr_in server;
    struct addrinfo hints, *res;

    SSL *ssl;
    SSL_CTX *ctx;

    char msg[100];

    char *host = "developer.mozilla.org";
    char *path = "/ja/docs/Web/HTTP/Headers/Connection";
    int   port = 443;

    // IPアドレスの解決
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    char *service = "https";

    int err = 0;
    if ((err = getaddrinfo(host, service, &hints, &res)) != 0) {
        fprintf(stderr, "Fail to resolve ip address - %d\n", err);
        return EXIT_FAILURE;
    }

    if ((mysocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        fprintf(stderr, "Fail to create a socket.\n");
        return EXIT_FAILURE;
    }

    if (connect(mysocket, res->ai_addr, res->ai_addrlen) != 0) {
        printf("Connection error.\n");
        return EXIT_FAILURE;
    }

    SSL_load_error_strings();
    SSL_library_init();

    ctx = SSL_CTX_new(SSLv23_client_method());
    ssl = SSL_new(ctx);
    err = SSL_set_fd(ssl, mysocket);
    SSL_connect(ssl);

    printf("Conntect to %s\n", host);

    sprintf(msg, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);

    SSL_write(ssl, msg, strlen(msg));

    int buf_size = 256;
    char buf[buf_size];
    int read_size;

    do {
        read_size = SSL_read(ssl, buf, buf_size);
        write(1, buf, read_size);
    } while(read_size > 0);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    ERR_free_strings();

    close(mysocket);

    return EXIT_SUCCESS;
}
