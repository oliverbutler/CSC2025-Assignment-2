CC=cc -Wall
LIBS=fslog_testutil.o

all: syscall_apps test_syscalls

fslog_testutil.o: ../util/fslog_testutil.c
	$(CC) -c $?

do_startfslog: do_startfslog.c $(LIBS)
	$(CC) $? -o $@
    
do_stopfslog: do_stopfslog.c $(LIBS)
	$(CC) $? -o $@
    
do_getfslog: do_getfslog.c $(LIBS)
	$(CC) $? -o $@

do_getfsloginf: do_getfsloginf.c $(LIBS)
	$(CC) $? -o $@

test_syscalls: test_syscalls.c $(LIBS)
	$(CC) $? -o $@

syscall_apps: do_startfslog do_stopfslog do_getfslog do_getfsloginf

clean: clean_syscall_apps clean_test_syscalls
	rm -f *.o
	rm -f core*
	
clean_syscall_apps:
	rm -f do_startfslog
	rm -f do_stopfslog
	rm -f do_getfslog
	rm -f do_getfsloginf

clean_test_syscalls:
	rm -f test_syscalls
