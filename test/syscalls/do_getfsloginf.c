#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <lib.h>
#include "../util/fslog_testutil.h" // provides print_fsloginf

int main(int argc, char **argv) {
    struct fsloginf fsloginf;
    
    message m;
    
    m.m1_p1 = (char *) &fsloginf;

    int r = _syscall(VFS_PROC_NR, GETFSLOGINF, &m);

    if (!r) {
        print_fsloginf(&fsloginf);
    } else {
        perror("do_getfsloginf error: ");
    }

    return r;
}
