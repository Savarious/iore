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
 * P R O T O T Y P E S
 ******************************************************************************/
static void *mpiio_create (iore_params_t *);
static void *mpiio_open (iore_params_t *);
static void mpiio_close (void *, iore_params_t *);
static void mpiio_delete (iore_params_t *);
static iore_size_t mpiio_io (void *, iore_size_t *, iore_size_t, iore_offset_t,
			     access_t, iore_params_t *);

/******************************************************************************
 * D E C L A R A T I O N S
 ******************************************************************************/

iore_aio_t iore_aio_mpiio =
  { "MPIIO", mpiio_create, mpiio_open, mpiio_close, mpiio_delete, mpiio_io };

/*****************************************************************************
 * G L O B A L S
 *****************************************************************************/

extern iore_task_t *task;

/******************************************************************************
 * F U N C T I O N S
 ******************************************************************************/

static void *
mpiio_create (iore_params_t *params)
{
	MPI_File *fd;
	int oflag = MPI_MODE_CREATE | MPI_MODE_RDWR;
	
	fd = (MPI_File *) malloc (sizeof(MPI_File));
	if (fd == NULL)
  	FATAL("Failed to allocate memory to file descriptor");
 
	MPI_TRYCATCH(MPI_File_open(MPI_COMM_SELF, task->test_file_name, oflag, MPI_INFO_NULL, fd), 
		"Could not create the test file");
	
	return ((void *) fd);
} /* mpiio_create (iore_params_t *) */

static void *
mpiio_open (iore_params_t *params)
{
	MPI_File *fd;
	int oflag = MPI_MODE_RDWR; 

	fd = (MPI_File *) malloc (sizeof(MPI_File));
	if (fd == NULL)
  	FATAL("Failed to allocate memory to file descriptor");

	MPI_TRYCATCH(MPI_File_open(MPI_COMM_SELF, task->test_file_name, oflag, MPI_INFO_NULL, fd), 
		"Could not open the test file");

  return ((void *) fd);
} /* mpiio_open (iore_params_t *) */

static void
mpiio_close (void *fd, iore_params_t *params)
{
  MPI_TRYCATCH(MPI_File_close(fd),
  	"Could not close the test file");
  free (fd);
} /* mpiio_close (void *, iore_params_t *) */

static void
mpiio_delete (iore_params_t *params)
{ 
  MPI_TRYCATCH(MPI_File_delete(task->test_file_name, MPI_INFO_NULL),
  	"Could not delete the test file");
} /* mpiio_delete (iore_params_t *) */

static iore_size_t 
mpiio_io (void *file, iore_size_t *buffer, iore_size_t length,
	  iore_offset_t offset, access_t access, iore_params_t *params)
{
  iore_size_t remaining = length;
  iore_size_t max, limit; 
  iore_size_t data_transferred;
  char *buf = (char *) buffer;
  MPI_File *fd = (MPI_File *) file;
  MPI_Offset mpi_offset = (MPI_Offset) offset;
  int retries = 0;
  MPI_Status status;
  

  max = string_to_bytes("2G");
  limit = max - string_to_bytes("1K"); /* Maximum data writable/readable by the functions */

  if (remaining >= max) /* More than 2 GiB of data */ 
  {
  		WARNF("Too much data for the task. Task %d will partially %s %lld of %lld bytes",
		task->rank, (access == WRITE ? "write" : "read"), limit, remaining);
  		remaining = limit;
  		
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
  	
  MPI_TRYCATCH(MPI_File_seek(*fd, mpi_offset, MPI_SEEK_SET),
  	"Failed seeking file offset");

  while (remaining > 0)
  {
    if (access == WRITE)
		{
	  	MPI_TRYCATCH(MPI_File_write(*fd, buf, remaining, MPI_BYTE, &status),
	  		"Failed to write to file");
		}
    else /* READ */
		{
	  	MPI_TRYCATCH(MPI_File_read (*fd, buf, remaining, MPI_BYTE, &status),
	  		"Failed to read from file");
		}

		MPI_Get_count( &status, MPI_BYTE, &data_transferred);
		
    remaining -= data_transferred;
    buf += data_transferred;
    retries++;
  }
  
  return (length);
} /* mpiio_io (void *, iore_size_t *, iore_size_t, iore_offset_t, ... *) */


