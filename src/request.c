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
    char uri[255];
    // parse first line: Method, Request-URI and HTTP-Version
    sscanf(request, "%s %s %s", method, uri, req->version);

    // parse query params in uri
    char *q_param_start = strchr(uri, '?');
    if (q_param_start != NULL) {
        // query param exist
        size_t uri_len = q_param_start-uri;
        req->uri = strndup(uri, uri_len);

        char *p1, *p2;
        char *q_param = strtok_r(q_param_start+1, ",", &p1); // skip '?' char
        char *q_key, *q_value;
        while (q_param != NULL) {
            // parse key and values
            q_key = strtok_r(q_param, "=", &p2);
            q_value = p2;
            __sc_add_query(req, q_key, q_value);
            q_param = strtok_r(NULL, ",", &p1);
        }

    } else {
        req->uri = strdup(uri);
    }

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

    char *p3, *p4;
    char *header = strtok_r(headers, SC_CRLF, &p3);
    char *name, *value;
    int h_success;

    while (header != NULL) {

        // name field
        name = strtok_r(header, ":", &p4);
        name = sc_trim(name);

        // the rest -value field
        value = sc_trim(p4);

        // TODO: make field names case-insensitive

        h_success = __sc_add_header(req, name, value);

        free(name);
        free(value);

        if (h_success == -1) return NULL;

        header = strtok_r(NULL, SC_CRLF, &p3);
    }    

    free(headers);

    // save body
    req->body = strdup(body_p);

    // parse URI segments
    req->seg_count = 0;
    req->segments = (char **) malloc(SC_MAX_SEG*sizeof(char *));
    if (req->segments == NULL) return NULL;

    char *uri_cpy = strdup(req->uri);
    const char *seg;

    seg = strtok(uri_cpy, "/");

    while (seg != NULL) {
        req->segments[req->seg_count] = (char *) malloc((strlen(seg)+1)*sizeof(char));
        if (req->segments[req->seg_count] == NULL) {
            free(uri_cpy);
            return NULL;
        }
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
    
    if (req->headers == NULL) return -1;

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


void __sc_add_query(Sc_Request *req, const char *query_key, const char *query_value) {

    req->query = (Sc_Query *) realloc(req->query,
        (req->query_count+1)*sizeof(Sc_Query));
    
    if (req->query == NULL) {
        printf("Cannot realloc memory.\n");
    }

    req->query[req->query_count].key = strdup(query_key);
    req->query[req->query_count].value = strdup(query_value);

    req->query_count++;
}


int sc_has_query(Sc_Request *req, const char *query_key) {

    for (int i = 0; i < req->query_count; ++i) {
        if (strcmp(req->query[i].key, query_key) == 0) {
            return 1;
        }
    }

    return 0;
}


char *sc_get_query(Sc_Request *req, const char *query_key) {

    for (int i = 0; i < req->query_count; ++i) {
        if (strcmp(req->query[i].key, query_key) == 0) {
            // return copy of original value
            return strdup(req->query[i].value);
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

    for (int i = 0; i < req->query_count; ++i) {
        free(req->query[i].key);
        free(req->query[i].value);
    }

    free(req->headers);
    free(req->segments);
    free(req->params);
    free(req->query);
    free(req->uri);
    free(req->method);
    free(req->body);
    free(req);
}
