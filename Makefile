#
# CMPSC311 - F21 Assignment #3
# Makefile - makefile for the assignment
#

# Make environment
INCLUDES=-I.
CC=./311cc
CFLAGS=-I. -c -g -Wall $(INCLUDES)
LINKARGS=-g
LIBS=-lm -lfs3lib -lcmpsc311 -L. -lgcrypt -lpthread -lcurl
                    
# Suffix rules
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS)  -o $@ $<
	
# Files
OBJECT_FILES=	fs3_sim.o \
				fs3_driver.o \
				fs3_cache.o \

# Productions
all : fs3_sim

fs3_sim : $(OBJECT_FILES)
	$(CC) $(LINKARGS) $(OBJECT_FILES) -o $@ -lfs3lib $(LIBS)

clean : 
	rm -f fs3_sim $(OBJECT_FILES)
	
test: fs3_sim 
	./fs3_sim -v assign3-workload.txt
