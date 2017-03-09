# Directories
export BINDIR = $(CURDIR)/bin
export INCDIR = $(CURDIR)/include
export SRCDIR = $(CURDIR)/src

# Toolchain
export CC = mpicc

# Toolchain configuration
export CFLAGS += -I $(INCDIR)
export CFLAGS += -Wall -Wextra -Werror

# Executable file
export EXEC = iore

# Builds everything
all:
	mkdir -p $(BINDIR)
	$(CC) $(SRCDIR)/*.c $(CFLAGS) -o $(BINDIR)/$(EXEC)

# Cleans compilation files
clean:
	rm -f $(BINDIR)/$(EXEC)
