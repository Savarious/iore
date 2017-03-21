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

#ifndef open64
#define open64 open /* unlike, but may pose */
#endif

#ifndef lseek64
#define lseek64 lseek /* unlike, but my pose */
#endif

/*****************************************************************************
 * P R O T O T Y P E S                                                       *
 *****************************************************************************/

static void *
posix_create (IORE_params_t *);
static void *
posix_open (IORE_params_t *);
static IORE_size_t
posix_io (void *, IORE_size_t *, IORE_size_t, enum ACCESS_TYPE, IORE_params_t *);
static void
posix_close (void *, IORE_params_t *);
static void
posix_delete (IORE_params_t *);

/*****************************************************************************
 * D E C L A R A T I O N S 						     *
 *****************************************************************************/
extern int rank;

IORE_aio_t ioreaio_posix =
  { "POSIX", posix_create, posix_open, posix_io, posix_close, posix_delete };

/*****************************************************************************
 * F U N C T I O N S                                                         *
 *****************************************************************************/

/*
 * Create a file.
 */
static void *
posix_create (IORE_params_t *params)
{
  char *file_name = params->file_name;

  int oflag = O_BINARY | O_CREAT | O_RDWR;
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  int *fd;

  fd = (int *) malloc (sizeof(int));
  if (fd == NULL)
    FATAL("Failed to allocate memory to file descriptor.");

  *fd = open64 (file_name, oflag, mode);
  if (*fd < 0)
    FATAL("Could not create the test file");

  return ((void *) fd);
} /* posix_create (IORE_params_t *params) */

/*
 * Open a file.
 */
static void *
posix_open (IORE_params_t *params)
{
  char *file_name = params->file_name;

  int oflag = O_BINARY | O_RDWR;
  int *fd;

  fd = (int *) malloc (sizeof(int));
  if (fd == NULL)
    FATAL("Failed to allocate memory to file descriptor.");

  *fd = open64 (file_name, oflag);
  if (*fd < 0)
    FATAL("Could not create the test file");

  return ((void *) fd);
} /* posix_open (IORE_params_t *) */

/*
 * Write/read data to/from a file.
 */
static IORE_size_t
posix_io (void *file, IORE_size_t *buf, IORE_size_t length,
	  enum ACCESS_TYPE access, IORE_params_t *params)
{
  long long n;
  int retries = 0;
  int fd = *(int *) file;
  long long remainder = (long long) length;
  char *b = (char *) buf;
  enum VERBOSE verbose = params->verbose;

  if (lseek64 (fd, params->offset, SEEK_SET) == -1)
    FATAL("File offset reposition failed.");

  while (remainder > 0)
    {
      if (access == WRITE)
	{
	  if (verbose >= DEBUG)
	    INFOF("Task %d writing to offset %lld\n", rank,
		  params->offset + length - remainder);

	  n = write (fd, b, remainder);
	  if (n == -1)
	    FATAL("Failed to write to file.");
	}
      else /* READ */
	{
	  if (verbose >= DEBUG)
	    INFOF("Task %d reading from offset %lld\n", rank,
		  params->offset + length - remainder);

	  n = read (fd, b, remainder);
	  if (n == 0)
	    FATAL("read() returned EOF prematurely.");
	  if (n == -1)
	    FATAL("Failed to read from file.");
	}

      if (n < remainder)
	{
	  WARNF("Task %d, partial %s, %lld of %lld bytes at offset %lld.\n",
		verbose, rank, access == WRITE ? "write()" : "read()", n,
		remainder, params->offset + length - remainder);
	  if (params->single_io_attempt)
	    FATAL("Single I/O attempt option defined; aborting test.");

	  if (retries > MAX_RETRY)
	    FATAL("Too many retries; aborting test.");
	}

      /* assert correct conditions */
      assert(n >= 0);
      assert(n <= remainder);
      remainder -= n;
      b += n;
      retries++;
    }

  return (length);
} /* posix_io (void *, IORE_size_t *, IORE_size_t, enum ACCESS_TYPE, ...) */

/*
 * Close a file.
 */
static void
posix_close (void *fd, IORE_params_t *params)
{
  if (close (*(int *) fd) != 0)
    FATAL("Could not close the test file");
  free (fd);
} /* posix_close (void *, IORE_params_t *)

 /*
 * Remove a file.
 */
static void
posix_delete (IORE_params_t *params)
{
  char *file_name = params->file_name;
  enum VERBOSITY verbose = params->verbose;

  if (unlink (file_name) != 0)
    {
      char msg[256];
      sprintf (msg, "[RANK %03d]: Failed to unlink file \"%s\".\n", rank,
	       file_name);
      ERR(msg);
    }
} /* posix_delete (IORE_params_t *) */
