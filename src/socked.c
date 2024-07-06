#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "../include/socked.h"


Sc_Server *sc_server() {

    Sc_Server *server = (Sc_Server *) malloc(sizeof(Sc_Server));

    if (server == NULL) {
        __sc_exit(-1, NULL, NULL, NULL, "Cannot malloc server");
    }

    memset(server, 0, sizeof(Sc_Server));

    server->bind = 0;
    server->route_count = 0;
    server->static_uri = strdup("");
    server->static_folder = strdup("");

    int soc = socket(AF_INET, SOCK_STREAM, 0);
    
    if (soc < 0) {
        __sc_exit(-1, server, NULL, NULL, "Socket create failed");
    }

    server->socket = soc;

    // reuse address
    int opt = 1;
    if (setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) < 0) {
        __sc_exit(soc, server, NULL, NULL, "Setsockopt failed");
    }

    return server;
}


void sc_listen(Sc_Server *server, const char *host, int port) {

    int soc = server->socket;

    // bind
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_aton(host, &address.sin_addr);

    if (bind(soc, (struct sockaddr *)&address, sizeof address) < 0) {
        __sc_exit(soc, server, NULL, NULL, "Bind failed");
    } 

    server->bind = 1;

    // listen
    if (listen(soc, 0) < 0) {
        __sc_exit(soc, server, NULL, NULL, "Listen failed");
    }

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
            __sc_exit(soc, server, NULL, NULL, "Select failed");
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
                    Sc_Request *req;
                    Sc_Response *res;

                    int success = __sc_handle_request(i, server, req, res);
                    if (success < 0) {
                        __sc_error(i, req, res, "Cannot handle request");
                    }
                    FD_CLR(i, &current_sockets);
                }
            }
        }
    }

    close(soc);
}


int __sc_handle_request(int client_socket, Sc_Server *server, Sc_Request *req, Sc_Response *res) {

    // read request and parse to Sc_Request object

    char request[SC_MAX_REQ];

    recv(client_socket, request, SC_MAX_REQ, 0);
    req = sc_parse_http_request(request);
    if (req == NULL) return -1;

    // log request
    // format: <%Y-%m-%dT%H:%M:%S%z> <METHOD> <URL>

    time_t now = time(NULL);
    struct tm *sTm;
    sTm = localtime(&now);
    char log_time[30];
    strftime(log_time, sizeof(log_time), "%Y-%m-%d %H:%M:%S", sTm);
    printf("[%s] %s http://%s:%d%s\n", log_time, req->method, server->host, server->port, req->uri);

    // prepare response
    res = (Sc_Response *) malloc(sizeof(Sc_Response));
    if (res == NULL) return -1;
    memset(res, 0, sizeof(Sc_Response));

    res->header_count = 0;
    res->body_len = 0;
    res->total_len = 0;

    strcpy(res->version, SC_HTTP_VERSION); // by default

    // route found, method matched but status code and body didnt specified yet
    // return 200 OK and empty body by default
    sc_set_status(res, 200, "OK");
    sc_set_body(res, " ");
    res->is_body_set = 0;

    int static_success = 0;

    // check if static served folder exists
    if (strlen(server->static_uri) != 0 && strlen(server->static_folder) != 0) {

        // check if the uri served as static
        if (strncmp(req->uri, server->static_uri, strlen(server->static_uri)) == 0) {
            static_success = __sc_handle_static(server, req, res);
            if (static_success < 0) return -1;
        }

    } 
    
    if (!static_success) {
        // static served folder does not exist
        // try match with rules

        // route if matched rule exists else return default response
        int route_success = __sc_route_request(server, req, res);
        if (route_success < 0) return -1;
    }
    
    // get response as string
    char *response = sc_get_res_as_text(res);
    if (response == NULL) return -1;

    // send response to client
    int s_ = send(client_socket, response, res->total_len, 0);

    if (s_ < 0) {
        // TODO: dont exit just log
    }

    if (s_ != res->total_len) {
        // TODO: dont exit just log
    } 

    // free memory and close connection
    free(response);
    sc_free_request(req);
    sc_free_response(res);
    close(client_socket);
}


