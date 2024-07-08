#ifndef SC_UTILS_H
#define SC_UTILS_H


#define SC_MAX_REQ 1024*8
#define SC_HTTP_VERSION "HTTP/1.1"
#define SC_MAX_SEG 10 // max segment count of path
#define SC_CRLF "\r\n"


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
    char *key; // Route Parameter key
    char *value; // Route Parameter value
} Sc_Param;


typedef struct {
    char *key; // Query Parameter key
    char *value; // Query Parameter value
} Sc_Query;


#endif // SC_UTILS_H
