#include <stdio.h>
#include <stdlib.h>

#include "iore.h"
#include "ioreaio.h"

/*****************************************************************************
 * P R O T O T Y P E S                                                       *
 *****************************************************************************/

static void posix_create(char *, IORE_param_t *);
static void posix_open(char *, IORE_param_t *);
static IORE_offset_t posix_io(int, void *, IORE_size_t *, IORE_offset_t,
			      IORE_param_t *); 
static void posix_close(void *, IORE_param_t *);
static void posix_delete(char *, IORE_param_t *);

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
static void posix_create(char *file_name, IORE_param_t *p)
{
  /* TODO: implement */
}

/*
 * Open a file.
 *
 * file_name: full name of the file
 * p: test params
 */
static void posix_open(char *file_name, IORE_param_t *p)
{
  /* TODO: implement */
}

/*
 * Write/read data to/from a file.
 *
 * TODO: describe arguments
 */
static IORE_offset_t posix_io(int access, void *file, IORE_size_t *buf,
			      IORE_offset_t length, IORE_param_t *p)
{
  /* TODO: implement */
}

/*
 * Close a file.
 *
 * fd: file descriptor
 * p: test params
 */
static void posix_close(void *fd, IORE_param_t *p)
{
  /* TODO: implement */
}
/*
 * Remove a file.
 *
 * file_name: full name of the file
 * p: test params
 */
static void posix_delete(char *file_name, IORE_param_t *p)
{
  /* TODO: implement */
}
