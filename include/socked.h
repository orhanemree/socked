#ifndef SC_SOCKED_H
#define SC_SOCKED_H


#include <stdio.h>

#include "request.h"
#include "response.h"


#define SC_MAX_REQ 1024*8
#define SC_HTTP_VERSION "HTTP/1.1"


typedef void (*Sc_Route_Handler)(Sc_Request*, Sc_Response*);


typedef struct {
    Sc_Method method;
    char *uri;
    Sc_Route_Handler handler;
} Sc_Route;


typedef struct {
    int socket;
    int bind;
    char *host;
    int port;
    Sc_Route *routes;
    size_t route_count;
} Sc_Server;


Sc_Server *sc_server();


void sc_listen(Sc_Server *server, const char *host, int port);


void __sc_handle_request(Sc_Server *server, int client_socket);


int __sc_route_request(Sc_Server *server, Sc_Request *req, Sc_Response *res);


void sc_get(Sc_Server *server, char *uri, Sc_Route_Handler handler);


void sc_post(Sc_Server *server, char *uri, Sc_Route_Handler handler);


void sc_put(Sc_Server *server, char *uri, Sc_Route_Handler handler);


void sc_delete(Sc_Server *server, char *uri, Sc_Route_Handler handler);


void sc_route(Sc_Server *server, char *uri, Sc_Route_Handler handler);


#endif // SC_SOCKED_H
