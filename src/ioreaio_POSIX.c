#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

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
static IORE_offset_t posix_io(int, void *, IORE_size_t *, IORE_offset_t,
			      IORE_param_t *); 
static void posix_close(void *, IORE_param_t *);
static void posix_delete(char *, int, IORE_param_t *);

/*****************************************************************************
 * V A R I A B L E S                                                         *
 *****************************************************************************/

IORE_aio_t ioreaio_posix = {
  "POSIX",
  posix_create,
  posix_open,
  posix_io,
  posix_close,
  posix_delete
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
  
  return ((void *)fd);
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

  return ((void *)fd);
}

/*
 * Write/read data to/from a file.
 *
 * TODO: describe arguments
 */
static IORE_offset_t posix_io(int access, void *file, IORE_size_t *buf,
			      IORE_offset_t length, IORE_param_t *p)
{
  /* TODO: continue... */
}

/*
 * Close a file.
 *
 * fd: file descriptor
 * p: test params
 */
static void posix_close(void *fd, IORE_param_t *p)
{
  if (close(*(int *)fd) != 0) {
    FATAL("Could not close the test file");
  }
  free(fd);
}

/*
 * Remove a file.
 *
 * file_name: full name of the file
 * rank: MPI rank
 * p: test params
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
