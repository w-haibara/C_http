#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

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

    if(memcmp(arg, "https://", 8) == 0) {
        printf("\nThis client don't adapt to HTTPS\n");
        abort();
    }

    size_t uriLen = strlen(arg) + 1;

    if(memcmp(arg, "http://", 7) == 0) {
        memmove(arg, (arg + 7), (uriLen - 7));
        uriLen -= 7;        
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
    strcat(req.up, "1");

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
    char* cmd;
    char buf[256];
    unsigned int **addrptr;
    char ipadress[256];
    char pass[256];

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
    server.sin_port = htons(80); // HTTPなのでポートは80番
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

    int n = write(sock, cmd, (int)strlen(cmd));
    printf("%s", cmd);
    if (n < 0) {
        perror("write");
        return 1;
    }

    //サーバからのHTTPメッセージ受信
    while (n > 0) {
        memset(buf, 0, sizeof(buf));
        n = read(sock, buf, sizeof(buf));
        if (n < 0) {
            perror("read");
            return 1;
        }

        //受信結果を標準出力へ表示
        write(1, buf, n);
    }

    close(sock);

    free(cmd);
  
    printf("\n\nhttp_client quit\n");

    return 0;
}
