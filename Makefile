# 
# 
# 'make'		make executable file 'ms'
# 'make clean'	removes all .o and executable files
#

# Compiler
CC=gcc

# Compilation flags
CFLAGS=-O3 -lm -I.

# define any libraries to link into executable:
LIBS=-lm

# Dependencies
DEPS=ms.h

# Object files
OBJ=ms.o streec.o

# Random functions using drand48()
RND_48=rand1.c

# Random functions using rand()
RND=rand2.c

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

default: ms

# download: packages
#	wget http://www.open-mpi.org/software/ompi/v1.8/downloads/openmpi-1.8.2.tar.gz
#	tar -xf openmpi-1.8.2.tar.gz -C $(CURDIR)/packages

#packages:
#	mkdir packages

# The .PHONY rule keeps make from doing something with a file named 'clean'
.PHONY: clean

clean:
	rm -f *.o *~ ms *.exe
	@echo ""
	@echo "*** All resources were cleaned-up ***"
	@echo ""

ms: $(OBJ) 
	$(CC) $(CFLAGS) -o $@ $^ $(RND_48) $(LIBS)
	@echo ""
	@echo "*** make complete: generated executable 'ms' ***"
