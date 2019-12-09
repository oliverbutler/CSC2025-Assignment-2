#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "../../lib/fsloglib.h"
#include "../util/fslog_testutil.h" // provides print_fslog

int main(int argc, char **argv) {
    struct fsloginf fsloginf;
    struct fslogrec fslog[NR_FSLOGREC];
    
    int r = getfslog(&fsloginf, fslog);
    
    if (!r) {
        print_fslog(&fsloginf, fslog);
    } else {
        perror("getfslog error: ");
    }

    return r;
}
