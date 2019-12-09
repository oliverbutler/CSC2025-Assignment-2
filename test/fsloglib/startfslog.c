#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../../lib/fsloglib.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: %s <ops2log>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int ops2log = atoi(argv[1]);
    
    int r = startfslog(ops2log);
    
    if (r)
        perror("startfslog error: ");
        
    return r;
}
