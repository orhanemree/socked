#ifndef SC_RES_H
#define SC_RES_H


#include "request.h"


typedef struct {
    char version[9]; // HTTP-Version
    int status_code; // Status Code
    char *status_message; // Reason Phrase
    Sc_Header *headers; // Array of Headers
    size_t header_count; // Headers count
    char *body; // Response Body
    size_t body_len; // Length of response body
    size_t total_len; // Length of response
} Sc_Response;


// join headers and return text compatible with HTTP response
char *__sc_get_headers_as_text(Sc_Header *headers, size_t header_count);


// join response line, headers and body and return valid HTTP response text
char *sc_get_res_as_text(Sc_Response *res);


// set status of response
void sc_set_status(Sc_Response *res, int status_code, const char *status_message);


// add header to response
void sc_set_header(Sc_Response *res, const char *header_name, const char *header_value);


// set string body of response 
void sc_set_body(Sc_Response *res, const char *data);


// add string data to end of existing response body
void sc_append_body(Sc_Response *res, const char *data);


// set file content as body of response return success status
int sc_set_body_file(Sc_Response *res, const char *filename);


// get mime type of file
const char *sc_get_mime_type(const char *path);


// free response memory
void sc_free_response(Sc_Response *res);


#endif // SC_RES_H
