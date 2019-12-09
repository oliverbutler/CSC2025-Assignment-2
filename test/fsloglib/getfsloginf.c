#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "../../lib/fsloglib.h"
#include "../util/fslog_testutil.h" // provides print_fsloginf

int main(int argc, char **argv) {
    struct fsloginf fsloginf;
    
    int r = getfsloginf(&fsloginf);

    if (!r) {
        print_fsloginf(&fsloginf);
    } else {
        perror("getfsloginf error: ");
    }

    return r;
}
