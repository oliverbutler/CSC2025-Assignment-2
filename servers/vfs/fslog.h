#ifndef __VFS_FSLOG_H__
#define __VFS_FSLOG_H__
#include <sys/types.h>

#define UNKNOWN_FD_NR   -1  /* unknown file descriptor number */
#define UNKNOWN_MODE     0  /* unknown file open mode */
#define UNKNOWN_SIZE    -1  /* unknown file size */
#define UNKNOWN_CP_PID  -2  /* unknown calling process pid */

/*
 * logfserr(int opcode, int result, char *path)
 * 
 * Description:
 * Use this function to instrument the virtual filesystem server (VFS) and 
 * log the details of a filesystem error. If error logging is enabled for the
 * operation identified by the given opcode, log the opcode, the result and
 * path of the file being operated on to the in-memory filesystem log.
 * Use this logging function when the file path is known. That is, use this
 * function for certain cases of errors in open, create and unlink operations
 * when the path of the file being operated on is known.
 * This function is responsible for checking that logging is enabled. The 
 * user of the function does not need to check and should not check. 
 *
 * Use logfserr_nopath to log errors when the path is unknown.
 * 
 * Parameters:
 * opcode - the filesystem logging opcode representing the current 
 *      filesystem call. Valid opcodes are defined in unistd.h.
 * result - the result/error status of the call.
 * path - the path of the file being operated on. The path is only known 
 *      for certain cases of errors in open, create and unlink operations. Use
 *      logfserr_nopath when the path is unknown.
 */
void logfserr(int opcode, int result, char *path);

/*
 * logfserr_nopath(int opcode, int result)
 * 
 * Description:
 * Use this function to instrument the virtual filesystem server (VFS) and 
 * log the details of a filesystem error. If error logging is enabled for the
 * operation identified by the given opcode, log the opcode and the result to
 * the in-memory filesystem log.
 * Use this logging function when the file path is not known. That is, use this 
 * function for certain cases of errors in open, create and unlink operations 
 * and for all cases of errors in close, read and write operations when the 
 * path of the file being operated on is unknown.
 * This function is responsible for checking that logging is enabled. The 
 * user of the function does not need to check and should not check. 
 *
 * Use logfserr to log errors when the path is known.
 *
 * Parameters:
 * opcode - the filesystem logging opcode representing the current 
 *      filesystem call. Valid opcodes are defined in unistd.h.
 * result - the result/error status of the call.
 */
void logfserr_nopath(int opcode, int result);

/*
 * logfsop(int opcode, int result, char *path, int fd_nr, mode_t mode, 
 *      off_t size)
 * 
 * Description:
 * Use this function to instrument the virtual filesystem server (VFS) and log
 * the details of a filesystem operation. If logging is enabled for the
 * operation identified by the given opcode, log the opcode, the result, the 
 * path of the file being operated, the file descriptor, the open mode of 
 * file and the file size. Note some of these parameters may be unknown, in 
 * which case use the relevant constant.
 * Use this logging function when the file path being operated on is known. 
 * That is, use this function for certain cases of open, create and unlink
 * operations.
 * This function is responsible for checking that logging is enabled. The 
 * user of the function does not need to check and should not check. 
 *
 * Use logfsop_nopath when the path is unknown.
 * 
 * Parameters:
 * opcode - the filesystem logging opcode representing the current 
 *      filesystem call. Valid opcodes are defined in unistd.h.
 * result - the result/error status of the call.
 * path - the path of the file being operated on. The path is only known 
 *      for certain cases of open, create and unlink operations. Use
 *      logfsop_nopath when the path is unknown.
 * fd_nr - the file descriptor number, use UNKNOWN_FD_NR if unknown
 * omode - the file open/permission mode, use UNKNOWN_MODE if unknown
 * size - the file size, use UNKNOWN_SIZE if unknown
 */
void logfsop(int opcode, int result, char *path, int fd_nr, mode_t omode,
    off_t size);
    
/*
 * logfsop_nopath(int opcode, int result, int fd_nr, mode_t omode,
 *      off_t size)
 * 
 * Description:
 * Use this function to instrument the virtual filesystem server (VFS) and log
 * the details of a filesystem operation. If logging is enabled for the
 * operation identified by the given opcode, log the opcode, the result, the
 * file descriptor, the open mode of file and the file size
 * Use this logging function when the file path is not known. That is, use 
 * this function for close, read and write operations when only the
 * file descriptor is known.
 * This function is responsible for checking that logging is enabled. The 
 * user of the function does not need to check and should not check. 
 *
 * Use logfsop when the path is known
 *
 * Parameters:
 * opcode - the filesystem logging opcode representing the current 
 *      filesystem call. Valid opcodes are defined in unistd.h.
 * result - the result/error status of the call.
 * fd_nr - the file descriptor number, use UNKNOWN_FD_NR if unknown
 * omode - the file open/permission mode, use UNKNOWN_MODE if unknown
 * size - the file size, use UNKNOWN_SIZE if unknown
 */
void logfsop_nopath(int opcode, int result, int fd_nr, mode_t omode,
    off_t size);



#endif