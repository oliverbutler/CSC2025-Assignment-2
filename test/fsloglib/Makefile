CC=cc -Wall
LIBS=fslog_testutil.o fsloglib.o

all: lib_apps test_fsloglib

fsloglib.o: ../../lib/fsloglib.c
	$(CC) -c $?

fslog_testutil.o: ../util/fslog_testutil.c
	$(CC) -c $?

initfslog: initfslog.c $(LIBS)
	$(CC) $? -o $@
    
startfslog: startfslog.c $(LIBS)
	$(CC) $? -o $@
    
stopfslog: stopfslog.c $(LIBS)
	$(CC) $? -o $@
    
getfslog: getfslog.c $(LIBS)
	$(CC) $? -o $@

getfsloginf: getfsloginf.c $(LIBS)
	$(CC) $? -o $@

test_fsloglib: test_fsloglib.c $(LIBS)
	$(CC) $? -o $@

lib_apps: initfslog startfslog stopfslog getfslog getfsloginf

clean: clean_lib_apps clean_test_lib
	rm -f *.o
	rm -f core*
	
clean_lib_apps:
	rm -f initfslog
	rm -f startfslog
	rm -f stopfslog
	rm -f getfslog
	rm -f getfsloginf

clean_test_lib:
	rm -f test_fsloglib
