#include "../include/socked.h"


/*
    The app responds with "Hello, World!" for requests to the URL "/"
    For every other path, it wil respond with a 404 Not Found  
*/


#define HOST "127.0.0.1" // localhost
#define PORT 8080

// request handler
void handle_index(Sc_Request *req, Sc_Response *res) {

    // send hello world response
    sc_set_body(res, "Hello, World!");

    // status set 200 Ok by default

    // you can specify headers if you want to
    sc_set_header(res, "Content-Type", "text/plain");
    sc_set_header(res, "Content-Length", "13");
} 

int main() {

    // create socket server
    Sc_Server *server = sc_server();

    // register GET request handler on URL "/"
    sc_get(server, "/", handle_index);

    // start server and listen to requests
    sc_listen(server, HOST, PORT);
}