#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../include/response.h"
#include "../include/socked.h"


char *sc_get_res_as_text(Sc_Response *res) {

    // calculate length of response
    size_t len = 8+1+3+1+strlen(res->status_message)+4+strlen(res->body)+1;

    // TODO: add headers

    char *response = (char *) malloc(len*sizeof(char));
    snprintf(response, len, "%s %d %s\r\n\r\n%s", res->version, res->status_code, res->status_message, res->body);

    return response;
}


void sc_set_status(Sc_Response *res, int status_code, char *status_message){

    res->status_code = status_code;
    res->status_message = strdup(status_message);

}


void sc_set_body(Sc_Response *res, char *data) {

    res->body = strdup(data);

}


void sc_append_body(Sc_Response *res, char *data) {

    size_t prev_len = strlen(res->body);
    size_t new_len = prev_len+strlen(data);

    printf("len: %ld\n", prev_len);

    char *new_body = (char *) realloc(res->body, (new_len+1)*sizeof(char));

    if (new_body == NULL) {
        printf("Cannot realloc memory.");
        return;
    }

    strcat(new_body, data);
    res->body = new_body;
}


void sc_free_response(Sc_Response *res) {

    for (int i = 0; i < res->header_count; ++i) {
        free(res->headers[i].name);
        free(res->headers[i].value);
    }

    free(res->headers);
    free(res->status_message);
    free(res->body);
    free(res);

}