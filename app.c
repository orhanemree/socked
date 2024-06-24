#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "include/socked.h"


void handle_index(Sc_Request *req, Sc_Response *res) {

    sc_set_header(res, "Content-Type", "text/html");
    sc_set_body_file(res, "www/index.html");
}


void handle_script(Sc_Request *req, Sc_Response *res) {

    sc_set_header(res, "Content-Type", "application/javascript");
    sc_set_body_file(res, "www/script.js");
}



int main() {

    Sc_Server *socked_server = sc_server();

    // sc_get(socked_server, "/", &handle_index);
    // sc_get(socked_server, "/index.html", &handle_index);

    // sc_get(socked_server, "/script.js", &handle_script);

    sc_static(socked_server, "/a/b", "www");

    /*
    GET /a/b HTTP/1.1 -> www/index.html
    GET /a/b/script.js HTTP/1.1 -> www/script.js
    */

    sc_listen(socked_server, "127.0.0.1", 8080);
}