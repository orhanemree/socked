#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#include "../include/socked.h"


Sc_Server *sc_server() {

    Sc_Server *server = (Sc_Server *) malloc(sizeof(Sc_Server));
    memset(server, 0, sizeof(Sc_Server));

    // TODO: add error handle

    int soc = socket(AF_INET, SOCK_STREAM, 0);
    server->socket = soc;    
    server->bind = 0;
    server->route_count = 0;
    server->static_uri = strdup("");
    server->static_folder = strdup("");

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


    // setup select
    fd_set current_sockets, readable_sockets;
    int max_sockets_so_far = 0;

    FD_ZERO(&current_sockets);
    FD_SET(soc, &current_sockets);

    max_sockets_so_far = soc;

    // accept loop

    while (1) {

        // select readable socket
        readable_sockets = current_sockets;

        int s = select(max_sockets_so_far+1, &readable_sockets, NULL, NULL, NULL);

        if (s < 0) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < max_sockets_so_far+1; ++i) {

            if (FD_ISSET(i, &readable_sockets)) {

                if (i == soc) {

                    // handle new connection
                    int client_socket = accept(soc, 0, 0);

                    if (client_socket == -1) { // just in case
                        close(client_socket);
                        return;
                    }

                    if (client_socket > max_sockets_so_far) {
                        max_sockets_so_far = client_socket;
                    }

                    FD_SET(client_socket, &current_sockets);

                } else {

                    // handle existing connection recv request
                    __sc_handle_request(server, i);
                    FD_CLR(i, &current_sockets);
                }
            }
        }
    }

    close(soc);
}


void __sc_handle_request(Sc_Server *server, int client_socket) {

    // read request and parse to Sc_Request object

    char request[SC_MAX_REQ];

    recv(client_socket, request, SC_MAX_REQ, 0);
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

    // check if uri is served as static
    if (strlen(server->static_uri) != 0 && strlen(server->static_folder) != 0) {
        // static served folder exists

        if (strncmp(req->uri, server->static_uri, strlen(server->static_uri)) == 0) {
            // matched

            // find abs path of static file
            size_t abs_path_len = strlen(req->uri)-strlen(server->static_uri)+strlen(server->static_folder);

            char *abs_path = (char *) malloc((abs_path_len+1)*sizeof(char));
            abs_path[0] = '\0';

            strcat(abs_path, server->static_folder);
            strcat(abs_path, req->uri+strlen(server->static_uri));

            abs_path[abs_path_len] = '\0';

            // change body directly
            // static routes does not have handler function
            int success = sc_set_body_file(res, abs_path);

            if (!success) {
                // file not found or something went wrong while reading the file
                res->status_code = 404;
                res->status_message = strdup("Not Found"); 
                res->body = strdup("404 Not Found");
            }

            free(abs_path);
        }


    } else {
        // static served folder does not exist
        // try match with rules

        // route if matched rule exists else return default response
        int matched = __sc_route_request(server, req, res);
    }

    // get response as string
    char *response;
    response = sc_get_res_as_text(res);

    // send response to client
    send(client_socket, response, strlen(response), 0);

    // free memory and close connection
    free(response);
    sc_free_request(req);
    sc_free_response(res);
    close(client_socket);
}


int __sc_route_request(Sc_Server *server, Sc_Request *req, Sc_Response *res) {

    int route_matched = 0;
    int method_matched = 0;

    // try for each rule
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


void sc_static(Sc_Server *server, char *uri, char *folder) {

    char *abs_path = realpath(folder, NULL);

    server->static_uri = strdup(uri);
    server->static_folder = strdup(abs_path);

    free(abs_path);
}