int __sc_handle_static(Sc_Server *server, Sc_Request *req, Sc_Response *res) {

    // static paths only handles GET 
    if (req->imethod != SC_GET) {
        
        // another method instead of GET on static path
        // return 405

        sc_set_status(res, 405, "Method Not Allowed");
        sc_set_body(res, "405 Method Not Allowed");
        res->is_body_set = 0;

        return 0;
    }

    int is_uri_root = strcmp(server->static_uri, "/") == 0;

    // find abs path of static file
    size_t abs_path_len = strlen(req->uri)-strlen(server->static_uri)
        +strlen(server->static_folder) + is_uri_root;

    char *abs_path = (char *) malloc((abs_path_len+1)*sizeof(char));
    if (abs_path == NULL) return -1;
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
            // return index.html
            char *new_abs_path = (char *) realloc(abs_path,
                (abs_path_len+strlen("/index.html")+1)*sizeof(char));
            if (new_abs_path == NULL) return -1;

            abs_path = new_abs_path;

            strcat(abs_path, "/index.html");

        } else {
            // redirect to return index.html with correct relative path
            sc_set_status(res, 307, "Temporary Redirect");
            sc_set_body(res, "307 Temporary Redirect");
            res->is_body_set = 0;

            size_t redirect_path_len = strlen(req->uri)+1;
            char *redirect_path = (char *) malloc((redirect_path_len+1)*sizeof(char));
            if (redirect_path == NULL) return -1;
            redirect_path[0] = '\0';
            strcat(redirect_path, req->uri);
            strcat(redirect_path, "/");
            redirect_path[redirect_path_len] = '\0';

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
        res->is_body_set = 0;

        return 0;
    }

    free(abs_path);

    return 1;
}


int __sc_route_request(Sc_Server *server, Sc_Request *req, Sc_Response *res) {

    int route_matched = 0;
    int method_matched = 0;
    int dynamic_mathced = 1;
    int matched_dynamic_i = -1;

    // try for each rule
    for (int i = 0; i < server->route_count; ++i) {

        // match route

        if (!(server->routes[i].has_dynamic_seg)) {
            // uri has no dynmic segment

            if (strcmp(server->routes[i].uri, req->uri) == 0) {

                route_matched = 1;

                // match method
                if (server->routes[i].method == SC_ALL ||
                    server->routes[i].method == req->imethod) {

                    sc_set_status(res, 200, "OK"); // by default
                    sc_set_body(res, " ");
                    res->is_body_set = 0;

                    // route and method matched, run callback
                    server->routes[i].handler(req, res);
                    method_matched = 1;

                    return 1;
                }
            }

        } else {
            // uri has dynamic segment


            // match segment counts
            if (req->seg_count == server->routes[i].seg_count) {
                // segment counts match

                route_matched = 1;
                
                // match method
                if (server->routes[i].method == SC_ALL ||
                    server->routes[i].method == req->imethod) {
                        
                    for (int j = 0; j < req->seg_count; ++j) {

                        // only compare not dynamic segments
                        if (dynamic_mathced && !server->routes[i].seg_is_dynamic[j]) {
                            
                            if (strcmp(req->segments[j], server->routes[i].segments[j]) != 0) {
                                dynamic_mathced = 0;
                                continue;
                            }
                        }
                    }

                } else {
                    dynamic_mathced = 0;
                }

                if (dynamic_mathced) {
                    matched_dynamic_i = i;
                }
            }
        }
    }

    if (dynamic_mathced && matched_dynamic_i != -1) {
        // send dynamic route paramaters to req object
        for (int j = 0; j < req->seg_count; ++j) {
            if (server->routes[matched_dynamic_i].seg_is_dynamic[j]) {
                __sc_add_param(req, server->routes[matched_dynamic_i].segments[j]+1, req->segments[j]);
            }
        }

        server->routes[matched_dynamic_i].handler(req, res);
        return 1;
    }

    if (route_matched && !method_matched) {

        // route found, method is not allowed
        // return 405

        sc_set_status(res, 405, "Method Not Allowed");
        sc_set_body(res, "405 Method Not Allowed");
        res->is_body_set = 0;

        return 0;
    }

    // route not found
    // return 404
    
    sc_set_status(res, 404, "Not Found");
    sc_set_body(res, "404 Not Found");
    res->is_body_set = 0;

    return 0;
}


int __sc_parse_route_uri(Sc_Route *route, const char *uri) {

    route->segments = (char **) malloc(SC_MAX_SEG*sizeof(char *));
    if (route->segments == NULL) return -1;
    route->seg_count = 0;
    // segments are not dynamic by default
    memset(route->seg_is_dynamic, 0, SC_MAX_SEG);

    char *uri_cpy = strdup(uri);
    const char *seg;

    seg = strtok(uri_cpy, "/");

    while (seg != NULL) {
        // it is a dynamic segment

        route->segments[route->seg_count] = (char *) malloc((strlen(seg)+1)*sizeof(char));
        if (route->segments[route->seg_count] == NULL) {
            free(uri_cpy);
            return -1;
        }
        route->segments[route->seg_count] = strdup(seg);

        if (seg[0] == ':') {
            if (!route->has_dynamic_seg) route->has_dynamic_seg = 1;
            route->seg_is_dynamic[route->seg_count] = 1;
        }
        seg = strtok(NULL, "/");
        route->seg_count++;
    }

    free(uri_cpy);
}


