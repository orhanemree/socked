#include <stdlib.h>


#include "../include/socked.h"


/*
    This examples shows how to add dynamic routes to socked server.

    - Add single dynamic route at /:username
    - Add multiple dynamic routes at /:username/posts/:post_id
    - Add casual static route at /admin to show  static handlers have priority over dynamic handlers
    - Check if request has dynamic route parameter with sc_has_param(req, param_key);
    - Get dynamic route parameter with sc_get_param(req, param_key);
*/


#define HOST "127.0.0.1" // localhost
#define PORT 8080


// request handler
void handle_username(Sc_Request *req, Sc_Response *res) {

    char *username = sc_get_param(req, "username");

    sc_set_body(res, "<h1>Hello, ");
    sc_append_body(res, username);
    sc_append_body(res, "!</h1>");

    // set Content-Type header if you want to
    sc_set_header(res, "Content-Type", "text/html");

    // you can check if param is exists
    if (sc_has_param(req, "post")) {
        // actually, this parameter cannot exist because parameter didn't specified
        // at request rule `sc_get(server, "/:username", handle_username);`
        printf("Unreachable\n");
    }

    // status set 200 OK by default

    // always free() params after use
    free(username);
} 


// request handler
void handle_post(Sc_Request *req, Sc_Response *res) {

    char *username = sc_get_param(req, "username");
    char *post_id = sc_get_param(req, "post_id");

    // check if post id exist in database or something
    // get post content ...

    sc_set_body(res, "<p>Showing post id: ");
    sc_append_body(res, post_id);
    sc_append_body(res, " from user: <b>");
    sc_append_body(res, username);
    sc_append_body(res, "</b></p>");

    // set Content-Type header if you want to
    sc_set_header(res, "Content-Type", "text/html");

    // status set 200 OK by default

    // always free() params after use
    free(username);
    free(post_id);
}


// request handler
void handle_admin(Sc_Request *req, Sc_Response *res) {

    sc_set_body(res, "ADMIN PAGE");

    // status set 200 OK by default
}


int main() {

    // create socket server
    Sc_Server *server = sc_server();

    // register GET dynamic request handler at URL "/:username"
    sc_get(server, "/:username", handle_username);

    // register GET dynamic request handler at URL "/:username/posts/:post_id"
    sc_get(server, "/:username/posts/:post_id", handle_post);

    // register GET request handler at URL "/admin"
    // static handlers have priority over dynamic handlers
    sc_get(server, "/admin", handle_admin);

    // start server and listen to requests
    sc_listen(server, HOST, PORT);
}