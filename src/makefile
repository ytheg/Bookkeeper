# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -O2 -g
#CFLAGS = 

# Static library name
LIBRARY = libdbms.a

# Object files
OBJS = btree.o dfile.o logfile.o dml.o ddl.o

# Targets
all: dbinit bookkeeper

# Rule to create the static library
$(LIBRARY): btree.o dfile.o logfile.o dml.o
	ar rcs $(LIBRARY) btree.o dfile.o logfile.o dml.o

# Rule to compile dbmscc
dbmscc: dcompiler.c
	$(CC) $(CFLAGS) -o dbmscc dcompiler.c

# Rule to run dbmscc (assuming it processes .sch files and generates ddl.o)
run_dbmscc: dbmscc
	./dbmscc *.sch

# Rule to compile ddl.o (depends on dbmscc being run)
ddl.o: run_dbmscc ddl.c
	$(CC) $(CFLAGS) -c ddl.c -o ddl.o

# Rule to compile dbinit (depends on ddl.o and the static library)
dbinit: dbinit.c ddl.o $(LIBRARY)
	$(CC) $(CFLAGS) -o dbinit dbinit.c ddl.o -L. -ldbms

# Rule to compile program (depends on program.c, ddl.o, and the static library)
bookkeeper: program.c ddl.o $(LIBRARY)
	$(CC) $(CFLAGS) -o bookkeeper program.c ddl.o -lncurses -lpanel -L. -ldbms

# Rule to compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f *.o $(LIBRARY) dbmscc dbinit bookkeeper ddl.h ddl.c

# Phony targets
.PHONY: all clean run_dbmscc
