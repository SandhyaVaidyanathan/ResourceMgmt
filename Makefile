CC	= gcc
TARGETS	= oss user 
OBJS	= oss.o user.o
HEADER = shm.h
LDFLAGS = -pthread

all: $(TARGETS)

$(TARGETS): % : %.o
		$(CC) -o $@ $< $(LDFLAGS)

$(OBJS) : %.o : %.c
		$(CC) -c $< $(LDFLAGS)

clean:
		/bin/rm -f *.o $(TARGETS) *.log *.out

cleanobj: 
		/bin/rm -f *.o
