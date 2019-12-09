#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../../lib/fsloglib.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: %s <ops2stoplog>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int ops2stoplog = atoi(argv[1]);
    
    int r = stopfslog(ops2stoplog);
    
    if (r)
        perror("stopfslog error: ");
        
    return r;
}
