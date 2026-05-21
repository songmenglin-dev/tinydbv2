#include "pager.h"
#include <stdlib.h>
#include <stdio.h>

/* File manager module */

int file_init(void) {
    return 0;
}

Pager* pager_create_file(const char* path) {
    return pager_create(path);
}

Pager* pager_open_file(const char* path) {
    return pager_open(path);
}