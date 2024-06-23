#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../include/helpers.h"
#include "../include/request.h"
#include "../include/socked.h"

#define SC_CRLF "\r\n"


Sc_Request *sc_parse_http_request(char *request) {

    Sc_Request *req = (Sc_Request *) malloc(sizeof(Sc_Request));
    memset(req, 0, sizeof(Sc_Request));
    
    char method[9];
    // parse first line: Method, Request-URI and HTTP-Version
    sscanf(request, "%s %s %s", method, req->uri, req->version);

    if (strcmp(method, "GET") == 0) {
        req->method = SC_GET;
    } else if (strcmp(method, "POST") == 0) {
        req->method = SC_POST;
    } else if (strcmp(method, "PUT") == 0) {
        req->method = SC_PUT;
    } else if (strcmp(method, "DELETE") == 0) {
        req->method = SC_DELETE;
    }else {
        req->method = SC_UNK;
    }

    // TODO: return 414 if uri > 255

    // split headers and body
    char *headers_p = strstr(request, SC_CRLF)+2;
    char *body_p = strstr(headers_p, (SC_CRLF SC_CRLF));

    size_t headers_len = body_p - headers_p;
    char *headers = (char *) malloc((headers_len+1)*sizeof(char));

    strncpy(headers, headers_p, headers_len);
    headers[headers_len] = '\0';

    // iterate 2 CRLFs
    body_p += 4;

    // parse headers
    char *p1, *p2;
    char *header = strtok_r(headers, SC_CRLF, &p1);
    char *name, *value;

    while (header != NULL) {

        // name field
        name = strtok_r(header, ":", &p2);
        name = sc_trim(name);

        // the rest -value field
        value = sc_trim(p2);

        // TODO: make field names case-insensitive

        __sc_add_header(req, name, value);

        free(name);
        free(value);

        header = strtok_r(NULL, SC_CRLF, &p1);
    }    

    free(headers);

    // save body
    req->body = strdup(body_p);

    return req;
}


int __sc_add_header(Sc_Request *req, const char *header_name, const char *header_value) {

    req->headers = (Sc_Header *) realloc(req->headers,
        (req->header_count+1)*sizeof(Sc_Header));
    
    if (req->headers == NULL) {
        printf("Cannot realloc memory.\n");
        return -1;
    }

    req->headers[req->header_count].name = strdup(header_name);
    req->headers[req->header_count].value = strdup(header_value);

    req->header_count++;

    return 0;
}


void sc_free_request(Sc_Request *req) {

    for (int i = 0; i < req->header_count; ++i) {
        free(req->headers[i].name);
        free(req->headers[i].value);
    }

    free(req->headers);
    free(req->body);
    free(req);
}
