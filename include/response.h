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
} Sc_Response;


char *sc_get_res_as_text(Sc_Response *res);


// TODO: add sc_has_header()
// TODO: add sc_set_header()


void sc_set_status(Sc_Response *res, int status_code, char *status_message);


void sc_set_body(Sc_Response *res, char *data);


void sc_append_body(Sc_Response *res, char *data);


void sc_free_response(Sc_Response *res);


#endif // SC_RES_H
