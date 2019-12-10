/*
 * Replace the following string of 0s with your student number
 * 180121816
 */
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include "fs.h"             // for glo.h:  fp, mp, call_nr, who_p etc.
#include "fproc.h"          // provides definition of fproc
#include "fslog.h"

#define INVALID_ARG -22 /* gets converted to EINVAL by syscall */

#define EVENTS_FD_NR 3                /* the file descriptor number of \
                                       * the events file to be ignored */
static char* EVENTS_PATH = "events";  /* the events file path to be ignored */
static char* DEVMAN_NAME = "devmand"; /* device manager process name to be
                                         * ignored
                                         */

/* fsloginf is the log meta-information struct, initialised to:
 * start: 0, len: 0, ops2log: FSOP_NONE
 * see unistd.h for struct fsloginf definition
 */
static struct fsloginf fsloginf = {0, 0, FSOP_NONE};

/* fslog is the array used for the in-memory log - see unistd.h for struct 
 * fslogrec definition
 */
static struct fslogrec fslog[NR_FSLOGREC];

/*
 * do_startfslog: implemented, do NOT change
 * 
 * Description:
 * Start or restart logging of filesystem operations specified by a combination
 * of opcodes passed as a message field.
 * This system call will reset the start field of the filesystem log 
 * meta-information to 0 and the length field to 0.
 * An ops to log parameter is passed as a message field. This is ORed with the 
 * meta-information ops2log field and if the result is > FSOP_NONE, logging will
 * be on for the specified filesystem operation(s). If the resulting value of 
 * the ops2log field is FSOP_NONE then no operations will be logged. If the 
 * resulting value is FSOP_ALL then all FSOP operations and errors will be
 * logged. FSOP values are defined in unistd.h.
 *
 * Expected incoming message fields:
 * m_in.m1_i1 - an ORed combination of FSOP values that must be in the range
 *      FSOP_NONE to FSOP_ALL (inclusive). If m_in.m1_i1 is FSOP_NONE then 
 *      the call has no effect on operation logging. If m_in.m1_i1 is
 *      FSOP_ALL then logging will be on for all operations and errors.
 *
 * Return:
 * OK if the call succeeds
 * INVALID_ARG if m_in.m1_i1 is less than FSOP_NONE or greater than FSOP_ALL
 */
int do_startfslog() {
    if (m_in.m1_i1 < FSOP_NONE || m_in.m1_i1 > FSOP_ALL)
        return INVALID_ARG;

    fsloginf.start = 0;
    fsloginf.len = 0;
    fsloginf.ops2log = fsloginf.ops2log | m_in.m1_i1;

    return OK;
}

/*
 * TODO: IMPLEMENT do_stopfslog
 * 
 * Description:
 * Stop logging of filesystem operations specified by a combination of opcodes
 * passed as a message field.
 * The ops to stop logging are passed as the value of a message field. Its
 * complement is ANDed with the meta-information ops2log field to stop logging
 * for the specified filesystem operations. If the resulting value of the
 * ops2log field is FSOP_NONE, logging will stop for all operations. If the 
 * resulting value is FSOP_ALL then logging will continue for all operations and
 * errors will be logged. FSOP values are defined in unistd.h.
 * This system call has no effect on the start or length fields of the 
 * filesystem log meta-information
 *
 * Expected incoming message fields:
 * m_in.m1_i1 - an ORed combination of FSOP values that must be in the range
 *      FSOP_NONE to FSOP_ALL (inclusive). If m_in.m1_i1 is FSOP_NONE then 
 *      the call has no effect on operation logging. If m_in.m1_i1 is
 *      FSOP_ALL then all logging is stopped.
 *
 * Return:
 * OK if the call succeeds
 * INVALID_ARG if m_in.m1_i1 is less than FSOP_NONE or greater than FSOP_ALL
 */
int do_stopfslog() {

    if (m_in.m1_i1 < FSOP_NONE || m_in.m1_i1 > FSOP_ALL)
        return INVALID_ARG;
    
    fsloginf.ops2log = fsloginf.ops2log & ~m_in.m1_i1;

    return OK;
}

/*
 * TODO: IMPLEMENT do_getfsloginf
 * 
 * Description:
 * Get the log meta-information - a fsloginf struct with fields for the 
 * start position of the log, the length of the log (i.e. the number of valid
 * entries in the log array) and the value of the ops2log selector. The 
 * fsloginf struct is defined in unistd.h. The struct is returned by copying to 
 * an address provided as a message field.
 *
 * Expected incoming message fields:
 * m_in.m1_p1 - a pointer field that is the address of a fsloginf struct in the
 *      address space of the calling user process. This is the address that the
 *      system call will copy its fsloginf struct to.
 *
 * Return:
 * OK if the call succeeds
 * In the case of failure:
 *      INVALID_ARG if m_in.m1_p1 is NULL
 *      An error status related to copying between process address spaces
 *      using sysvircopy (one of EDOM, EFAULT, EPERM, EINVAL) 
 *
 see: https://wiki.minix3.org/doku.php?id=developersguide:kernelapi#sys_vircopy
 */
int do_getfsloginf() {
    if (m_in.m1_p1 == NULL)
        return INVALID_ARG;

    vir_bytes pointer_src = (vir_bytes) & fsloginf;
    vir_bytes pointer_dst = (vir_bytes)m_in.m1_p1;

    int r = sys_vircopy(SELF, pointer_src, who_e, pointer_dst, sizeof(fsloginf));

    return r;
}

