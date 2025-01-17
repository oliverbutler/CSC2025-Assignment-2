/* This file contains the procedures for creating, opening, closing, and
 * seeking on files.
 *
 * The entry points into this file are
 *   do_creat:	perform the CREAT system call
 *   do_open:	perform the OPEN system call
 *   do_mknod:	perform the MKNOD system call
 *   do_mkdir:	perform the MKDIR system call
 *   do_close:	perform the CLOSE system call
 *   do_lseek:  perform the LSEEK system call
 *   do_llseek: perform the LLSEEK system call
 */

#include "fs.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <minix/u64.h>
#include "file.h"
#include "fproc.h"
#include "scratchpad.h"
#include "dmap.h"
#include "lock.h"
#include "param.h"
#include <dirent.h>
#include <assert.h>
#include <minix/vfsif.h>
#include "vnode.h"
#include "vmnt.h"
#include "path.h"
/* CSC2025 mod start */
#include "fslog.h"
/* CSC2025 mod end */

char mode_map[] = {R_BIT, W_BIT, R_BIT | W_BIT, 0};

static struct vnode *new_node(struct lookup *resolve, int oflags,
                              mode_t bits);
static int pipe_open(struct vnode *vp, mode_t bits, int oflags);

/*===========================================================================*
 *				do_creat				     *
 *===========================================================================*/
int do_creat() {
    /* Perform the creat(name, mode) system call.
 * syscall might provide 'name' embedded in the message.
 */

    char fullpath[PATH_MAX];
    vir_bytes vname;
    size_t vname_length;
    mode_t open_mode;

    vname = (vir_bytes)job_m_in.name;
    vname_length = (size_t)job_m_in.name_length;
    open_mode = (mode_t)job_m_in.mode;

    if (copy_name(vname_length, fullpath) != OK) {
        /* Direct copy failed, try fetching from user space */
        if (fetch_name(vname, vname_length, fullpath) != OK) {
            /* CSC2025 mod start */
            logfserr_nopath(FSOP_OPEN | FSOP_CREAT, err_code);
            // create implies open
            /* CSC2025 mod end */
            return (err_code);
        }
    }

    return common_open(fullpath, O_WRONLY | O_CREAT | O_TRUNC, open_mode);
}

/*===========================================================================*
 *				do_open					     *
 *===========================================================================*/
int do_open() {
    /* Perform the open(name, flags,...) system call.
 * syscall might provide 'name' embedded in message when not creating file */

    int create_mode;   /* is really mode_t but this gives problems */
    int open_mode = 0; /* is really mode_t but this gives problems */
    int r = OK;
    char fullpath[PATH_MAX];
    vir_bytes vname;
    size_t vname_length;

    open_mode = (mode_t)job_m_in.mode;
    create_mode = job_m_in.c_mode;
    /* CSC2025 mod start */
    unsigned short fsopcode = FSOP_OPEN;
    /* CSC2025 mod end */

    /* If O_CREAT is set, open has three parameters, otherwise two. */
    if (open_mode & O_CREAT) {
        /* CSC2025 mod start */
        fsopcode = fsopcode | FSOP_CREAT;  // open and create
                                           /* CSC2025 mod end */
        vname = (vir_bytes)job_m_in.name1;
        vname_length = (size_t)job_m_in.name1_length;
        r = fetch_name(vname, vname_length, fullpath);
    } else {
        vname = (vir_bytes)job_m_in.name;
        vname_length = (size_t)job_m_in.name_length;
        create_mode = 0;
        if (copy_name(vname_length, fullpath) != OK) {
            /* Direct copy failed, try fetching from user space */
            if (fetch_name(vname, vname_length, fullpath) != OK)
                r = err_code;
        }
    }

    if (r != OK) {
        /* CSC2025 mod start */
        logfserr_nopath(fsopcode, err_code);
        /* CSC2025 mod end */
        return (err_code); /* name was bad */
    }

    return common_open(fullpath, open_mode, create_mode);
}

/*===========================================================================*
 *				common_open				     *
 *===========================================================================*/
