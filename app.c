#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "include/socked.h"


void handle_index(Sc_Request *req, Sc_Response *res) {
    sc_set_status(res, 200, "Ok");
    sc_set_body(res, "Hello, World!\n");

    sc_set_header(res, "Server", "socked");
    sc_set_header(res, "Content-Type", "text/plain");
}


int main() {

    Sc_Server *server = sc_server();

    sc_get(server, "/", &handle_index);

    sc_listen(server, "127.0.0.1", 8080);
}