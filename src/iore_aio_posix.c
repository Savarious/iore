#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <mpi.h>

#include "iore_aio.h"
#include "iore_params.h"
#include "iore_task.h"

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef open64
#define open64 open /* unlike, but may pose */
#endif

#ifndef lseek64
#define lseek64 lseek /* unlike, but my pose */
#endif

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

static void *posix_create (iore_params_t *);
static void *posix_open (iore_params_t *);
static void posix_close (void *, iore_params_t *);
static void posix_delete (iore_params_t *);
static iore_size_t posix_io (void *, iore_size_t *, iore_size_t, iore_offset_t,
			     access_t, iore_params_t *);

/******************************************************************************
 * D E C L A R A T I O N S
 ******************************************************************************/

iore_aio_t iore_aio_posix =
  { "POSIX", posix_create, posix_open, posix_close, posix_delete, posix_io };

/*****************************************************************************
 * G L O B A L S
 *****************************************************************************/

extern iore_task_t *task;

/******************************************************************************
 * F U N C T I O N S
 ******************************************************************************/

static void *
posix_create (iore_params_t *params)
{
  int *fd;
  int oflag = O_BINARY | O_CREAT | O_RDWR;
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  fd = (int *) malloc (sizeof(int));
  if (fd == NULL)
    FATAL("Failed to allocate memory to file descriptor");

  *fd = open64 (task->test_file_name, oflag, mode);
  if (*fd < 0)
    FATAL("Could not create the test file");

  return ((void *) fd);
} /* posix_create (iore_params_t *) */

static void *
posix_open (iore_params_t *params)
{
  int *fd;
  int oflag = O_BINARY | O_RDWR;

  fd = (int *) malloc (sizeof(int));
  if (fd == NULL)
    FATAL("Failed to allocate memory to file descriptor");

  *fd = open64 (task->test_file_name, oflag);
  if (*fd < 0)
    FATAL("Could not open the test file");

  return ((void *) fd);
} /* posix_open (iore_params_t *) */

static void
posix_close (void *fd, iore_params_t *params)
{
  if (close (*(int *) fd) != 0)
    FATAL("Could not close the test file");
  free (fd);
} /* posix_close (void *, iore_params_t *) */

static void
posix_delete (iore_params_t *params)
{
  if (unlink (task->test_file_name) != 0)
    ERRF("Task %d failed to unlink file \"%s\"", task->rank,
	 task->test_file_name);
} /* posix_delete (iore_params_t *) */

static iore_size_t
posix_io (void *file, iore_size_t *buffer, iore_size_t length,
	  iore_offset_t offset, access_t access, iore_params_t *params)
{
  iore_size_t remaining = length;
  char *buf = (char *) buffer;
  int fd = *(int *) file;
  int retries = 0;
  iore_size_t n;

  if (lseek64 (fd, offset, SEEK_SET) == -1)
    {
      FATAL("Failed seeking file offset");
      MPI_Abort (MPI_COMM_WORLD, -1);
    }

  while (remaining > 0)
    {
      if (access == WRITE)
	{
	  n = write (fd, buf, remaining);
	  if (n == -1)
	    {
	      FATAL("Failed to write to file");
	      MPI_Abort (MPI_COMM_WORLD, -1);
	    }
	}
      else /* READ */
	{
	  n = read (fd, buf, remaining);
	  if (n == 0)
	    {
	      FATAL("read() returned EOF prematurely");
	      MPI_Abort (MPI_COMM_WORLD, -1);
	    }
	  else if (n == -1)
	    {
	      FATAL("Failed to read from file");
	      MPI_Abort (MPI_COMM_WORLD, -1);
	    }
	}

      if (n < remaining)
	{
	  WARNF("Task %d partially %s %lld of %lld bytes at offset %lld",
		task->rank, (access == WRITE ? "wrote" : "read"), n,
		remaining, offset + length - remaining);

	  if (params->single_io_attempt)
	    {
	      FATAL("Single I/O attempt option defined; aborting");
	      MPI_Abort (MPI_COMM_WORLD, -1);
	    }

	  if (retries > MAX_RETRIES)
	    {
	      FATAL("Too many retries; aborting");
	      MPI_Abort (MPI_COMM_WORLD, -1);
	    }
	}

      remaining -= n;
      buf += n;
      retries++;
    }
  
  return (length);
} /* posix_io (void *, iore_size_t *, iore_size_t, iore_offset_t, ... *) */