int common_open(char path[PATH_MAX], int oflags, mode_t omode) {
    /* Common code from do_creat and do_open. */
    int b, r, exist = TRUE, major_dev;
    dev_t dev;
    mode_t bits;
    struct filp *filp, *filp2;
    struct vnode *vp;
    struct vmnt *vmp;
    struct dmap *dp;
    struct lookup resolve;
    /* CSC2025 mod start */
    unsigned short fsopcode = FSOP_OPEN;
    /* CS2025 mod end */

    /* Remap the bottom two bits of oflags. */
    bits = (mode_t)mode_map[oflags & O_ACCMODE];
    /* CSC2025 mod start - add log statement and curly brackets */
    if (!bits) {
        logfserr_nopath(fsopcode, EINVAL);
        return (EINVAL);
    }
    /* CS2025 mod end */

    /* See if file descriptor and filp slots are available. */
    /* CSC2025 mod start - add log statement and curly brackets */
    if ((r = get_fd(0, bits, &(scratch(fp).file.fd_nr), &filp)) != OK) {
        logfserr_nopath(fsopcode, r);
        return (r);
    }
    /* CSC2025 mod start - add log statement and curly brackets */

    lookup_init(&resolve, path, PATH_NOFLAGS, &vmp, &vp);

    /* If O_CREATE is set, try to make the file. */
    if (oflags & O_CREAT) {
        /* CSC2025 mod start */
        fsopcode = fsopcode | FSOP_CREAT;  // log open with create flag
        /* CSC2025 mod end */

        omode = I_REGULAR | (omode & ALLPERMS & fp->fp_umask);
        vp = new_node(&resolve, oflags, omode);
        r = err_code;
        if (r == OK)
            exist = FALSE;      /* We just created the file */
        else if (r != EEXIST) { /* other error */
                                /* CSC2025 mod start */
            logfserr(fsopcode, r, path);
            /* CSC2025 mod end */
            if (vp) unlock_vnode(vp);
            unlock_filp(filp);
            return (r);
        } else
            exist = !(oflags & O_EXCL); /* file exists, if the O_EXCL
					   flag is set this is an error */
    } else {
        /* Scan path name */
        resolve.l_vmnt_lock = VMNT_READ;
        resolve.l_vnode_lock = VNODE_OPCL;
        if ((vp = eat_path(&resolve, fp)) == NULL) {
            /* CSC2025 mod start */
            logfserr(fsopcode, err_code, path);
            /* CSC2025 mod end */
            unlock_filp(filp);
            return (err_code);
        }

        if (vmp != NULL) unlock_vmnt(vmp);
    }

    /* Claim the file descriptor and filp slot and fill them in. */
    fp->fp_filp[scratch(fp).file.fd_nr] = filp;
    FD_SET(scratch(fp).file.fd_nr, &fp->fp_filp_inuse);
    filp->filp_count = 1;
    filp->filp_vno = vp;
    filp->filp_flags = oflags;

    /* Only do the normal open code if we didn't just create the file. */
    if (exist) {
        /* Check protections. */
        if ((r = forbidden(fp, vp, bits)) == OK) {
            /* Opening reg. files, directories, and special files differ */
            switch (vp->v_mode & S_IFMT) {
                case S_IFREG:
                    /* Truncate regular file if O_TRUNC. */
                    if (oflags & O_TRUNC) {
                        if ((r = forbidden(fp, vp, W_BIT)) != OK)
                            break;
                        upgrade_vnode_lock(vp);
                        truncate_vnode(vp, 0);
                    }
                    break;
                case S_IFDIR:
                    /* Directories may be read but not written. */
                    r = (bits & W_BIT ? EISDIR : OK);
                    break;
                case S_IFCHR:
                    /* Invoke the driver for special processing. */
                    dev = (dev_t)vp->v_sdev;
                    /* TTY needs to know about the O_NOCTTY flag. */
                    r = dev_open(dev, who_e, bits | (oflags & O_NOCTTY));
                    if (r == SUSPEND)
                        suspend(FP_BLOCKED_ON_DOPEN);
                    else
                        vp = filp->filp_vno; /* Might be updated by
						   * dev_open/clone_opcl */
                    break;
                case S_IFBLK:

                    lock_bsf();

                    /* Invoke the driver for special processing. */
                    dev = (dev_t)vp->v_sdev;
                    r = bdev_open(dev, bits);
                    if (r != OK) {
                        unlock_bsf();
                        break;
                    }

                    major_dev = major(vp->v_sdev);
                    dp = &dmap[major_dev];
                    if (dp->dmap_driver == NONE) {
                        printf("VFS: block driver disappeared!\n");
                        unlock_bsf();
                        r = ENXIO;
                        break;
                    }

                    /* Check whether the device is mounted or not. If so,
			 * then that FS is responsible for this device.
			 * Otherwise we default to ROOT_FS.
			 */
                    vp->v_bfs_e = ROOT_FS_E; /* By default */
                    for (vmp = &vmnt[0]; vmp < &vmnt[NR_MNTS]; ++vmp)
                        if (vmp->m_dev == vp->v_sdev &&
                            !(vmp->m_flags & VMNT_FORCEROOTBSF)) {
                            vp->v_bfs_e = vmp->m_fs_e;
                        }

                    /* Send the driver label to the file system that will
			 * handle the block I/O requests (even when its label
			 * and endpoint are known already), but only when it is
			 * the root file system. Other file systems will
			 * already have it anyway.
			 */
                    if (vp->v_bfs_e != ROOT_FS_E) {
                        unlock_bsf();
                        break;
                    }

                    if (req_newdriver(vp->v_bfs_e, vp->v_sdev,
                                      dp->dmap_label) != OK) {
                        printf("VFS: error sending driver label\n");
                        bdev_close(dev);
                        r = ENXIO;
                    }
                    unlock_bsf();
                    break;

                case S_IFIFO:
                    /* Create a mapped inode on PFS which handles reads
			   and writes to this named pipe. */
                    upgrade_vnode_lock(vp);
                    r = map_vnode(vp, PFS_PROC_NR);
                    if (r == OK) {
                        if (vp->v_ref_count == 1) {
                            if (vp->v_size != 0)
                                r = truncate_vnode(vp, 0);
                        }
                        oflags |= O_APPEND; /* force append mode */
                        filp->filp_flags = oflags;
                    }
                    if (r == OK) {
                        r = pipe_open(vp, bits, oflags);
                    }
                    if (r != ENXIO) {
                        /* See if someone else is doing a rd or wt on
				 * the FIFO.  If so, use its filp entry so the
				 * file position will be automatically shared.
				 */
                        b = (bits & R_BIT ? R_BIT : W_BIT);
                        filp->filp_count = 0; /* don't find self */
                        if ((filp2 = find_filp(vp, b)) != NULL) {
                            /* Co-reader or writer found. Use it.*/
                            fp->fp_filp[scratch(fp).file.fd_nr] = filp2;
                            filp2->filp_count++;
                            filp2->filp_vno = vp;
                            filp2->filp_flags = oflags;

                            /* v_count was incremented after the vnode
				     * has been found. i_count was incremented
				     * incorrectly in FS, not knowing that we
				     * were going to use an existing filp
				     * entry.  Correct this error.
				     */
                            unlock_vnode(vp);
                            put_vnode(vp);
                        } else {
                            /* Nobody else found. Restore filp. */
                            filp->filp_count = 1;
                        }
                    }
                    break;
            }
        }
    }

    /* CSC2025 mod start */
    /* log before unlock - need fd_nr if OK - see line 358 for how to get it */
    if (r == OK)
        logfsop(fsopcode, r, path, scratch(fp).file.fd_nr, vp->v_mode, vp->v_size);
    else
        logfserr(fsopcode, r, path);
    /* CSC2025 mod end */

    unlock_filp(filp);

    /* If error, release inode. */
    if (r != OK) {
        if (r != SUSPEND) {
            fp->fp_filp[scratch(fp).file.fd_nr] = NULL;
            FD_CLR(scratch(fp).file.fd_nr, &fp->fp_filp_inuse);
            filp->filp_count = 0;
            filp->filp_vno = NULL;
            filp->filp_state &= ~FS_INVALIDATED; /* Prevent garbage col. */
            put_vnode(vp);
        }
    } else {
        r = scratch(fp).file.fd_nr;
    }

    return (r);
}

