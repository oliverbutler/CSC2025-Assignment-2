#!/bin/sh
USR_SRC=/usr/src
INCLUDE=include
SERVERS_VFS=servers/vfs
INCLUDE_MINIX=${INCLUDE}/minix

cp ${INCLUDE}/unistd.h ${USR_SRC}/${INCLUDE}
cp ${INCLUDE_MINIX}/callnr.h ${USR_SRC}/${INCLUDE_MINIX}

cp ${SERVERS_VFS}/fslog.c ${USR_SRC}/${SERVERS_VFS}
cp ${SERVERS_VFS}/fslog.h ${USR_SRC}/${SERVERS_VFS}
cp ${SERVERS_VFS}/link.c ${USR_SRC}/${SERVERS_VFS}
cp ${SERVERS_VFS}/Makefile ${USR_SRC}/${SERVERS_VFS}
cp ${SERVERS_VFS}/open.c ${USR_SRC}/${SERVERS_VFS}
cp ${SERVERS_VFS}/proto.h ${USR_SRC}/${SERVERS_VFS}
cp ${SERVERS_VFS}/read.c ${USR_SRC}/${SERVERS_VFS}
cp ${SERVERS_VFS}/table.c ${USR_SRC}/${SERVERS_VFS}


