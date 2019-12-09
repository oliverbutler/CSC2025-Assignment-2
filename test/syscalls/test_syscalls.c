#include <lib.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include "../util/fslog_testutil.h" 

#define TEST_FILE_NR 40     /* the number of files for workload generation */
#define OPS_LOGGING   6     /* the number of ops that will be logged */
#define TEST_FILE_OPS TEST_FILE_NR * OPS_LOGGING

/* test functions */
void test_do_startfslog();
void test_do_stopfslog();
void test_do_getfsloginf();
void test_do_getfslog();

/* wrappers for system calls used by test functions */
int do_startfslog_sc(unsigned short ops2log);
int do_stopfslog_sc(unsigned short ops2stoplog);
int do_getfsloginf_sc(struct fsloginf *fsloginf);
int do_getfslog_sc(struct fsloginf *fsloginf, struct fslogrec *fslog);


void init_fslog();

void assert_fsloginf(struct fsloginf *fsloginf, int exp_start, int exp_len,
    int exp_ops2log, int (*len_cmp)(int, int));
int cmp_eq(int act, int exp) { return act == exp; }
int cmp_ge(int act, int exp_min) { return act >= exp_min; }
int cmp_tr(int act, int exp) { return 1; }

void test_invalid_start(int ops2log, int exp_start, int exp_len,
    int exp_ops2log);
void test_valid_start(int ops2log, int exp_start, int exp_len, int exp_ops2log);
void test_invalid_stop(int ops2stoplog, int exp_start, int exp_len,
    int exp_ops2log);
void test_valid_stop(int ops2stoplog);
void test_invalid_getfslog(struct fsloginf *fsloginf, struct fslogrec *fslog);
void test_valid_getfslog();

void print_opcode_help(char *opstr, char *opdesc, int opcode);

int main(int argc, char **argv) {
    printf("---------------------------------------------------\n");
    init_fslog();
    test_do_startfslog();
    printf("---------------------------------------------------\n");
    init_fslog();
    test_do_stopfslog();
    printf("---------------------------------------------------\n");
    init_fslog();
    test_do_getfsloginf();    
    printf("---------------------------------------------------\n");
    init_fslog();
    test_do_getfslog();    
    printf("---------------------------------------------------\n");
}

void test_do_startfslog() {
    printf("TESTING DO_STARTFSLOG\n");
    
    test_invalid_start(FSOP_NONE - 1, 0, 0, 0);
    test_invalid_start(FSOP_ALL + 1, 0, 0, 0);
    test_valid_start(FSOP_NONE, 0, 0, 0);
    test_valid_start(FSOP_OPEN, 0, 0, FSOP_OPEN);
    test_valid_start(FSOP_ALL, 0, 0, FSOP_ALL);
}

void test_do_stopfslog() {
    printf("TESTING DO_STOPFSLOG\n");
    
    test_invalid_stop(FSOP_NONE - 1, 0, 0, 0);
    test_invalid_stop(FSOP_ALL + 1, 0, 0, 0);
    test_valid_stop(FSOP_ALL);
    test_valid_stop(FSOP_OPEN);
    test_valid_stop(FSOP_NONE);
}

void test_do_getfsloginf() {
    printf("TESTING DO_GETFSLOGINF\n");
    struct fsloginf fsloginf;

    init_fslog();
    
    printf("test invalid fsloginf == NULL ...\n");
    int r = do_getfsloginf_sc(NULL);
    
    assert(r == -1 && errno == EINVAL);
    
    printf("... success\n");

    printf("test valid fsloginf (before and after logging all) ...\n");
    
    r = do_startfslog_sc(FSOP_ALL);
    
    assert(!r);
    
    create_log_workload(TEST_FILE_NR);
    
    int (*cmp)(int, int) = cmp_eq;
    int exp_len = NR_FSLOGREC;
    
    if (TEST_FILE_OPS < NR_FSLOGREC) {
        cmp = cmp_ge;
        exp_len = TEST_FILE_OPS;
    }
    
    assert_fsloginf(&fsloginf, -1, exp_len, FSOP_ALL, cmp);
         
    print_fsloginf(&fsloginf);

    r = do_stopfslog_sc(FSOP_ALL);
    
    assert(!r);
    
    assert_fsloginf(&fsloginf, -1, exp_len, FSOP_NONE, cmp);
             
    print_fsloginf(&fsloginf);

    printf("... success\n");
}