/*
 * TODO: IMPLEMENT do_getfslog
 * 
 * Description:
 * Get the log meta-information (as provided by do_getfsloginf) and the
 * in-memory log - the circular buffer of filesystem operation entries.
 * The information obtained is a fsloginf struct (as in do_getfsloginf) and 
 * an array of fslogrec structs. Both struct types are defined in unistd.h.
 * The information is returned by copying to addresses provided as message
 * fields.
 *
 * Expected incoming message fields:
 * m_in.m1_p1 - a pointer field that is the address of a fsloginf struct in the
 *      address space of the calling user process. This is the address that the
 *      system call will copy the system's fsloginf struct to.
 * m_in.m1_p2 - a pointer field that is the address of an array of fslogrec
 *      structs in the address space of the calling user process. This is
 *      the address that the system call will copy the system's fslog to.
 *
 * Return:
 * OK if the call succeeds
 * In the case of failure:
 *      INVALID_ARG if either m_in.m1_p1 or m_in.m1_p2 is NULL:
 *      An error status related to copying between process address spaces
 *      using sysvircopy (one of EDOM, EFAULT, EPERM, EINVAL) 
 *
 see: https://wiki.minix3.org/doku.php?id=developersguide:kernelapi#sys_vircopy
 */
int do_getfslog() {
    if (m_in.m1_p1 == NULL || m_in.m1_p2 == NULL)
        return INVALID_ARG;

    vir_bytes pointer_1 = (vir_bytes)m_in.m1_p1;
    vir_bytes pointer_2 = (vir_bytes)m_in.m1_p2;

    int r1 = sys_vircopy(SELF, (vir_bytes)&fsloginf, who_e, pointer_1, sizeof(fsloginf));
    int r2 = sys_vircopy(SELF, (vir_bytes)&fslog, who_e, pointer_2, sizeof(fslog) * NR_FSLOGREC);

    if (r1 != OK)
        return r1;
    if (r2 != OK)
        return r2;
    return OK;
}

/* 
 * logfserr: implemented, do NOT change
 * see fslog.h for specification of this logging function 
 */
void logfserr(int opcode, int result, char* path) {
    if (fsloginf.ops2log & FSOP_ERR)
        logfsop(opcode, result, path, UNKNOWN_FD_NR, UNKNOWN_MODE,
                UNKNOWN_SIZE);
}

/* 
 * logfserr_nopath: implemented, do NOT change
 * see fslog.h for specification of this logging function
 */
void logfserr_nopath(int opcode, int result) {
    logfserr(opcode, result, NULL);
}

/* 
 * logfsop:
 * TODO: COMPLETE the implementation of logfsop - see TODO comment in body 
 * of the function.
 *
 * see fslog.h for specification of this logging function 
 */
void logfsop(int opcode, int result, char* path, int fd_nr, mode_t omode,
             off_t size) {
    if (path && fd_nr == EVENTS_FD_NR && strncmp(EVENTS_PATH, path, PATH_MAX) == 0)
        return;  // system events operation that is ignored

    if (fp->fp_name && strncmp(fp->fp_name, DEVMAN_NAME, PROC_NAME_LEN) == 0)
        return;  // ignore device manager daemon events

    if (fsloginf.ops2log & opcode) {
        int next = (fsloginf.start + fsloginf.len) % NR_FSLOGREC;

        fslog[next].timestamp = time(NULL);
        fslog[next].opcode = opcode;
        fslog[next].result = result;
        fslog[next].fd_nr = fd_nr;
        fslog[next].mode = omode;
        fslog[next].size = size;

        /* 
         * TODO: update the following fslogrec field assignments of path,
         * cp_pid and cp_name by setting them to their correct values.
         *
         * Hints:
         * - strings must have their data copied to the corresponding struct
         * field and copying must be restricted to the appropriate length for 
         * the relevant string
         * - calling process information can be obtained via the global fp
         * pointer to the struct fproc entry for the calling the process in the
         * filesystem process table
         * - you must allow for the fact that data may not be known. For
         * example, the calling process may not be known, its name may not 
         * be known and the name of the file being operated on may not be known.
         * 
         */
        if (fp->fp_pid)
            fslog[next].cp_pid = fp->fp_pid;
        else
            fslog[next].cp_pid = UNKNOWN_CP_PID;

        if (fp->fp_name) {
            int len = strnlen(fp->fp_name, PROC_NAME_LEN);
            strncpy(fslog[next].cp_name, fp->fp_name, sizeof(fp->fp_name));
            fslog[next].path[len] = '\0';
        } else
            fslog[next].cp_name[0] = '\0';

        if (path) {
            int len = strnlen(path, PATH_MAX);
            strncpy(fslog[next].path, path, PATH_MAX);
            fslog[next].path[len] = '\0';
        } else
            fslog[next].path[0] = '\0';

        /* do NOT change the following lines */
        if (fsloginf.len == NR_FSLOGREC)
            fsloginf.start = (fsloginf.start + 1) % NR_FSLOGREC;
        else
            fsloginf.len++;
    }
}

/* 
 * logfsop_nopath: implemented, do NOT change
 * see fslog.h for specification of this logging function 
 */
void logfsop_nopath(int opcode, int result, int fd_nr, mode_t omode,
                    off_t size) {
    logfsop(opcode, result, NULL, fd_nr, omode, size);
}
