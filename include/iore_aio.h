#ifndef _IORE_AIO_H
#define _IORE_AIO_H

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

/* abstract I/O interface */
typedef struct iore_aio
{
  char *name; /* abstract I/O name */

  void *
  (*create) (iore_params_t *);
  void *
  (*open) (iore_params_t *);
  void
  (*close) (void *, iore_params_t *);
  void
  (*delete) (iore_params_t *);
  iore_size_t
  (*io) (void *, iore_size_t *, iore_size_t, access_t, iore_params_t *);
} iore_aio_t;

#endif /* _IORE_AIO_H */
