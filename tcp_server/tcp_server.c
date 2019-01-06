#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() { 
    char msg[255];

    int sock0; 
    int sock;

    struct sockaddr_in addr;
    struct sockaddr_in client;
    
    int len;
    int yes = 1;

    fputs("msg : ", stdout);
    fgets(msg, 255, stdin);

    sock0 = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;

    setsockopt(sock0, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    bind(sock0, (struct sockaddr *)&addr, sizeof(addr));

    listen(sock0, 5);

    while (1) {
        len = sizeof(client);
        sock = accept(sock0, (struct sockaddr *)&client, &len);
        write(sock, msg, strlen(msg));
        close(sock);
    }

    close(sock0);

    return 0;
}
