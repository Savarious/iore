# Directories
export BINDIR = $(CURDIR)/bin
export INCDIR = $(CURDIR)/include
export SRCDIR = $(CURDIR)/src

# Meta variables
VERSION := $(shell cat $(SRCDIR)/META)

# Toolchain
export CC = mpicc

# Toolchain configuration
export CFLAGS += -I $(INCDIR)
export CFLAGS += -Wall -Wextra
export CFLAGS += -lm
export CFLAGS += -DUSE_POSIX_AIO -DMETA_VERSION=$(VERSION)

# Executable file
export EXEC = iore

# Builds everything
all:
	mkdir -p $(BINDIR)
	$(CC) $(SRCDIR)/*.c $(CFLAGS) -o $(BINDIR)/$(EXEC)

# Debug
debug: CFLAGS += -g
debug: all

# Cleans compilation files
clean:
	rm -f $(BINDIR)/$(EXEC)
