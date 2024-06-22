#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "parse-http.c"


#define HOST "127.0.0.1"
#define PORT 8080
#define MAX_RECV 1024


int main() {

    int soc = socket(AF_INET, SOCK_STREAM, 0);

    if (soc == -1) {
        printf("Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // reuse address
    int opt = 1;
    if (setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) == -1) {
        printf("Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // bind
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    inet_aton(HOST, &address.sin_addr);
    int addrlen = sizeof address;

    if (bind(soc, (struct sockaddr *)&address, sizeof address) == -1) {
        printf("Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // listen
    if (listen(soc, 0) == -1) {
        printf("Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Server running on http://%s:%d\n", HOST, PORT);

    int client_soc;
    char request[MAX_RECV];
    char res[MAX_RECV];

    while (1) {

        // accept
        client_soc = accept(soc, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        if (client_soc == -1) {
            close(soc);
            printf("Error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // connection successful

        printf("Client %d connected.\n", ntohs(address.sin_port));

        // read request
        recv(client_soc, request, MAX_RECV, 0);

        printf("\n\n\n\n%s\n\n\n", request);

        Req *req = parse_http_request(request);

        printf("method: %s uri: %s version: %s\n", req->method, req->uri, req->version);

        for (int i = 0; i < req->header_count; ++i) {
            printf("%s: %s\n", req->headers[i].name, req->headers[i].value);
        }

        printf("body: %s\n", req->body);

        free_req(req);

        // send response
        snprintf(res, MAX_RECV, "HTTP/1.1 200 OK\r\n\r\n<h1>Hello %d!</h1>", ntohs(address.sin_port));
        send(client_soc, res, strlen(res), 0);

        close(client_soc);
        printf("Client %d disconnected.\n", ntohs(address.sin_port));
    }

    printf("Exiting\n");
    close(soc);

    return 0;
}