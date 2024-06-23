#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "../include/socked.h"


Sc_Server *sc_server() {

    Sc_Server *server = (Sc_Server *) malloc(sizeof(Sc_Server));
    memset(server, 0, sizeof(Sc_Server));

    // TODO: add error handle

    int soc = socket(AF_INET, SOCK_STREAM, 0);
    server->socket = soc;    
    server->bind = 0;
    server->route_count = 0;

    // reuse address
    int opt = 1;
    setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    return server;
}


void sc_listen(Sc_Server *server, const char *host, int port) {

    int soc = server->socket;

    // TODO: add error handle

    // bind
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_aton(host, &address.sin_addr);
    int addrlen = sizeof address;

    bind(soc, (struct sockaddr *)&address, sizeof address);

    server->bind = 1;

    // listen
    listen(soc, 0);

    server->host = strdup(host);
    server->port = port;

    printf("Server running on http://%s:%d\n", host, port);


    // accept loop
    int client_soc;
    char request[SC_MAX_REQ];

    while (1) {

        client_soc = accept(soc, 0, 0);

        if (client_soc == -1) {
            close(client_soc);
        }

        // read request and parse to Sc_Request object
        recv(client_soc, request, SC_MAX_REQ, 0);
        printf("==Request==\n%s====\n", request);
        Sc_Request *req = sc_parse_http_request(request);
        req->header_count = 0;

        // prepare response
        Sc_Response *res = (Sc_Response *) malloc(sizeof(Sc_Response));
        memset(res, 0, sizeof(Sc_Response));
        res->header_count = 0;

        strcpy(res->version, SC_HTTP_VERSION); // by default

        // route found, method matched but status code and body didnt specified yet
        // return 200 OK and empty body by default
        res->status_code = 200;
        res->status_message = strdup("Ok"); 
        res->body = strdup("");

        // route if matched rule exists else return default response
        int matched = __sc_route_request(server, req, res);

        // get response as string
        char *response;
        response = sc_get_res_as_text(res);

        // send response to client
        send(client_soc, response, strlen(response), 0);

        // free memory and close connection
        free(response);
        sc_free_request(req);
        sc_free_response(res);
        close(client_soc);
    }

    printf("Exiting\n");
    close(soc);
}


int __sc_route_request(Sc_Server *server, Sc_Request *req, Sc_Response *res) {

    int route_matched = 0;
    int method_matched = 0;

    for (int i = 0; i < server->route_count; ++i) {
        
        // match route
        if (strcmp(server->routes[i].uri, req->uri) == 0) {

            route_matched = 1;

            // match method
            if (server->routes[i].method == SC_ALL ||
                server->routes[i].method == req->method) {

                // route and method matched, run callback
                server->routes[i].handler(req, res);
                method_matched = 1;

                return 1;
            }
        }
    }

    if (route_matched && !method_matched) {

        // route found, method is not allowed
        // return 405
        
        res->status_code = 405;
        res->status_message = strdup("Method Not Allowed"); 
        res->body = strdup("405 Method Not Allowed");

        return 0;
    }

    // route not found
    // return 404

    res->status_code = 404;
    res->status_message = strdup("Not Found"); 
    res->body = strdup("404 Not Found");

    return 0;
}


void sc_get(Sc_Server *server, char *uri, Sc_Route_Handler handler) {

    Sc_Route *route = (Sc_Route *) malloc(sizeof(Sc_Route));
    memset(route, 0, sizeof(Sc_Route));

    server->routes = (Sc_Route *) realloc(server->routes,
        (server->route_count+1)*sizeof(Sc_Route));

    if (server->routes == NULL) {
        printf("Cannot realloc memory.\n");
    }

    server->routes[server->route_count].method = SC_GET;
    server->routes[server->route_count].uri = strdup(uri);
    server->routes[server->route_count].handler = handler;

    server->route_count++;
}


void sc_post(Sc_Server *server, char *uri, Sc_Route_Handler handler) {

    Sc_Route *route = (Sc_Route *) malloc(sizeof(Sc_Route));
    memset(route, 0, sizeof(Sc_Route));

    server->routes = (Sc_Route *) realloc(server->routes,
        (server->route_count+1)*sizeof(Sc_Route));

    if (server->routes == NULL) {
        printf("Cannot realloc memory.\n");
    }

    server->routes[server->route_count].method = SC_POST;
    server->routes[server->route_count].uri = strdup(uri);
    server->routes[server->route_count].handler = handler;

    server->route_count++;
}


void sc_put(Sc_Server *server, char *uri, Sc_Route_Handler handler) {

    Sc_Route *route = (Sc_Route *) malloc(sizeof(Sc_Route));
    memset(route, 0, sizeof(Sc_Route));

    server->routes = (Sc_Route *) realloc(server->routes,
        (server->route_count+1)*sizeof(Sc_Route));

    if (server->routes == NULL) {
        printf("Cannot realloc memory.\n");
    }

    server->routes[server->route_count].method = SC_PUT;
    server->routes[server->route_count].uri = strdup(uri);
    server->routes[server->route_count].handler = handler;

    server->route_count++;
}


void sc_delete(Sc_Server *server, char *uri, Sc_Route_Handler handler) {

    Sc_Route *route = (Sc_Route *) malloc(sizeof(Sc_Route));
    memset(route, 0, sizeof(Sc_Route));

    server->routes = (Sc_Route *) realloc(server->routes,
        (server->route_count+1)*sizeof(Sc_Route));

    if (server->routes == NULL) {
        printf("Cannot realloc memory.\n");
    }

    server->routes[server->route_count].method = SC_DELETE;
    server->routes[server->route_count].uri = strdup(uri);
    server->routes[server->route_count].handler = handler;

    server->route_count++;
}


void sc_route(Sc_Server *server, char *uri, Sc_Route_Handler handler) {

    Sc_Route *route = (Sc_Route *) malloc(sizeof(Sc_Route));
    memset(route, 0, sizeof(Sc_Route));

    server->routes = (Sc_Route *) realloc(server->routes,
        (server->route_count+1)*sizeof(Sc_Route));

    if (server->routes == NULL) {
        printf("Cannot realloc memory.\n");
    }

    server->routes[server->route_count].method = SC_ALL;
    server->routes[server->route_count].uri = strdup(uri);
    server->routes[server->route_count].handler = handler;

    server->route_count++;
}