/*===========================================================================*
 *				new_node				     *
 *===========================================================================*/
static struct vnode *new_node(struct lookup *resolve, int oflags, mode_t bits) {
    /* Try to create a new inode and return a pointer to it. If the inode already
   exists, return a pointer to it as well, but set err_code accordingly.
   NULL is returned if the path cannot be resolved up to the last
   directory, or when the inode cannot be created due to permissions or
   otherwise. */
    struct vnode *dirp, *vp;
    struct vmnt *dir_vmp, *vp_vmp;
    int r;
    struct node_details res;
    struct lookup findnode;
    char *path;

    path = resolve->l_path; /* For easy access */

    lookup_init(&findnode, path, resolve->l_flags, &dir_vmp, &dirp);
    findnode.l_vmnt_lock = VMNT_WRITE;
    findnode.l_vnode_lock = VNODE_WRITE; /* dir node */

    /* When O_CREAT and O_EXCL flags are set, the path may not be named by a
   * symbolic link. */
    if (oflags & O_EXCL) findnode.l_flags |= PATH_RET_SYMLINK;

    /* See if the path can be opened down to the last directory. */
    if ((dirp = last_dir(&findnode, fp)) == NULL) return (NULL);

    /* The final directory is accessible. Get final component of the path. */
    lookup_init(&findnode, findnode.l_path, findnode.l_flags, &vp_vmp, &vp);
    findnode.l_vmnt_lock = VMNT_WRITE;
    findnode.l_vnode_lock = (oflags & O_TRUNC) ? VNODE_WRITE : VNODE_OPCL;
    vp = advance(dirp, &findnode, fp);
    assert(vp_vmp == NULL); /* Lookup to last dir should have yielded lock
				 * on vmp or final component does not exist.
				 * Either way, vp_vmp ought to be not set.
				 */

