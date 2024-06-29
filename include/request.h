#ifndef SC_REQ_H
#define SC_REQ_H


#include <stdio.h>


typedef enum {
    SC_OPTIONS,
    SC_GET,
    SC_HEAD,
    SC_POST,
    SC_PUT,
    SC_DELETE,
    SC_TRACE,
    SC_CONNECT,
    SC_ALL, // match with all methods
    SC_UNK // unknown-invalid method
} Sc_Method;


typedef struct {
    char *name; // Header name field
    char *value; // Header value field
} Sc_Header;


typedef struct {
    char *key; // Parameter key
    char *value; // Parameter value
} Sc_Param;


typedef struct {
    Sc_Method method; // Request Method
    char uri[255]; // Request-URI
    char **segments; // array of URI segments
    size_t seg_count; // path segment count of route URI
    char version[9]; // HTTP-Version
    Sc_Header *headers; // Array of Headers
    size_t header_count; // Headers count
    char *body; // Request Body
    Sc_Param *params; // array of dynamic route parameters
    size_t param_count; // dynamic route parameters count
} Sc_Request;


// parse plain HTTP request text into Request object
Sc_Request *sc_parse_http_request(char *request);


// add new header to headers array in Request object
// used in sc_parse_http_request() do not run directly
int __sc_add_header(Sc_Request *req, const char *header_name, const char *header_value);


// check if Request object has header specified by name
int sc_req_has_header(Sc_Request *req, const char *header_name);


// get copy of header value from Request object if exists, else return NULL
// run free() after you used the value
char *sc_req_get_header(Sc_Request *req, const char *header_name);


// add new parameter to params array in Request object
// runs in __sc_route_request() in file /src/socked.c, do not run directly
void __sc_add_param(Sc_Request *req, const char *param_key, const char *param_value);


// check if Request object has specified parameter
int sc_has_param(Sc_Request *req, const char *param_key);


// return copy of speficied paramter from Request object if exists, else return NULL
// run free() after used the value
char *sc_get_param(Sc_Request *req, const char *param_key);


// free Request object
void sc_free_request(Sc_Request *req);


#endif // SC_REQ_H
