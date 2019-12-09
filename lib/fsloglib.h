#ifndef __LIB_FSLOGLIB_H__
#define __LIB_FSLOGLIB_H__
#include <unistd.h>

/*
 * initfslog()
 * 
 * Description:
 * Resets/initialises the filesystem log to start position 0, length 0 and
 * logging switched off (no operations to log).
 * 
 * Usage: 
 *      initfslog(); 
*
 * Return:
 * Not applicable
 *
 * Errors:
 * Not applicable
 */
void initfslog(void);

/*
 * startfslog(unsigned short ops2log)
 * 
 * Description:
 * Start or restart the in-memory filesystem operations log and 
 * start logging operations specified by the the ops2log selector, which must 
 * be in the range FSOP_NONE to FSOP_ALL, inclusive.
 * Calling this function resets the meta-information of the filesystem log to
 * a start position of 0 and length of 0 and changes the operations to 
 * log selector to the ORed combination of the current ops2log value and the
 * ops2log value passed as a parameter to this function.
 * 
 * Usage: 
 *      startfslog(FSOP_OPEN | FSOP_CLOSE | FSOP_ERR); 
 *                          // reset the in-memory log to log both 
 *                          // successful and unsuccessful open and close
 *                          // operations in addition to any existing logging
 *      starfslog(FSOP_READ); 
 *                          // reset the in-memory log to log read operations 
 *                          // in addition to any existing logging   
 *
 * Parameters:
 * ops2log - the ORed combination of one or more of the following FSOP values 
 * defined in unistd.h.
 *      FSOP_NONE       - no selection (no change to operations to log)
 *      FSOP_OPEN       - log open filesytem operations
 *      FSOP_CLOSE      - log close filesytem operations
 *      FSOP_READ       - log read filesytem operations
 *      FSOP_WRITE      - log write filesytem operations   
 *      FSOP_CREAT      - log create filesytem operations   
 *      FSOP_UNLNK      - log unlink filesytem operations   
 *      FSOP_ALL_WOERR  - log all filesytem operations, excluding errors
 *      FSOP_ERR        - log errors for operations being logged
 *      FSOP_ALL        - log all filesytem operations, including errors
 * FSOP_NONE has no effect on the operations selected to log. 
 *
 * Return:
 * 0 on success
 * -1 on failure, in which case the global errno will be set to a value 
 * specified in errors.
 *
 * Errors:
 * On failure -1 is returned and the errno is set as follows:
 * EINVAL - invalid argument: if ops2log is less than FSOP_NONE or greater 
 *      than FSOP_ALL
 */
int startfslog(unsigned short ops2log);

/*
 * stopfslog(unsigned short ops2stoplog)
 * 
 * Description:
 * Switch off logging for the operations specified by the ops2stoplog selector,
 * which must be in the range FSOP_NONE to FSOP_ALL, inclusive.
 * Calling this function has no effect on the start position or length of log
 * meta-information. It deselects the operations to log specified by the 
 * ops2stoplog parameter to this function.
 *
 * Usage: 
 *      stopfslog(FSOP_OPEN);   // stop logging open operations 
 *      stopfslog(FSOP_ALL);    // stop logging all operations
 *
 * Parameters:
 * ops2stoplog - the ORed combination of one or more of the following FSOP 
 * values defined in unistd.h.
 *      FSOP_NONE       - no selection (no change to operations to log)
 *      FSOP_OPEN       - switch off logging of open filesytem operations
 *      FSOP_CLOSE      - switch off logging of close filesytem operations
 *      FSOP_READ       - switch off logging of read filesytem operations
 *      FSOP_WRITE      - switch off logging of write filesytem operations   
 *      FSOP_CREAT      - switch off logging of create filesytem operations   
 *      FSOP_UNLNK      - switch off logging of unlink filesytem operations   
 *      FSOP_ALL_WOERR  - switch off logging of all filesytem operations
 *      FSOP_ERR        - switch off logging of errors
 *      FSOP_ALL        - switch off logging of all filesytem operations
 * FSOP_NONE has no effect on the operations selected to log. 
 *
 * Return:
 * 0 on success
 * -1 on failure, in which case the global errno will be set to a value 
 * specified in errors.
 *
 * Errors:
 * On failure -1 is returned and the errno is set as follows:
 * EINVAL - invalid argument: if ops2stoplog is less than FSOP_NONE or greater 
 *      than FSOP_ALL
 */
int stopfslog(unsigned short ops2stoplog);


/*
 * getfsloginf(struct fsloginf *fsloginf)
 * 
 * Description:
 * Get the log meta-information - a fsloginf struct with fields for the 
 * start position of the log, the length of the log (i.e. the number of valid
 * entries in the log array) and the value of the ops2log selector. The 
 * fsloginf struct is defined in unistd.h.
 * 
 * Usage: 
 *      struct fsloginf fsloginf;
 *
 *      int r = getfsloginf(&fsloginf);
 *
 *      if (!r) {
 *          printf("start:   %d\n", fsloginf.start);
 *          printf("length:  %d\n", fsloginf.len);
 *          printf("ops2log: %d\n", fsloginf.ops2log);
 *      }
 *
 * Parameters:
 * fsloginf - a pointer to a fsloginf struct that will be updated with the 
 *      current values of the start of the log, the length of the log 
 *      and the operations to log selector.
 *
 * Return:
 * 0 on success
 * -1 on failure, in which case the global errno will be set to a value 
 * specified in errors.
 *
 * Errors:
 * On failure -1 is returned and the errno is set as follows:
 * EINVAL - invalid argument: if fsloginf is NULL
 * Other errors resulting from system call memory copying errors.
 */
int getfsloginf(struct fsloginf *fsloginf);


/*
 * getfslog(struct fsloginf *fsloginf, struct fslogrec fslog[])
 * 
 * Description:
 * Get the log meta-information (as provided by getfsloginf) and the
 * in-memory log - the circular buffer of filesystem operation entries.
 * The information obtained is a fsloginf struct (as in getfsloginf) and 
 * an array of fslogrec structs. Both struct types are defined in unistd.h.
 * 
 * Usage: 
 *      struct fsloginf fsloginf;
 *      struct fslogrec fslog[NR_FSLOGREC];
 *
 *      int r = getfsloginf(&fsloginf, fslog);
 *
 *      if (!r) {
 *          printf("start:   %d\n", fsloginf.start);
 *          printf("length:  %d\n", fsloginf.len);
 *          printf("ops2log: %d\n", fsloginf.ops2log);
 *          int i = fsloginf.start;
 *          int entries = fsloginf.len;
 *          while (entries) {
 *              printf("idx:   %d\n", i);
 *              printf("fd_nr: %d\n", fslog[i].fd_nr);
 *              ...
 *              ...
 *              i = (i + 1) % NR_FSLOGREC;
 *              entries--;
 *          }
 *      }
 *
 * Parameters:
 * fsloginf - a pointer to a fsloginf struct that will be updated with the 
 *      current values of the start of the log, the length of the log 
 *      and the operations to log selector.
 * fslog - an array of fslogrec structs that will be updated with information 
 *      from the filesystem operations log 
 *
 * Return:
 * 0 on success
 * -1 on failure, in which case the global errno will be set to a value 
 * specified in errors.
 *
 * Errors:
 * On failure -1 is returned and the errno is set as follows:
 * EINVAL - invalid argument: if fsloginf or fslog is NULL
 * Other errors resulting from system call memory copying errors.
 */
int getfslog(struct fsloginf *fsloginf, struct fslogrec fslog[]);

#endif