    /* The combination of a symlink with absolute path followed by a danglink
   * symlink results in a new path that needs to be re-resolved entirely. */
    if (path[0] == '/') {
        unlock_vnode(dirp);
        unlock_vmnt(dir_vmp);
        put_vnode(dirp);
        if (vp != NULL) {
            unlock_vnode(vp);
            put_vnode(vp);
        }
        return new_node(resolve, oflags, bits);
    }

    if (vp == NULL && err_code == ENOENT) {
        /* Last path component does not exist. Make a new directory entry. */
        if ((vp = get_free_vnode()) == NULL) {
            /* Can't create new entry: out of vnodes. */
            unlock_vnode(dirp);
            unlock_vmnt(dir_vmp);
            put_vnode(dirp);
            return (NULL);
        }

        lock_vnode(vp, VNODE_OPCL);
        upgrade_vmnt_lock(dir_vmp); /* Creating file, need exclusive access */

        if ((r = forbidden(fp, dirp, W_BIT | X_BIT)) != OK ||
            (r = req_create(dirp->v_fs_e, dirp->v_inode_nr, bits, fp->fp_effuid,
                            fp->fp_effgid, path, &res)) != OK) {
            /* Can't create inode either due to permissions or some other
		 * problem. In case r is EEXIST, we might be dealing with a
		 * dangling symlink.*/

            /* Downgrade lock to prevent deadlock during symlink resolving*/
            downgrade_vmnt_lock(dir_vmp);

            if (r == EEXIST) {
                struct vnode *slp, *old_wd;

                /* Resolve path up to symlink */
                findnode.l_flags = PATH_RET_SYMLINK;
                findnode.l_vnode_lock = VNODE_READ;
                findnode.l_vnode = &slp;
                slp = advance(dirp, &findnode, fp);
                if (slp != NULL) {
                    if (S_ISLNK(slp->v_mode)) {
                        /* Get contents of link */

                        r = req_rdlink(slp->v_fs_e,
                                       slp->v_inode_nr,
                                       VFS_PROC_NR,
                                       (vir_bytes)path,
                                       PATH_MAX - 1, 0);
                        if (r < 0) {
                            /* Failed to read link */
                            unlock_vnode(slp);
                            unlock_vnode(dirp);
                            unlock_vmnt(dir_vmp);
                            put_vnode(slp);
                            put_vnode(dirp);
                            err_code = r;
                            return (NULL);
                        }
                        path[r] = '\0'; /* Terminate path */
                    }
                    unlock_vnode(slp);
                    put_vnode(slp);
                }

                /* Try to create the inode the dangling symlink was
			 * pointing to. We have to use dirp as starting point
			 * as there might be multiple successive symlinks
			 * crossing multiple mountpoints.
			 * Unlock vnodes and vmnts as we're going to recurse.
			 */
                unlock_vnode(dirp);
                unlock_vnode(vp);
                unlock_vmnt(dir_vmp);

                old_wd = fp->fp_wd; /* Save orig. working dirp */
                fp->fp_wd = dirp;
                vp = new_node(resolve, oflags, bits);
                fp->fp_wd = old_wd; /* Restore */

                if (vp != NULL) {
                    put_vnode(dirp);
                    *(resolve->l_vnode) = vp;
                    return (vp);
                }
                r = err_code;
            }

            if (r == EEXIST)
                err_code = EIO; /* Impossible, we have verified that
					 * the last component doesn't exist and
					 * is not a dangling symlink. */
            else
                err_code = r;

            unlock_vmnt(dir_vmp);
            unlock_vnode(dirp);
            unlock_vnode(vp);
            put_vnode(dirp);
            return (NULL);
        }

        /* Store results and mark vnode in use */

        vp->v_fs_e = res.fs_e;
        vp->v_inode_nr = res.inode_nr;
        vp->v_mode = res.fmode;
        vp->v_size = res.fsize;
        vp->v_uid = res.uid;
        vp->v_gid = res.gid;
        vp->v_sdev = res.dev;
        vp->v_vmnt = dirp->v_vmnt;
        vp->v_dev = vp->v_vmnt->m_dev;
        vp->v_fs_count = 1;
        vp->v_ref_count = 1;
    } else {
        /* Either last component exists, or there is some other problem. */
        if (vp != NULL) {
            r = EEXIST; /* File exists or a symlink names a file while
				 * O_EXCL is set. */
        } else
            r = err_code; /* Other problem. */
    }

