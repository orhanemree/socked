#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>

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
    res->body_len = 0;
    res->total_len = 0;

    strcpy(res->version, SC_HTTP_VERSION); // by default

    // route found, method matched but status code and body didnt specified yet
    // return 200 OK and empty body by default
    sc_set_status(res, 200, "Ok");
    sc_set_body(res, "");

    int static_success = 0;

    // check if static served folder exists
    if (strlen(server->static_uri) != 0 && strlen(server->static_folder) != 0) {

        // check if the uri served as static
        if (strncmp(req->uri, server->static_uri, strlen(server->static_uri)) == 0) {
            static_success = __sc_handle_static(server, req, res);
        }

    } 
    
    if (!static_success) {
        // static served folder does not exist
        // try match with rules

        // route if matched rule exists else return default response
        int matched = __sc_route_request(server, req, res);
    }
    
    // get response as string
    char *response;
    response = sc_get_res_as_text(res);

    // send response to client
    int s_ = send(client_socket, response, res->total_len, 0);

    // free memory and close connection
    free(response);
    sc_free_request(req);
    sc_free_response(res);
    close(client_socket);
}


int __sc_handle_static(Sc_Server *server, Sc_Request *req, Sc_Response *res) {

    // static paths only handles GET 
    if (req->method != SC_GET) {
        
        // another method instead of GET on static path
        // return 405

        sc_set_status(res, 405, "Method Not Allowed");
        sc_set_body(res, "405 Method Not Allowed");

        return 0;
    }

    int is_uri_root = strcmp(server->static_uri, "/") == 0;

    // find abs path of static file
    size_t abs_path_len = strlen(req->uri)-strlen(server->static_uri)
        +strlen(server->static_folder) + is_uri_root;

    char *abs_path = (char *) malloc((abs_path_len+1)*sizeof(char));
    abs_path[0] = '\0';

    strcat(abs_path, server->static_folder);

    if (is_uri_root) {
        strcat(abs_path, "/");
    }

    strcat(abs_path, req->uri+strlen(server->static_uri));

    abs_path[abs_path_len] = '\0';

    // check if path is directory. if so return index.html if exists in the directory
    struct stat path_stat;
    stat(abs_path, &path_stat);
    int is_path_dir = S_ISDIR(path_stat.st_mode);

    if (is_path_dir) {

        if (req->uri[strlen(req->uri)-1] == '/') {
            // retunr index.html
            char *new_abs_path = (char *) realloc(abs_path,
                (abs_path_len+strlen("/index.html")+1)*sizeof(char));

            abs_path = new_abs_path;

            strcat(abs_path, "/index.html");

        } else {
            // redirect to return index.html with correct relative path
            sc_set_status(res, 301, "Moved Permanently");
            sc_set_body(res, "");

            char *redirect_path = (char *) malloc(strlen((req->uri)+2)*sizeof(char));
            strcat(redirect_path, req->uri);
            strcat(redirect_path, "/");

            sc_set_header(res, "Location", redirect_path);

            free(redirect_path);
            return 1;
        }
    }

    // change body directly
    // static routes does not have handler function
    int success = sc_set_body_file(res, abs_path);

    if (!success) {
        // file not found or something went wrong while reading the file
        sc_set_status(res, 404, "Not Found");
        sc_set_body(res, "404 Not Found");

        return 0;
    }

    free(abs_path);

    return 1;
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

                sc_set_status(res, 200, "Ok"); // by default
                sc_set_body(res, "");

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

        sc_set_status(res, 405, "Method Not Allowed");
        sc_set_body(res, "405 Method Not Allowed");

        return 0;
    }

    // route not found
    // return 404
    
    sc_set_status(res, 404, "Not Found");
    sc_set_body(res, "404 Not Found");

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
