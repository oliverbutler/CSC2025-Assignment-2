/*
 * Replace the following string of 0s with your student number
 * 180121816
 */
#include "fsloglib.h"
#include <errno.h>
#include <lib.h>  // provides _syscall and message

/*
 * TODO: implement initfslog
 * see fsloglib.h for specification of initfslog
 */
void initfslog(void) {
    message m;
    m.m1_i1 = (int)FSOP_NONE;

    return _syscall(VFS_PROC_NR, STARTFSLOG, &m);
}

/* startfslog: implemented, do NOT change */
int startfslog(unsigned short ops2log) {
    if (ops2log < FSOP_NONE || ops2log > FSOP_ALL) {
        errno = EINVAL;
        return -1;
    }

    message m;
    m.m1_i1 = (int)ops2log;

    return _syscall(VFS_PROC_NR, STARTFSLOG, &m);
}

/*
 * TODO: implement stopfslog
 * see fsloglib.h for specification of stopfslog
 */
int stopfslog(unsigned short ops2stoplog) {
    if (ops2stoplog < FSOP_NONE || ops2stoplog > FSOP_ALL) {
        errno = EINVAL;
        return -1;
    }

    message m;
    m.m1_i1 = (int)ops2stoplog;

    return _syscall(VFS_PROC_NR, STOPFSLOG, &m);
}

/*
 * TODO: implement getfsloginf
 * see fsloglib.h for specification of initfslog
 */
int getfsloginf(struct fsloginf *fsloginf) {
    if (!fsloginf) {
        errno = EINVAL;
        return -1;
    }

    message m;
    m.m1_p1 = &fsloginf;

    return _syscall(VFS_PROC_NR, GETFSLOGIF, &m);
}

/*
 * TODO: implement getfslog
 * see fsloglib.h for specification of getfslog
 */
int getfslog(struct fsloginf *fsloginf, struct fslogrec fslog[]) {
    if (!fsloginf || !fslog) {
        errno = EINVAL;
        return -1;
    }

    message m;
    m.m1_p1 = &fsloginf;
    m.m1_p2 = &fslog;

    return _syscall(VFS_PROC_NR, GETFSLOGIF, &m);
}