    err_code = r;
    /* When dirp equals vp, we shouldn't release the lock as a vp is locked only
   * once. Releasing the lock would cause the resulting vp not be locked and
   * cause mayhem later on. */
    if (dirp != vp) {
        unlock_vnode(dirp);
    }
    unlock_vmnt(dir_vmp);
    put_vnode(dirp);

    *(resolve->l_vnode) = vp;
    return (vp);
}

/*===========================================================================*
 *				pipe_open				     *
 *===========================================================================*/
static int pipe_open(struct vnode *vp, mode_t bits, int oflags) {
    /*  This function is called from common_open. It checks if
 *  there is at least one reader/writer pair for the pipe, if not
 *  it suspends the caller, otherwise it revives all other blocked
 *  processes hanging on the pipe.
 */

    if ((bits & (R_BIT | W_BIT)) == (R_BIT | W_BIT)) return (ENXIO);

    /* Find the reader/writer at the other end of the pipe */
    if (find_filp(vp, bits & W_BIT ? R_BIT : W_BIT) == NULL) {
        /* Not found */
        if (oflags & O_NONBLOCK) {
            if (bits & W_BIT) return (ENXIO);
        } else {
            /* Let's wait for the other side to show up */
            suspend(FP_BLOCKED_ON_POPEN);
            return (SUSPEND);
        }
    } else if (susp_count > 0) { /* revive blocked processes */
        release(vp, OPEN, susp_count);
        release(vp, CREAT, susp_count);
    }
    return (OK);
}

/*===========================================================================*
 *				do_mknod				     *
 *===========================================================================*/
int do_mknod() {
    /* Perform the mknod(name, mode, addr) system call. */
    register mode_t bits, mode_bits;
    int r;
    struct vnode *vp;
    struct vmnt *vmp;
    char fullpath[PATH_MAX];
    struct lookup resolve;
    vir_bytes vname1;
    size_t vname1_length;
    dev_t dev;

    vname1 = (vir_bytes)job_m_in.name1;
    vname1_length = (size_t)job_m_in.name1_length;
    mode_bits = (mode_t)job_m_in.mk_mode; /* mode of the inode */
    dev = job_m_in.m1_i3;

    lookup_init(&resolve, fullpath, PATH_NOFLAGS, &vmp, &vp);
    resolve.l_vmnt_lock = VMNT_WRITE;
    resolve.l_vnode_lock = VNODE_WRITE;

    /* Only the super_user may make nodes other than fifos. */
    if (!super_user && (!S_ISFIFO(mode_bits) && !S_ISSOCK(mode_bits))) {
        return (EPERM);
    }
    bits = (mode_bits & S_IFMT) | (mode_bits & ACCESSPERMS & fp->fp_umask);

    /* Open directory that's going to hold the new node. */
    if (fetch_name(vname1, vname1_length, fullpath) != OK) return (err_code);
    if ((vp = last_dir(&resolve, fp)) == NULL) return (err_code);

    /* Make sure that the object is a directory */
    if (!S_ISDIR(vp->v_mode)) {
        r = ENOTDIR;
    } else if ((r = forbidden(fp, vp, W_BIT | X_BIT)) == OK) {
        r = req_mknod(vp->v_fs_e, vp->v_inode_nr, fullpath, fp->fp_effuid,
                      fp->fp_effgid, bits, dev);
    }

    unlock_vnode(vp);
    unlock_vmnt(vmp);
    put_vnode(vp);
    return (r);
}

