#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

/*
 * opensslライブラリのインクルード
 */
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUF_SIZE 256

#define REQ_SIZE \
    strlen(req.get) + \
    strlen(req.acpt) + \
    strlen(req.acpt_en) + \
    strlen(req.acpt_la) + \
    strlen(req.cache) + \
    strlen(req.con) + \
    strlen(req.host) + \
    strlen(req.pragma) + \
    strlen(req.up) + \
    22 \

char* make_cmd(char *arg, char* ipadress, char* pass){
    struct httpcmd {
        char get[1024];
        char acpt[1024];
        char acpt_en[1024];
        char acpt_la[1024];
        char cache[1024];
        char con[1024];
        char host[1024];
        char pragma[1024];
        char up[1024]; // Upgrade-Insecure-Requests 
    };

    struct httpcmd req;

    memset(req.get, 0, sizeof(req.get)); 
    memset(req.acpt, 0, sizeof(req.acpt));
    memset(req.acpt_en, 0, sizeof(req.acpt_en));
    memset(req.acpt_la, 0, sizeof(req.acpt_la));
    memset(req.cache, 0, sizeof(req.cache));
    memset(req.con, 0, sizeof(req.con));
    memset(req.host, 0, sizeof(req.host));
    memset(req.pragma, 0, sizeof(req.pragma));
    memset(req.up, 0, sizeof(req.up));

    strcpy(req.get, "GET "); 
    strcpy(req.acpt, "Accpet: ");
    strcpy(req.acpt_en, "Accept-Encoding: ");
    strcpy(req.acpt_la, "Accept-Language: ");
    strcpy(req.cache, "Cache-Control: ");
    strcpy(req.con, "Connection: ");
    strcpy(req.host, "Host: ");
    strcpy(req.pragma, "Pragma: ");
    strcpy(req.up, "Upgrade-Insecure-Requests: ");

    if(memcmp(arg, "http://", 7) == 0) {
        printf("\nThis client don't adapt to HTTP\n");
        abort();
    }

    size_t uriLen = strlen(arg) + 1;

    if(memcmp(arg, "https://", 8) == 0) {
        memmove(arg, (arg + 8), (uriLen - 8));
        uriLen -= 8;
    }

    if(uriLen > 255) {
        printf("The length of URI is too long\n");
        abort();
        return " ";
    }

    char uri[uriLen];
    memset(uri, 0, sizeof(uri));
    strcpy(uri, arg);

    char *sla = strchr(uri, '/');

    if(sla != NULL) {
        size_t ipadrLen =  (int)(sla - uri); 
        memmove(ipadress, uri, ipadrLen);
        ipadress[ipadrLen] = '\0';

        strcpy(pass, sla); 
    } else {
        memmove(ipadress, uri, strlen(uri));
        pass[0] = '/';
        pass[1] = '\0';
    }

    strcat(req.get, pass); 
    strcat(req.acpt, "*/*");
    strcat(req.acpt_en, "gzip, deflate, sdch, br");
    strcat(req.acpt_la, "en-US;q=0.8,en;q=0.6");
    strcat(req.cache, "no-cache");
    strcat(req.con, "close");
    strcat(req.host, ipadress);
    strcat(req.pragma, "no-cache");
    strcat(req.up, "0");

    strcat(req.get, " HTTP/1.1");

    size_t bufSize = sizeof(char) * REQ_SIZE;
    char *buf = malloc(bufSize);
    memset(buf, 0, bufSize);
    snprintf(buf, REQ_SIZE, 
            "%s\r\n"
            "%s\r\n"
            "%s\r\n"
            "%s\r\n"
            "%s\r\n"
            "%s\r\n"
            "%s\r\n"
            "%s\r\n"
            "%s\r\n\r\n" 
            , req.get
            , req.acpt
            , req.acpt_en
            , req.acpt_la
            , req.cache
            , req.con
            , req.host
            , req.pragma
            , req.up   
            );

    return buf;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server;
    int sock;
    int port = 443;
    char* cmd;
    char buf[BUF_SIZE];
    unsigned int **addrptr;
    char ipadress[256];
    char pass[256];

    SSL *ssl;
    SSL_CTX *ctx;

    if (argc != 2) {
        printf("Specify the destination\n");
        return 1;
    }

    cmd = make_cmd(argv[1], ipadress, pass);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ipadress);

    if (server.sin_addr.s_addr == 0xffffffff) {
        struct hostent *host;

        host = gethostbyname(ipadress);
        if (host == NULL) {
            printf("%s : %s\n", hstrerror(h_errno), ipadress);
            return 1;
        }

        addrptr = (unsigned int **)host->h_addr_list;

        while (*addrptr != NULL) {
            server.sin_addr.s_addr = *(*addrptr);

            if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0) {
                break;
            }

            addrptr++;
        }

        if (*addrptr == NULL) {
            perror("connect");
            return 1;
        }
    } else {
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) != 0) {
            perror("connect");
            return 1;
        }
    }

    /*
     * opensslの設定
     */
    SSL_load_error_strings();
    SSL_library_init();

    ctx = SSL_CTX_new(SSLv23_method());
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    SSL_connect(ssl);

    int n = SSL_write(ssl, cmd, (int)strlen(cmd));
    printf("%s", cmd);

    if (n <= 0) {
        perror("write");
        return 1;
    }

    //サーバからのHTTPメッセージ受信
    int sslret;
    do {
        ERR_clear_error();
        memset(buf, 0, sizeof(buf));
        sslret = SSL_read(ssl, buf, BUF_SIZE);

        if(sslret > 0){
            write(1, buf, sslret);
        }
    } while (sslret > 0); 

    close(sock);

    SSL_shutdown(ssl);  // SSL/TLS接続をシャットダウンする。
    SSL_free(ssl);

    SSL_CTX_free(ctx);
    ERR_free_strings();

    free(cmd);

    printf("\n\nhttps_client quit\n");
    return 0;
}
