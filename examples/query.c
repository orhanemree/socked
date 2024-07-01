#include <stdlib.h>


#include "../include/socked.h"


/*
    This example shows how to get query parameters from request.
    App behaves differently depending on if username query param is provided or not at URL "/"
*/


#define HOST "127.0.0.1" // localhost
#define PORT 8080


// request handler
void handle_username(Sc_Request *req, Sc_Response *res) {


    // check if query param exists
    if (sc_get_query(req, "username")) {

        // get query param
        char *username = sc_get_query(req, "username");

        sc_set_body(res, "<h1>Hello, ");
        sc_append_body(res, username);
        sc_append_body(res, "!</h1>");

        // always free() params after use
        free(username);

    } else {
        sc_set_body(res, "Index page. Username query paramter didn't provide");
    }

    // set Content-Type header if you want to
    sc_set_header(res, "Content-Type", "text/html");

    // status set 200 OK by default
} 


int main() {

    // create socket server
    Sc_Server *server = sc_server();

    // register GET request handler at URL "/"
    sc_get(server, "/", handle_username);

    // start server and listen to requests
    sc_listen(server, HOST, PORT);
}