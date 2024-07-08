#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

#include "../include/response.h"


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
    if (text == NULL) return NULL;
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
    if (headers_as_text == NULL) return NULL;

    size_t len = 13;
    len += strlen(res->status_message) + 2; // +2 for CRLF
    len += strlen(headers_as_text) + 2; // +2 for CRLF
    len += res->body_len;

    res->total_len = len;
    
    char *response = (char *) malloc(len*sizeof(char));
    if (response == NULL) return NULL;
    snprintf(response, len, "%s %d %s\r\n%s\r\n",
        res->version, res->status_code,
        res->status_message, headers_as_text);

    if (res->body_len != 0) {
        memcpy(response+len-res->body_len, res->body, res->body_len);
    }

    return response;
}


void sc_set_status(Sc_Response *res, int status_code, const char *status_message){

    res->status_code = status_code;
    res->status_message = strdup(status_message);

}


int sc_res_has_header(Sc_Response *res, const char *header_name) {

    for (int i = 0; i < res->header_count; ++i) {
        if (strcmp(res->headers[i].name, header_name) == 0) {
            return 1;
        }
    }

    return 0;
}


char *sc_res_get_header(Sc_Response *res, const char *header_name) {

    for (int i = 0; i < res->header_count; ++i) {
        if (strcmp(res->headers[i].name, header_name) == 0) {
            // return copy of original value
            return strdup(res->headers[i].value);
        }
    }

    return NULL;
}


void sc_set_header(Sc_Response *res, const char *header_name, const char *header_value) {

    // create if not exists else update existing

    for (int i = 0; i < res->header_count; ++i) {
        if (strcmp(res->headers[i].name, header_name) == 0) {
            // update existing
            free(res->headers[i].value);
            res->headers[i].value = strdup(header_value);
            return;
        }
    }

    // create new

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


void sc_set_body(Sc_Response *res, const char *data) {

    if (data == NULL) return;

    if (res->body) {
        free(res->body);
    }

    // save body as binary to make it compatible with binary files

    size_t len = strlen(data);

    res->body = (char *) malloc(len);

    if (res->body) {
        memcpy(res->body, data, len);
        res->body_len = len;
        res->is_body_set = 1;
    }
}


void sc_append_body(Sc_Response *res, const char *data) {

    if (!(res->is_body_set)) {
        sc_set_body(res, data);
        return;
    }

    if (data == NULL) return;

    // append body as binary to make it compatible with binary files

    size_t prev_len = res->body_len;
    size_t data_len = strlen(data);
    size_t new_len = prev_len+data_len;

    char *new_body = (char *) realloc(res->body, new_len*sizeof(char));

    if (new_body == NULL) {
        printf("Cannot realloc memory.");
        return;
    }

    memcpy(new_body+prev_len, data, data_len);

    res->body = new_body;
    res->body_len = new_len;
}


int sc_set_body_file(Sc_Response *res, const char *filename) {

    if (res->body) {
        free(res->body);
    }

    // get absolute path of file
    char *abs_path = realpath(filename, NULL);

    // get length of file
    struct stat fst;

    if (stat(abs_path, &fst) != 0) {
        perror("Cannot get file stat");
    }

    long f_size = fst.st_size;

    // get file
    FILE *f = fopen(abs_path, "rb");

    if (f == NULL) {
        perror("Cannot open file");
        return 0;
    }

    res->body = malloc(f_size);
    // read file
    size_t buff_read = fread(res->body, 1, f_size, f);
    if (f_size != buff_read) {
        perror("Cannot read file");
        free(res->body);
        free(abs_path);
        fclose(f);
        return 0;
    }

    fclose(f);
    res->body_len = f_size;

    // set content type header
    const char *mime_type = sc_get_mime_type(abs_path);
    sc_set_header(res, "Content-Type", mime_type);

    // set content length header
    char content_length[32];
    snprintf(content_length, sizeof(content_length), "%ld", f_size);
    sc_set_header(res, "Content-Length", content_length);

    free(abs_path);

    return 1;
}


const char *sc_get_mime_type(const char *path) {

    const char *ext = strrchr(path, '.');

    // thanks to GPT <333
    if (ext) {
        if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
        if (strcmp(ext, ".css") == 0) return "text/css";
        if (strcmp(ext, ".js") == 0) return "application/javascript";
        if (strcmp(ext, ".json") == 0) return "application/json";
        if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(ext, ".png") == 0) return "image/png";
        if (strcmp(ext, ".gif") == 0) return "image/gif";
        if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
        if (strcmp(ext, ".ico") == 0) return "image/x-icon";
        if (strcmp(ext, ".pdf") == 0) return "application/pdf";
        if (strcmp(ext, ".txt") == 0) return "text/plain";
        if (strcmp(ext, ".xml") == 0) return "application/xml";
        if (strcmp(ext, ".wasm") == 0) return "application/wasm";
        if (strcmp(ext, ".mp4") == 0) return "video/mp4";
        if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
        if (strcmp(ext, ".wav") == 0) return "audio/wav";
        if (strcmp(ext, ".ogg") == 0) return "audio/ogg";
        if (strcmp(ext, ".webm") == 0) return "video/webm";
        if (strcmp(ext, ".zip") == 0) return "application/zip";
        if (strcmp(ext, ".rar") == 0) return "application/vnd.rar";
        if (strcmp(ext, ".tar") == 0) return "application/x-tar";
        if (strcmp(ext, ".gz") == 0) return "application/gzip";
        if (strcmp(ext, ".bmp") == 0) return "image/bmp";
        if (strcmp(ext, ".webp") == 0) return "image/webp";
        if (strcmp(ext, ".flac") == 0) return "audio/flac";
        if (strcmp(ext, ".midi") == 0 || strcmp(ext, ".mid") == 0) return "audio/midi";
        if (strcmp(ext, ".rtf") == 0) return "application/rtf";
        if (strcmp(ext, ".wav") == 0) return "audio/wav";
        if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
        if (strcmp(ext, ".mov") == 0) return "video/quicktime";
        if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
    }
    return "text/plain";
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
