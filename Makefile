
CC      = gcc
CFLAGS  = -g -lrt
TARGET1 = oss
TARGET2 = user
OBJS1   = oss.o project4.h
OBJS2   = user.o project4.h

.SUFFIXES: .c .o

all: $(TARGET1) $(TARGET2)

oss: $(OBJS1)
	$(CC) $(CFLAGS) $(OBJS1) -o $@
    
user: $(OBJS2)
	$(CC) $(CFLAGS) $(OBJS2) -o $@
    
.c.o:
	$(CC) $(CFLAGS) -c $<
    
.PHONY: clean

clean:
	/bin/rm -f *.log *.o *~ $(TARGET1) $(TARGET2)
