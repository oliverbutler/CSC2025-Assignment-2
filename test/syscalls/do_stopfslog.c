#include <stdio.h>
#include <stdlib.h>
#include <lib.h>
#include <errno.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: %s <ops2stoplog>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    errno = 0;
    
    message m;
    
    m.m1_i1 = (unsigned short) atoi(argv[1]);
    
    int r = _syscall(VFS_PROC_NR, STOPFSLOG, &m);
    
    if (r)
        perror("do_stopfslog error: ");
        
    return r;
}
