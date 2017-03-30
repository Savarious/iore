#include "iore_aio.h"
#include "iore_params.h"

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

static void *posix_create (iore_params_t *);
static void *posix_open (iore_params_t *);
static void posix_close (void *, iore_params_t *);
static void posix_delete (iore_params_t *);
static iore_size_t posix_io (void *, iore_size_t *, iore_size_t, access_t,
			     iore_params_t *);

/******************************************************************************
 * D E C L A R A T I O N S
 ******************************************************************************/

iore_aio_t iore_aio_posix =
  { "POSIX", posix_create, posix_open, posix_close, posix_delete, posix_io };

/******************************************************************************
 * F U N C T I O N S
 ******************************************************************************/

static void *
posix_create (iore_params_t *params)
{
  /* TODO: implement */
  return (NULL);
} /* posix_create (iore_params_t *) */

static void *
posix_open (iore_params_t *params)
{
  /* TODO: implement */
  return (NULL);
} /* posix_open (iore_params_t *) */

static void
posix_close (void *fd, iore_params_t *params)
{
  /* TODO: implement */
} /* posix_close (void *, iore_params_t *) */

static void
posix_delete (iore_params_t *params)
{
  /* TODO: implement */
} /* posix_delete (iore_params_t *) */

static iore_size_t
posix_io (void *file, iore_size_t *buf, iore_size_t length, access_t access,
	  iore_params_t *params)
{
  /* TODO: implement */
  return (0);
} /* posix_io (void *, iore_size_t *, iore_size_t, access_t, iore_params_t *) */
