#ifndef _IOREAIO_H
#define _IOREAIO_H

/*****************************************************************************
 * T Y P E S   A N D   S T R U C T S                                         *
 *****************************************************************************/

typedef struct IORE_aio_t {
  char *name;
  void (*create)(char *, IORE_param_t *);
  void (*open)(char *, IORE_param_t *);
  IORE_offset_t (*io)(int, void *, IORE_size_t *, IORE_offset_t,
		      IORE_param_t *);
  void (*close)(void *, IORE_param_t *);
  void (*delete)(char *, IORE_param_t *);
} IORE_aio_t;

/*****************************************************************************
 * D E C L A R A T I O N S                                                   *
 *****************************************************************************/

IORE_aio_t ioreaio_posix;

#endif /* _IOREAIO_H */
