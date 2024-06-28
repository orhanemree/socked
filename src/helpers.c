#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/helpers.h"


char *sc_trim(char *str) {

    // use free after

    if (str == NULL) return NULL;

    size_t len = strlen(str);
    const char *start = str;
    const char *end = str + len - 1;

    while (isspace(*start)) {
        start++;
    }

    while (end > start && isspace(*end)) {
        end--;
    }

    size_t trimmed_len = (end - start) + 1;

    char *trimmed_str = (char *) malloc((trimmed_len + 1) * sizeof(char));

    // TODO: add error handle

    strncpy(trimmed_str, start, trimmed_len);
    trimmed_str[trimmed_len] = '\0';

    return trimmed_str;
}