/*===========================================================================*
 *				do_mkdir				     *
 *===========================================================================*/
int do_mkdir() {
    /* Perform the mkdir(name, mode) system call. */
    mode_t bits; /* mode bits for the new inode */
    int r;
    struct vnode *vp;
    struct vmnt *vmp;
    char fullpath[PATH_MAX];
    struct lookup resolve;
    vir_bytes vname1;
    size_t vname1_length;
    mode_t dirmode;

    vname1 = (vir_bytes)job_m_in.name1;
    vname1_length = (size_t)job_m_in.name1_length;
    dirmode = (mode_t)job_m_in.mode;

    lookup_init(&resolve, fullpath, PATH_NOFLAGS, &vmp, &vp);
    resolve.l_vmnt_lock = VMNT_WRITE;
    resolve.l_vnode_lock = VNODE_WRITE;

    if (fetch_name(vname1, vname1_length, fullpath) != OK) return (err_code);
    bits = I_DIRECTORY | (dirmode & RWX_MODES & fp->fp_umask);
    if ((vp = last_dir(&resolve, fp)) == NULL) return (err_code);

    /* Make sure that the object is a directory */
    if (!S_ISDIR(vp->v_mode)) {
        r = ENOTDIR;
    } else if ((r = forbidden(fp, vp, W_BIT | X_BIT)) == OK) {
        r = req_mkdir(vp->v_fs_e, vp->v_inode_nr, fullpath, fp->fp_effuid,
                      fp->fp_effgid, bits);
    }

    unlock_vnode(vp);
    unlock_vmnt(vmp);
    put_vnode(vp);
    return (r);
}

/*===========================================================================*
 *				do_lseek				     *
 *===========================================================================*/
int do_lseek() {
    /* Perform the lseek(ls_fd, offset, whence) system call. */
    register struct filp *rfilp;
    int r = OK, seekfd, seekwhence;
    off_t offset;
    u64_t pos, newpos;

    seekfd = job_m_in.ls_fd;
    seekwhence = job_m_in.whence;
    offset = (off_t)job_m_in.offset_lo;

    /* Check to see if the file descriptor is valid. */
    if ((rfilp = get_filp(seekfd, VNODE_READ)) == NULL) return (err_code);

    /* No lseek on pipes. */
    if (S_ISFIFO(rfilp->filp_vno->v_mode)) {
        unlock_filp(rfilp);
        return (ESPIPE);
    }

    /* The value of 'whence' determines the start position to use. */
    switch (seekwhence) {
        case SEEK_SET:
            pos = cvu64(0);
            break;
        case SEEK_CUR:
            pos = rfilp->filp_pos;
            break;
        case SEEK_END:
            pos = cvul64(rfilp->filp_vno->v_size);
            break;
        default:
            unlock_filp(rfilp);
            return (EINVAL);
    }

    if (offset >= 0)
        newpos = add64ul(pos, offset);
    else
        newpos = sub64ul(pos, -offset);

    /* Check for overflow. */
    if (ex64hi(newpos) != 0) {
        r = EOVERFLOW;
    } else if ((off_t)ex64lo(newpos) < 0) { /* no negative file size */
        r = EOVERFLOW;
    } else {
        /* insert the new position into the output message */
        m_out.reply_l1 = ex64lo(newpos);

        if (cmp64(newpos, rfilp->filp_pos) != 0) {
            rfilp->filp_pos = newpos;

            /* Inhibit read ahead request */
            r = req_inhibread(rfilp->filp_vno->v_fs_e,
                              rfilp->filp_vno->v_inode_nr);
        }
    }

    unlock_filp(rfilp);
    return (r);
}

