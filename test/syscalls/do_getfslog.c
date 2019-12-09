#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <lib.h>
#include "../util/fslog_testutil.h" // provides print_fslog

int main(int argc, char **argv) {
    struct fsloginf fsloginf;
    struct fslogrec fslog[NR_FSLOGREC];
    
    message m;
    
    m.m1_p1 = (char *) &fsloginf;
    m.m1_p2 = (char *) fslog;
    
    int r = _syscall(VFS_PROC_NR, GETFSLOG, &m);
    
    if (!r) {
        print_fslog(&fsloginf, fslog);
    } else {
        perror("do_getfslog error: ");
    }

    return r;
}
