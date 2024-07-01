#include "../include/socked.h"


/*
    The app handles different HTTP methods (GET, POST, etc.) with sc_route()

    - GET request to "/": Responds with "Your method is GET!"
    - POST request to "/": Responds with "Your method is POST!"
    - Other methods: Responds with "405 Method Not Allowed"
*/


#define HOST "127.0.0.1" // localhost
#define PORT 8080
 

// request handler
void handle_index(Sc_Request *req, Sc_Response *res) {

    // Do different operations for each method

    if (req->imethod == SC_GET) {
        // or strcmp(req->method, "GET") == 0

        sc_set_body(res, "Your method is GET!");

        // status is 200 by default

    } else if (req->imethod == SC_POST) {
        // or strcmp(req->method, "POST") == 0

        sc_set_body(res, "Your method is POST!");

        // status is 200 by default
    }
    // ...
    else { 
        // unknown method or method is not allowed

        // sc_route() does not 405 unlike other functions do
        // for example sc_post() returns a 405 error if method is GET
        // or something but not POST
        
        // send 405 manually

        sc_set_status(res, 405, "Method Not Allowed");
        sc_set_body(res, "405 Method Not Allowed");
    }
    
} 


int main() {

    // create socket server
    Sc_Server *server = sc_server();

    // register handler at URL "/" that matchs with all methods
    sc_route(server, "/", handle_index);

    // start server and listen to requests
    sc_listen(server, HOST, PORT);
}