void test_do_getfslog() {
    printf("TESTING DO_GETFSLOG\n");
    struct fsloginf fsloginf;
    struct fslogrec fslog[NR_FSLOGREC];

    init_fslog();
    
    test_invalid_getfslog(NULL, fslog);
    test_invalid_getfslog(&fsloginf, NULL);
    test_invalid_getfslog(NULL, NULL);
    
    test_valid_getfslog();
}

void assert_fsloginf(struct fsloginf *fsloginf, int exp_start, int exp_len, 
    int exp_ops2log, int (*len_cmp)(int, int)) {
    int r = do_getfsloginf_sc(fsloginf);
    
    assert(!r);
    if (exp_start > -1) assert(fsloginf->start == exp_start);
    assert(len_cmp(fsloginf->len, exp_len));
    assert(fsloginf->ops2log == exp_ops2log);
}

void init_fslog() {
    int r = do_stopfslog_sc(FSOP_ALL);    
    assert(!r);
    
    r = do_startfslog_sc(FSOP_NONE);
    assert(!r);
    
    struct fsloginf fsloginf;
    
    assert_fsloginf(&fsloginf, 0, 0, FSOP_NONE, cmp_eq);
}

void test_invalid_start(int ops2log, int exp_start, int exp_len, 
    int exp_ops2log) {
    printf("test invalid ops2log == %d ...\n", ops2log);
    
    init_fslog();

    errno = 0;
       
    int r = do_startfslog_sc(ops2log);
    
    assert(r == -1 && errno == EINVAL);
    
    struct fsloginf fsloginf;
    
    assert_fsloginf(&fsloginf, exp_start, exp_len, exp_ops2log, cmp_eq);

    printf("... success\n");
}

void test_valid_start(int ops2log, int exp_start, int exp_len, 
    int exp_ops2log) {
    printf("test valid ops2log == %d ...\n", ops2log);
    
    init_fslog();
    
    errno = 0;
    
    int r = do_startfslog_sc(ops2log);
    
    assert(!r && !errno);
    
    struct fsloginf fsloginf;
    
    assert_fsloginf(&fsloginf, exp_start, exp_len, exp_ops2log, cmp_ge);

    printf("... success\n");
}

void test_invalid_stop(int ops2stoplog, int exp_start, int exp_len, 
    int exp_ops2log) {
    printf("test invalid ops2stoplog == %d ...\n", ops2stoplog);
    
    init_fslog();
    
    errno = 0;
    
    int r = do_stopfslog_sc(ops2stoplog);
        
    assert(r == -1 && errno == EINVAL);
    
    struct fsloginf fsloginf;
    
    assert_fsloginf(&fsloginf, exp_start, exp_len, exp_ops2log, cmp_eq);

    printf("... success\n");
}

void test_valid_stop(int ops2stoplog) {
    printf("test valid ops2stoplog == %d ...\n", ops2stoplog);
    struct fsloginf fsloginf;
    
    init_fslog();

    errno = 0;
    
    int r = do_startfslog_sc(FSOP_ALL);
    
    assert(!r && !errno);
    
    assert_fsloginf(&fsloginf, -1, 0, FSOP_ALL, cmp_tr);

    errno = 0;
    
    r = do_stopfslog_sc(ops2stoplog);
    
    assert(!r && !errno);
    
    assert_fsloginf(&fsloginf, -1, 0, FSOP_ALL - ops2stoplog, cmp_tr);
    
    printf("... success\n");
}

