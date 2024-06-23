#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/helpers.h"


char *sc_trim(char *str) {

    // use free() after

    size_t len = strlen(str);

    char *trimmed = (char *) malloc((len+1)*sizeof(char));
    memset(trimmed, '\0', len+1);

    int j = 0;

    for (int i = 0; i < len; ++i) {
        if (str[i] != ' ' &&
            str[i] != '\t' &&
            str[i] != '\n' &&
            str[i] != '\r' &&
            str[i] != '\v') {

                trimmed[j] = str[i];
                j++;
            }
    }

    return trimmed;
}