#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

#include "iore.h"
#include "ioreaio.h"
#include "util.h"

/*****************************************************************************
 * D E F I N I T I O N S                                                     *
 *****************************************************************************/

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_DIRECT
#ifndef O_DIRECTIO
#define O_DIRECT 000000
#else /* O_DIRECTIO */
#define O_DIRECT O_DIRECTIO
#endif 
#endif /* O_DIRECT */

#ifndef open64
#define open64 open /* unlike, but may pose */
#endif

#ifndef lseek64
#define lseek64 lseek /* unlike, but my pose */
#endif

/*****************************************************************************
 * P R O T O T Y P E S                                                       *
 *****************************************************************************/

static void *posix_create(char *, IORE_param_t *);
static void *posix_open(char *, IORE_param_t *);
static IORE_offset_t posix_io(enum ACCESS, void *, IORE_size_t *, IORE_offset_t,
			      IORE_param_t *, int); 
static void posix_close(void *, IORE_param_t *);
static void posix_delete(char *, int, IORE_param_t *);
static void posix_fsync(void *, IORE_param_t *);

/*****************************************************************************
 * V A R I A B L E S                                                         *
 *****************************************************************************/

IORE_aio_t ioreaio_posix = {
  "POSIX",
  posix_create,
  posix_open,
  posix_io,
  posix_close,
  posix_delete,
  posix_fsync
};

/*****************************************************************************
 * F U N C T I O N S                                                         *
 *****************************************************************************/

/*
 * Create a file.
 *
 * file_name: full name of the file
 * p: test params
 */
static void *posix_create(char *file_name, IORE_param_t *p)
{
  int oflag = O_BINARY | O_CREAT | O_RDWR;
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  int *fd;
  
  fd = (int *) malloc(sizeof(int));
  if (fd == NULL) {
    FATAL("Unable to allocate memory to file descriptor.");
  }
  
  if (p->use_o_direct) {
    oflag |= O_DIRECT;
  }
  
  *fd = open64(file_name, oflag, mode);
  if (*fd < 0) {
    FATAL("Could not create the test file");
  }
  
  return ((void *) fd);
}
 
/*
 * Open a file.
 *
 * file_name: full name of the file
 * p: test params
 */
static void *posix_open(char *file_name, IORE_param_t *p)
{
  int oflag = O_BINARY | O_RDWR;
  int *fd;
  
  fd = (int *) malloc(sizeof(int));
  if (fd == NULL) {
    FATAL("Unable to allocate memory to file descriptor.");
  }
  
  if (p->use_o_direct) {
    oflag |= O_DIRECT;
  }
  
  *fd = open64(file_name, oflag);
  if (*fd < 0) {
    FATAL("Could not create the test file");
  }
  
  return ((void *) fd);
}

/*
 * Write/read data to/from a file.
 *
 * access: type of access
 * file: file pointer
 * buf: buffer for read/write
 * length: amount of data
 * p: test parameters
 * rank: MPI rank
 */
static IORE_offset_t posix_io(enum ACCESS access, void *file, IORE_size_t *buf,
			      IORE_offset_t length, IORE_param_t *p, int rank)
{
  long long n;
  int retries = 0;
  int fd = *(int *) file;
  long long remainder = (long long) length;
  char *b = (char *) buf;
  enum VERBOSE verbose = p->verbose;
  
  if (lseek64(fd, p->offset, SEEK_SET) == -1) {
    FATAL("File offset reposition failed.");
  }
  
  while (remainder > 0) {
    if (access == WRITE) {
      if (verbose >= VERBOSE_4) {
	INFOF("Task %d writing to offset %lld\n", rank,
	     p->offset + length - remainder);
      }
      n = write(fd, b, remainder);
      if (n == -1) {
	FATAL("write() failed");
      }
      if (p->fsync_per_write == TRUE) {
	posix_fsync(&fd, p);
      }
    } else { /* READ or CHECK */
      if (verbose >= VERBOSE_4) {
	INFOF("Task %d reading from offset %lld\n", rank,
	     p->offset + length - remainder);
      }
      n = read(fd, b, remainder);
      if (n == 0) {
	FATAL("read() returned EOF prematurely.");
      }
      if (n == -1) {
	FATAL("read() failed");
      }
    }
    
    if (n < remainder) {
      WARNF("Task %d, partial %s, %lld of %lld bytes at offset %lld.\n",
	    verbose, rank, access == WRITE ? "write()" : "read()", n,
	    remainder, p->offset + length - remainder);
      if (p->single_io_attempt) {
        FATAL("Single I/O attempt option defined; aborting test.");
      }
      if (retries > MAX_RETRY) {
        FATAL("Too many retries; aborting test.");
      }
    }
    
    /* assert correct conditions */
    assert(n >= 0);
    assert(n <= remainder);
    remainder -= n;
    b += n;
    retries++;
  }
  
  return (length);
}

/*
 * Close a file.
 *
 * fd: file descriptor
 * p: test parameters
 */
static void posix_close(void *fd, IORE_param_t *p)
{
  if (close(*(int *) fd) != 0) {
    FATAL("Could not close the test file");
  }
  free(fd);
}

/*
 * Remove a file.
 *
 * file_name: full name of the file
 * rank: MPI rank
 * p: test parameters
 */
static void posix_delete(char *file_name, int rank, IORE_param_t *p)
{
  if (unlink(file_name) != 0) {
    char msg[256];
    sprintf(msg, "[RANK %03d]: Unable to unlink file \"%s\".\n", rank,
	    file_name);
    ERR(msg, p->verbose);
  }  
}

/*
 * Synchronize file data in memory to disk.
 *
 * fd: file descriptor
 * p: test parameters
 */
static void posix_fsync(void *fd, IORE_param_t *p)
{
  if (fsync(*(int *) fd) != 0) {
    ERR("fsync() failed", p->verbose);
  }
}