/*===========================================================================*
 *				do_llseek				     *
 *===========================================================================*/
int do_llseek() {
    /* Perform the llseek(ls_fd, offset, whence) system call. */
    register struct filp *rfilp;
    u64_t pos, newpos;
    int r = OK, seekfd, seekwhence;
    long off_hi, off_lo;

    seekfd = job_m_in.ls_fd;
    seekwhence = job_m_in.whence;
    off_hi = job_m_in.offset_high;
    off_lo = job_m_in.offset_lo;

    /* Check to see if the file descriptor is valid. */
    if ((rfilp = get_filp(seekfd, VNODE_READ)) == NULL) return (err_code);

    /* No lseek on pipes. */
    if (S_ISFIFO(rfilp->filp_vno->v_mode)) {
        unlock_filp(rfilp);
        return (ESPIPE);
    }

    /* The value of 'whence' determines the start position to use. */
    switch (seekwhence) {
        case SEEK_SET:
            pos = cvu64(0);
            break;
        case SEEK_CUR:
            pos = rfilp->filp_pos;
            break;
        case SEEK_END:
            pos = cvul64(rfilp->filp_vno->v_size);
            break;
        default:
            unlock_filp(rfilp);
            return (EINVAL);
    }

    newpos = add64(pos, make64(off_lo, off_hi));

    /* Check for overflow. */
    if ((off_hi > 0) && cmp64(newpos, pos) < 0)
        r = EINVAL;
    else if ((off_hi < 0) && cmp64(newpos, pos) > 0)
        r = EINVAL;
    else {
        /* insert the new position into the output message */
        m_out.reply_l1 = ex64lo(newpos);
        m_out.reply_l2 = ex64hi(newpos);

        if (cmp64(newpos, rfilp->filp_pos) != 0) {
            rfilp->filp_pos = newpos;

            /* Inhibit read ahead request */
            r = req_inhibread(rfilp->filp_vno->v_fs_e,
                              rfilp->filp_vno->v_inode_nr);
        }
    }

    unlock_filp(rfilp);
    return (r);
}

/*===========================================================================*
 *				do_close				     *
 *===========================================================================*/
int do_close() {
    /* Perform the close(fd) system call. */

    scratch(fp).file.fd_nr = job_m_in.fd;
    return close_fd(fp, scratch(fp).file.fd_nr);
}

/*===========================================================================*
 *				close_fd				     *
 *===========================================================================*/
int close_fd(rfp, fd_nr) struct fproc *rfp;
int fd_nr;
{
    /* Perform the close(fd) system call. */
    register struct filp *rfilp;
    register struct vnode *vp;
    struct file_lock *flp;
    int lock_count;

    /* First locate the vnode that belongs to the file descriptor. */
    /* CSC2025 mod start */
    if ((rfilp = get_filp2(rfp, fd_nr, VNODE_OPCL)) == NULL) {
        logfserr_nopath(FSOP_CLOSE, err_code);
        return (err_code);
    }
    /* CSC2025 mod end */

    vp = rfilp->filp_vno;

    /* CSC2025 mod start */
    logfsop_nopath(FSOP_CLOSE, OK, fd_nr, vp->v_mode, vp->v_size);
    /* CSC2025 mod end */

    close_filp(rfilp);
    rfp->fp_filp[fd_nr] = NULL;
    FD_CLR(fd_nr, &rfp->fp_cloexec_set);
    FD_CLR(fd_nr, &rfp->fp_filp_inuse);

    /* Check to see if the file is locked.  If so, release all locks. */
    if (nr_locks > 0) {
        lock_count = nr_locks; /* save count of locks */
        for (flp = &file_lock[0]; flp < &file_lock[NR_LOCKS]; flp++) {
            if (flp->lock_type == 0) continue; /* slot not in use */
            if (flp->lock_vnode == vp && flp->lock_pid == rfp->fp_pid) {
                flp->lock_type = 0;
                nr_locks--;
            }
        }
        if (nr_locks < lock_count)
            lock_revive(); /* one or more locks released */
    }

    return (OK);
}

/*===========================================================================*
 *				close_reply				     *
 *===========================================================================*/
void close_reply() {
    /* No need to do anything */
}
