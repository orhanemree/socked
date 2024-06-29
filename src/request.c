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
    if (req == NULL) return NULL;
    memset(req, 0, sizeof(Sc_Request));
    
    char method[9];
    // parse first line: Method, Request-URI and HTTP-Version
    sscanf(request, "%s %s %s", method, req->uri, req->version);

    req->method = strdup(method);

    if (strcmp(method, "OPTIONS") == 0) {
        req->imethod = SC_OPTIONS;
    } else if (strcmp(method, "GET") == 0) {
        req->imethod = SC_GET;
    } else if (strcmp(method, "HEAD") == 0) {
        req->imethod = SC_HEAD;
    } else if (strcmp(method, "POST") == 0) {
        req->imethod = SC_POST;
    } else if (strcmp(method, "PUT") == 0) {
        req->imethod = SC_PUT;
    } else if (strcmp(method, "DELETE") == 0) {
        req->imethod = SC_DELETE;
    } else if (strcmp(method, "TRACE") == 0) {
        req->imethod = SC_TRACE;
    } else if (strcmp(method, "CONNECT") == 0) {
        req->imethod = SC_CONNECT;
    } else {
        req->imethod = SC_UNK;
    }

    // TODO: return 414 if uri > 255

    // split headers and body
    char *headers_p = strstr(request, SC_CRLF)+2;
    char *body_p = strstr(headers_p, (SC_CRLF SC_CRLF));

    size_t headers_len = body_p - headers_p;
    char *headers = (char *) malloc((headers_len+1)*sizeof(char));
    if (headers == NULL) return NULL;

    strncpy(headers, headers_p, headers_len);
    headers[headers_len] = '\0';

    // iterate 2 CRLFs
    body_p += 4;

    // parse headers
    req->header_count = 0;

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

    // parse URI segments
    req->seg_count = 0;
    req->segments = (char **) malloc(SC_MAX_SEG*sizeof(char *));

    char *uri_cpy = strdup(req->uri);
    const char *seg;

    seg = strtok(uri_cpy, "/");

    while (seg != NULL) {
        req->segments[req->seg_count] = (char *) malloc((strlen(seg)+1)*sizeof(char));
        req->segments[req->seg_count] = strdup(seg);

        seg = strtok(NULL, "/");
        req->seg_count++;
    }

    free(uri_cpy);

    // set up params
    req->param_count = 0;

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


int sc_req_has_header(Sc_Request *req, const char *header_name) {

    for (int i = 0; i < req->header_count; ++i) {
        if (strcmp(req->headers[i].name, header_name) == 0) {
            return 1;
        }
    }

    return 0;
}


char *sc_req_get_header(Sc_Request *req, const char *header_name) {

    for (int i = 0; i < req->header_count; ++i) {
        if (strcmp(req->headers[i].name, header_name) == 0) {
            // return copy of original value
            return strdup(req->headers[i].value);
        }
    }

    return NULL;
}


void __sc_add_param(Sc_Request *req, const char *param_key, const char *param_value) {

    req->params = (Sc_Param *) realloc(req->params,
        (req->param_count+1)*sizeof(Sc_Param));
    
    if (req->params == NULL) {
        printf("Cannot realloc memory.\n");
    }

    req->params[req->param_count].key = strdup(param_key);
    req->params[req->param_count].value = strdup(param_value);

    req->param_count++;
}


int sc_has_param(Sc_Request *req, const char *param_key) {

    for (int i = 0; i < req->param_count; ++i) {
        if (strcmp(req->params[i].key, param_key) == 0) {
            return 1;
        }
    }

    return 0;
}


char *sc_get_param(Sc_Request *req, const char *param_key) {

    for (int i = 0; i < req->param_count; ++i) {
        if (strcmp(req->params[i].key, param_key) == 0) {
            // return copy of original value
            return strdup(req->params[i].value);
        }
    }

    return NULL;
}


void sc_free_request(Sc_Request *req) {

    for (int i = 0; i < req->header_count; ++i) {
        free(req->headers[i].name);
        free(req->headers[i].value);
    }

    for (int i = 0; i < req->seg_count; ++i) {
        free(req->segments[i]);
    }

    for (int i = 0; i < req->param_count; ++i) {
        free(req->params[i].key);
        free(req->params[i].value);
    }

    free(req->headers);
    free(req->segments);
    free(req->params);
    free(req->method);
    free(req->body);
    free(req);
}
