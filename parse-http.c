#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "helpers.c"
#include "request.c"

#define CRLF "\r\n"


int __add_header(Req *req, const char *header_name, const char *header_value) {

    req->headers = (Header *) realloc(req->headers,
        (req->header_count+1)*sizeof(Header));
    
    if (req->headers == NULL) {
        printf("Cannot realloc memory.\n");
        return -1;
    }

    req->headers[req->header_count].name = strdup(header_name);
    req->headers[req->header_count].value = strdup(header_value);

    req->header_count++;

    return 0;
}


Req *parse_http_request(char *request) {

    Req *req = (Req *) malloc(sizeof(Req));
    memset(req, 0, sizeof(Req));

    // parse first line: Method, Request-URI and HTTP-Version
    sscanf(request, "%s %s %s", req->method, req->uri, req->version);

    // TODO: return 405 if method not in list
    // TODO: return 414 if uri > 255

    // split headers and body
    char *headers_p = strstr(request, CRLF)+2;
    char *body_p = strstr(headers_p, (CRLF CRLF));

    size_t headers_len = body_p - headers_p;
    char *headers = (char *) malloc((headers_len+1)*sizeof(char));

    strncpy(headers, headers_p, headers_len);
    headers[headers_len] = '\0';

    // iterate 2 CRLFs
    body_p += 4;

    // parse headers
    char *p1, *p2;
    char *header = strtok_r(headers, CRLF, &p1);
    char *name, *value;

    while (header != NULL) {

        // name field
        name = strtok_r(header, ":", &p2);
        name = trim(name);

        // the rest -value field
        value = trim(p2);

        // TODO: make field names case-insensitive

        __add_header(req, name, value);

        free(name);
        free(value);

        header = strtok_r(NULL, CRLF, &p1);
    }    

    free(headers);

    // save body
    req->body = strdup(body_p);

    return req;
}


void free_req(Req *req) {

    for (int i = 0; i < req->header_count; ++i) {
        free(req->headers[i].name);
        free(req->headers[i].value);
    }

    free(req->headers);
    free(req);
}


// int main() {

    // char *request = "POST / HTTP/1.1\
    // \r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\nBody";

//     Req *req = parse_http_request(request);


//     printf("method: %s uri: %s version: %s\n", req->method, req->uri, req->version);

//     for (int i = 0; i < req->header_count; ++i) {
//         printf("%s: %s\n", req->headers[i].name, req->headers[i].value);
//     }

//     printf("%s\n", req->body);

//     free_req(req);

//     return 0;
// }