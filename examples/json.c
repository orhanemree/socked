#include "../include/socked.h"


/*
    The app responds with a JSON object for requests to the URL "/"
*/


#define HOST "127.0.0.1" // localhost
#define PORT 8080

// request handler
void handle_index(Sc_Request *req, Sc_Response *res) {

    // send json object
    sc_set_body(res, "{\"message\": \"Hello, this a json message!\"}");

    // set content type header
    sc_set_header(res, "Content-Type", "application/json");

    // status set 200 Ok by default
} 

int main() {

    // create socket server
    Sc_Server *server = sc_server();

    // register GET request handler on URL "/"
    sc_get(server, "/", handle_index);

    // start server and listen to requests
    sc_listen(server, HOST, PORT);
}