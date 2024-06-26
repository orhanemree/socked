#ifndef SC_SOCKED_H
#define SC_SOCKED_H


#include <stdio.h>

#include "request.h"
#include "response.h"


#define SC_MAX_REQ 1024*8
#define SC_HTTP_VERSION "HTTP/1.1"


typedef void (*Sc_Route_Handler)(Sc_Request*, Sc_Response*);


typedef struct {
    Sc_Method method; // HTTP Method
    char *uri; // HTTP URI
    Sc_Route_Handler handler; // callback function handles request
} Sc_Route;


typedef struct {
    int socket; // the server socket 
    int bind; // is socket binded
    char *host; // server host eg. "127.0.0.1"
    int port; // server port eg. 8080
    Sc_Route *routes; // array of routes that server handles
    size_t route_count; // route count
    char *static_uri; // static URI
    char *static_folder; // static served folder
} Sc_Server;


// create socket server
Sc_Server *sc_server();


// listen and accept connections. this function has to run at the end of the file
void sc_listen(Sc_Server *server, const char *host, int port);


// handle selected request. do not run directly, runs in sc_listen()
void __sc_handle_request(Sc_Server *server, int client_socket);



// handle static served path return success state do not run directly, runs in __sc_handle_request()
int __sc_handle_static(Sc_Server *server, Sc_Request *req, Sc_Response *res);


// route handled request return success state do not run directly, runs in __sc_handle_request()
int __sc_route_request(Sc_Server *server, Sc_Request *req, Sc_Response *res);


// add get rule to server on the uri
void sc_get(Sc_Server *server, const char *uri, Sc_Route_Handler handler);


// add post rule to server on the uri
void sc_post(Sc_Server *server, const char *uri, Sc_Route_Handler handler);


// add put rule to server on the uri
void sc_put(Sc_Server *server, const char *uri, Sc_Route_Handler handler);


// add delete rule to server on the uri
void sc_delete(Sc_Server *server, const char *uri, Sc_Route_Handler handler);


// add rule for all methods to server on the uri
void sc_route(Sc_Server *server, const char *uri, Sc_Route_Handler handler);


// serve specified folder on the uri staticly
void sc_static(Sc_Server *server, const char *uri, const char *folder);


#endif // SC_SOCKED_H