void sc_get(Sc_Server *server, const char *uri, Sc_Route_Handler handler) {

    server->routes = (Sc_Route *) realloc(server->routes,
        (server->route_count+1)*sizeof(Sc_Route));
    if (server->routes == NULL) {
        __sc_error(-1, NULL, NULL, "Routes realloc failed");
        return;
    }

    int p_success = __sc_parse_route_uri(&(server->routes[server->route_count]), uri);

    if (p_success < 0) {
        __sc_error(-1, NULL, NULL, "Route URI parse failed");
        return;
    }

    server->routes[server->route_count].method = SC_GET;
    server->routes[server->route_count].uri = strdup(uri);
    server->routes[server->route_count].handler = handler;

    server->route_count++;
}


void sc_post(Sc_Server *server, const char *uri, Sc_Route_Handler handler) {

    server->routes = (Sc_Route *) realloc(server->routes,
        (server->route_count+1)*sizeof(Sc_Route));
    if (server->routes == NULL) {
        __sc_error(-1, NULL, NULL, "Routes realloc failed");
        return;
    }

    int p_success = __sc_parse_route_uri(&(server->routes[server->route_count]), uri);

    if (p_success < 0) {
        __sc_error(-1, NULL, NULL, "Route URI parse failed");
        return;
    }

    server->routes[server->route_count].method = SC_POST;
    server->routes[server->route_count].uri = strdup(uri);
    server->routes[server->route_count].handler = handler;

    server->route_count++;
}


void sc_put(Sc_Server *server, const char *uri, Sc_Route_Handler handler) {

    server->routes = (Sc_Route *) realloc(server->routes,
        (server->route_count+1)*sizeof(Sc_Route));
    if (server->routes == NULL) {
        __sc_error(-1, NULL, NULL, "Routes realloc failed");
        return;
    }

    int p_success = __sc_parse_route_uri(&(server->routes[server->route_count]), uri);

    if (p_success < 0) {
        __sc_error(-1, NULL, NULL, "Route URI parse failed");
        return;
    }

    server->routes[server->route_count].method = SC_PUT;
    server->routes[server->route_count].uri = strdup(uri);
    server->routes[server->route_count].handler = handler;

    server->route_count++;
}


void sc_delete(Sc_Server *server, const char *uri, Sc_Route_Handler handler) {

    server->routes = (Sc_Route *) realloc(server->routes,
        (server->route_count+1)*sizeof(Sc_Route));
    if (server->routes == NULL) {
        __sc_error(-1, NULL, NULL, "Routes realloc failed");
        return;
    }

    int p_success = __sc_parse_route_uri(&(server->routes[server->route_count]), uri);

    if (p_success < 0) {
        __sc_error(-1, NULL, NULL, "Route URI parse failed");
        return;
    }

    server->routes[server->route_count].method = SC_DELETE;
    server->routes[server->route_count].uri = strdup(uri);
    server->routes[server->route_count].handler = handler;

    server->route_count++;
}


void sc_route(Sc_Server *server, const char *uri, Sc_Route_Handler handler) {

    server->routes = (Sc_Route *) realloc(server->routes,
        (server->route_count+1)*sizeof(Sc_Route));
    if (server->routes == NULL) {
        __sc_error(-1, NULL, NULL, "Routes realloc failed");
        return;
    }

    int p_success = __sc_parse_route_uri(&(server->routes[server->route_count]), uri);

    if (p_success < 0) {
        __sc_error(-1, NULL, NULL, "Route URI parse failed");
        return;
    }

    server->routes[server->route_count].method = SC_ALL;
    server->routes[server->route_count].uri = strdup(uri);
    server->routes[server->route_count].handler = handler;

    server->route_count++;
}


void sc_static(Sc_Server *server, const char *uri, const char *folder) {

    char *abs_path = realpath(folder, NULL);
    if (abs_path == NULL) {
        __sc_error(-1, NULL, NULL, "abs_path malloc failed");
        return;
    }

    server->static_uri = strdup(uri);
    server->static_folder = strdup(abs_path);

    free(abs_path);
}


void sc_free_server(Sc_Server *server) {

    for (int i = 0; i < server->route_count; ++i) {
        free(server->routes[i].uri);
        for (int j = 0; j < server->routes[i].seg_count; ++j) {
            free(server->routes[i].segments[j]);
        }
        free(server->routes[i].segments);
    }

    free(server->routes);
    free(server->host);
    free(server->static_uri);
    free(server->static_folder);
    free(server);
}


void __sc_exit(int soc, Sc_Server *server, Sc_Request *req, Sc_Response *res, const char *text) {

    if (soc != -1) close(soc);

    if (server) sc_free_server(server);
    if (req) sc_free_request(req);
    if (res) sc_free_response(res);

    fprintf(stderr, "%s: %s\nExiting\n", text, strerror(errno));
    exit(EXIT_FAILURE);
}


void __sc_error(int client_soc, Sc_Request *req, Sc_Response *res, const char *text) {
    
    if (client_soc != -1) close(client_soc);

    if (req) sc_free_request(req);
    if (res) sc_free_response(res);

    fprintf(stderr, "[ERR] %s: %s\n", text, strerror(errno));
}
