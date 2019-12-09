#ifndef __FSLOG_TESTUTIL_H__
#define __FSLOG_TESTUTIL_H__
/* functions for generating test workload for logs */
void create_log_workload(int file_nr);
void open_test_files(int path_len, char *path_fmt, int fildes[], int file_nr);
void read_test_files(int fildes[], int file_nr);
void write_test_files(int fildes[], int file_nr);
void close_test_files(int fildes[], int file_nr);
void unlink_test_files(int path_len, char *path_fmt, int tfd[], int file_nr);

/* functions to output log inf. and log */
void print_fslog(struct fsloginf *fsloginf, struct fslogrec fslog[]);
void print_fsloginf(struct fsloginf *fsloginf);
#endif

