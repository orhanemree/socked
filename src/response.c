#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../include/response.h"
#include "../include/socked.h"


char *__sc_get_headers_as_text(Sc_Header *headers, size_t header_count) {

    size_t text_len = 0;

    if (header_count == 0) {
        return "";
    }

    for (int i = 0; i < header_count; ++i) {
        text_len += strlen(headers[i].name)+strlen(headers[i].value);
        text_len += 2; // for ": "
        text_len += 2; // for CRLF
    }

    char *text = (char *) malloc((text_len+1)*sizeof(char));
    text[0] = '\0';

    for (int i = 0; i < header_count; ++i) {
        strcat(text, headers[i].name);
        strcat(text, ": ");
        strcat(text, headers[i].value);
        strcat(text, "\r\n");
    }

    text[text_len] = '\0';

    return text;
}


char *sc_get_res_as_text(Sc_Response *res) {

    // get headers as text
    char *headers_as_text = __sc_get_headers_as_text(res->headers, res->header_count);

    // calculate length of response
    size_t len = 14; // for first line
    len += strlen(res->status_message) + 2; // for CRLF
    len += strlen(headers_as_text) + 2; // for CRLF
    len += strlen(res->body);

    char *response = (char *) malloc((len+1)*sizeof(char));
    snprintf(response, len, "%s %d %s\r\n%s\r\n%s",
        res->version, res->status_code,
        res->status_message, headers_as_text, res->body);

    if (res->header_count != 0) {
        free(headers_as_text);
    }

    return response;
}


void sc_set_status(Sc_Response *res, int status_code, char *status_message){

    res->status_code = status_code;
    res->status_message = strdup(status_message);

}


void sc_set_header(Sc_Response *res, char *header_name, char *header_value) {

    res->headers = (Sc_Header *) realloc(res->headers,
        (res->header_count+1)*sizeof(Sc_Header));
    
    if (res->headers == NULL) {
        printf("Cannot realloc memory.\n");
        return;
    }

    res->headers[res->header_count].name = strdup(header_name);
    res->headers[res->header_count].value = strdup(header_value);

    res->header_count++;
}


void sc_set_body(Sc_Response *res, char *data) {

    res->body = strdup(data);

}


void sc_append_body(Sc_Response *res, char *data) {

    size_t prev_len = strlen(res->body);
    size_t new_len = prev_len+strlen(data);

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
