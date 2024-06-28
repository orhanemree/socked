#include <string.h>
#include <stdlib.h>


#include "../include/socked.h"


/*
    The app simulates an auth request at path "/auth".
    Responds with 200 OK if auth is successful.
    Responds with 401 Unauthorized if auth has faied.
*/


#define HOST "127.0.0.1" // localhost
#define PORT 8080


// request handler
void handle_auth(Sc_Request *req, Sc_Response *res) {

    // simulate a VERY basic auth

    // first check if Authorization was specified in the request
    if (!sc_req_has_header(req, "Authorization")) {

        // send 401 error
        sc_set_status(res, 401, "Unauthorized");

        // send WWW-Authenticate header
        sc_set_header(res, "WWW-Authenticate", "Basic Realm=\"REALM\"");

        sc_set_body(res, "Auth failed!");
        return;
    }

    // header exists, get auth header
    char *auth_header = sc_req_get_header(req, "Authorization");

    if (strcmp(auth_header, "Basic CREDENTIAL") == 0) {

        sc_set_body(res, "Auth successful!");

        // status is 200 by default

    } else {

        // send 401 error
        sc_set_status(res, 401, "Unauthorized");

        sc_set_body(res, "Auth failed!");
    }

    // free headers after use
    free(auth_header);
} 


int main() {

    // create socket server
    Sc_Server *server = sc_server();

    // register POST request handler at URL "/"
    sc_post(server, "/auth", handle_auth);

    // start server and listen to requests
    sc_listen(server, HOST, PORT);
}