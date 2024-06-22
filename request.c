typedef struct {
    char *name;
    char *value;
} Header;


typedef struct {
    char method[8];
    char uri[255];
    char version[8];
    Header *headers;
    size_t header_count;
    char *body;
} Req;
