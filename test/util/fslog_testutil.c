#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include "fslog_testutil.h"
#define DEBUG_ON 0
#define PATH_LEN 19
#define OUTPUT_LEN 14
#define FERR -1
#define FCLS -2

int set_path(char *path, int path_len, char *path_fmt, int nr);

char *DELIM = "---------------------------------------------------";

void print_fslog(struct fsloginf *fsloginf, struct fslogrec *fslog) {
    print_fsloginf(fsloginf);
    
    int entries = fsloginf->len;
    int i = fsloginf->start;
    
    printf("%s\n", DELIM);
    printf("Filesystem log entries\n");
    printf("%s\n", DELIM);
    printf(" idx | opcode | result |   fd |  mode    | cp_pid | cp_name, "
            "path, size, timestamp\n");
    
    while (entries) {
        char* path = fslog[i].path[0] == '\0' ? "N/A" : fslog[i].path;
        printf("%4d | %0#6o | %6d | %4d | %#8o | %6d | %s, %s, "
                "%d, %s", 
            i, fslog[i].opcode, fslog[i].result, fslog[i].fd_nr, 
            fslog[i].mode, fslog[i].cp_pid, fslog[i].cp_name, path,   
            fslog[i].size, ctime(&fslog[i].timestamp));
                
        i = (i + 1) % NR_FSLOGREC;
        entries--;
    
    }

    printf("%s\n", DELIM);
}

void print_fsloginf(struct fsloginf *fsloginf) {
    printf("%s\n", DELIM);
    printf("Filesystem log meta-information\n");
    printf("%s\n", DELIM);
    printf("index of first entry (start):   %4d\n", fsloginf->start);
    printf("entries in log (len):           %4d\n", fsloginf->len);
    printf("operations to log (ops2log):    %0#4o\n", fsloginf->ops2log);
    printf("%s\n", DELIM);
}

void create_log_workload(int file_nr) {
    char *path_fmt = "/tmp/%02d_fslog_test";
    int fildes[file_nr];
    
    open_test_files(PATH_LEN, path_fmt, fildes, file_nr);
    usleep(500000);
    
    write_test_files(fildes, file_nr);
    usleep(500000);
    
    read_test_files(fildes, file_nr);
    usleep(500000);
    
    close_test_files(fildes, file_nr);
    usleep(500000);
    
    unlink_test_files(PATH_LEN, path_fmt, fildes, file_nr);
}

void open_test_files(int path_len, char *path_fmt, int fildes[], int file_nr) {
    char path[path_len];
    
    for (int i = 0; i < file_nr; i++) {
        errno = 0;
        
        if (set_path(path, path_len, path_fmt, i)) {
            fildes[i] = FERR;        
            perror("set_path error");
        } else {
            fildes[i] = open(path, O_CREAT | O_RDWR | O_TRUNC, 
                                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fildes[i] == FERR)
                perror("open error");
        }
        
    }    
}

void write_test_files(int fildes[], int file_nr) {
    char *output_fmt = "test output %02d\n";
    
    for (int i = 0; i < file_nr; i++) {
        if (fildes[i] > FERR)
            (void) dprintf(fildes[i], output_fmt, i);
    }
}

void read_test_files(int fildes[], int file_nr) {
    char buf[OUTPUT_LEN+1];

    for (int i = 0; i < file_nr; i++) {
        if (fildes[i] > FERR) {
            (void) lseek(fildes[i], 0, SEEK_SET);
            int len = read(fildes[i], buf, OUTPUT_LEN);
            if (DEBUG_ON && len) {
                buf[len] = '\0';
                printf("%s\n", buf);
            }
        }
    }
}

void close_test_files(int fildes[], int file_nr) {
    for (int i = 0; i < file_nr; i++) {
        errno = 0;
        if (fildes[i] > FERR) {
            if (close(fildes[i]) == -1)
                perror("close error");
                
            fildes[i] = FCLS;
        }
    }
}

void unlink_test_files(int path_len, char *path_fmt, int fildes[], 
    int file_nr) {
    char path[path_len];
    
    for (int i = 0; i < file_nr; i++) {
        errno = 0;
        
        if (fildes[i] != FERR) {
            if (set_path(path, path_len, path_fmt, i))
                perror("set_tpath error");
            else {
                if (fildes[i] != FCLS) {
                    if (DEBUG_ON) 
                        printf("closing %s first\n", path);
                    
                    (void) close(fildes[i]);
                }
                    
                if (unlink(path))
                    perror("unlink error");
            }
            
            fildes[i] = FERR;
        }
    }
}

int set_path(char *path, int path_len, char *path_fmt, int nr) {
    int r = snprintf(path, path_len, path_fmt, nr) == path_len - 1 ? 0 : FERR;
    
    if (r && !errno)
        errno = EINVAL;
        
    return r;
}