void test_invalid_getfslog(struct fsloginf *fsloginf, struct fslogrec *fslog) {
    char *fsloginf_s = fsloginf ? "fsloginf" : "NULL";
    char *fslog_s = fslog ? "fslog" : "NULL";
    
    printf("test invalid (fsloginf, fslog) == (%s, %s) ...\n", fsloginf_s, 
        fslog_s);
        
    int r = do_getfslog_sc(fsloginf, fslog);
    
    assert(r == -1 && errno == EINVAL);
    
    printf("... success\n");
}
void test_valid_getfslog() {
    struct fsloginf fsloginf;
    struct fslogrec fslog[NR_FSLOGREC];

    printf("test valid (fsloginf, fslog) ...\n");
    
    printf("Filesystem ops to log are selected by bitwise combinations of \n");
    printf("one or more of the following opcodes:\n");
    print_opcode_help("FSOP_NONE", "none", FSOP_NONE);
    print_opcode_help("FSOP_OPEN", "open", FSOP_OPEN);
    print_opcode_help("FSOP_CLOSE", "close", FSOP_CLOSE);
    print_opcode_help("FSOP_READ", "read", FSOP_READ);
    print_opcode_help("FSOP_WRITE", "write", FSOP_WRITE);
    print_opcode_help("FSOP_CREAT", "create", FSOP_CREAT);
    print_opcode_help("FSOP_UNLNK", "unlink/delete", FSOP_UNLNK);
    print_opcode_help("FSOP_ALL_WOERR", "all without errors", FSOP_ALL_WOERR);
    print_opcode_help("FSOP_ERR", "errors when ORed with others", FSOP_ERR);
    print_opcode_help("FSOP_ALL", "all", FSOP_ALL);

    printf("(see unistd.h for definition of FSOP_* constants)\n");
    printf("Enter a valid combination of opcode(s) to log ");
    printf("(anything else to abort): ");

    int ops2log = 0;
    char line[MAX_INPUT];
    
    char *l = fgets(line, MAX_INPUT, stdin);
    
    if (!l || (sscanf(l, "%d", &ops2log) != 1) || ops2log < FSOP_NONE || 
        ops2log > FSOP_ALL) {
        printf("... test aborted\n");
        return;
    }
    
    int r = do_startfslog_sc(ops2log);
    
    assert(!r);
    
    create_log_workload(TEST_FILE_NR);
    
    r = do_stopfslog_sc(FSOP_ALL);
    
    assert(!r);
    
    r = do_getfslog_sc(&fsloginf, fslog);
    
    assert(!r);

    print_fslog(&fsloginf, fslog);
    
    printf("... success\n");
}


void print_opcode_help(char *opstr, char *opdesc, int opcode) {
    printf("  %-14s : %-28s : %0#4o oct or %3d dec\n", opstr, opdesc, 
        opcode, opcode);
}

int do_startfslog_sc(unsigned short ops2log) {
    message m;
    
    m.m1_i1 = ops2log;

    return _syscall(VFS_PROC_NR, STARTFSLOG, &m);
}

int do_stopfslog_sc(unsigned short ops2stoplog) {
    message m;
    
    m.m1_i1 = ops2stoplog;

    return _syscall(VFS_PROC_NR, STOPFSLOG, &m);
}

int do_getfsloginf_sc(struct fsloginf *fsloginf) {
    message m;
    
    m.m1_p1 = (char *) fsloginf;

    return _syscall(VFS_PROC_NR, GETFSLOGINF, &m);
}

int do_getfslog_sc(struct fsloginf *fsloginf, struct fslogrec fslog[]) {
    message m;
    
    m.m1_p1 = (char *) fsloginf;
    m.m1_p2 = (char *) fslog;

    return _syscall(VFS_PROC_NR, GETFSLOG, &m);
}
