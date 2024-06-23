#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "include/socked.h"


void handle_index(Sc_Request *req, Sc_Response *res) {
    sc_set_status(res, 200, "Ok");
    sc_set_body(res, "Hello, World!\n");
}


void handle_admin(Sc_Request *req, Sc_Response *res) {
    sc_set_status(res, 201, "Created");
    sc_set_body(res, "Hello, Admin! Created!\n");
}


int main() {

    Sc_Server *server = sc_server();

    sc_get(server, "/", &handle_index);
    sc_post(server, "/admin", &handle_admin);

    sc_listen(server, "127.0.0.1", 8080);
}