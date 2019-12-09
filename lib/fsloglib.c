/*
 * Replace the following string of 0s with your student number
 * 000000000
 */
#include <lib.h>    // provides _syscall and message
#include <errno.h>
#include "fsloglib.h"

/*
 * TODO: implement initfslog
 * see fsloglib.h for specification of initfslog
 */
void initfslog(void) {
    return;
}

/* startfslog: implemented, do NOT change */
int startfslog(unsigned short ops2log) {
    if (ops2log < FSOP_NONE || ops2log > FSOP_ALL) {
        errno = EINVAL;
        return -1;
    }
    
    message m;
    m.m1_i1 = (int) ops2log;

    return _syscall(VFS_PROC_NR, STARTFSLOG, &m);
}

/*
 * TODO: implement stopfslog
 * see fsloglib.h for specification of stopfslog
 */
int stopfslog(unsigned short ops2stoplog) {
    return -1;
}

/*
 * TODO: implement getfsloginf
 * see fsloglib.h for specification of initfslog
 */
int getfsloginf(struct fsloginf *fsloginf) {
    return -1;
}

/*
 * TODO: implement getfslog
 * see fsloglib.h for specification of getfslog
 */
int getfslog(struct fsloginf *fsloginf, struct fslogrec fslog[]) {
    return -1;
}
