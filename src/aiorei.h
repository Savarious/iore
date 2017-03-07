/******************************************************************************\
 * IORE - The IOR-Extended Benchmark
 * 
 * Author: Camilo <eduardo.camilo@posgrad.ufsc.br>
 * Creation date: 2017-03-07
 * License: GPLv3
 * 
 * Distributed Systems Research Laboratory (LAPESD)
 * Federal University of Santa Catarina (UFSC)
\******************************************************************************/

#ifndef _AIOREI_H
#define _AIOREI_H

#include <mpi.h>

#ifndef MPI_FILE_NULL
#include <mpio.h>
#endif

#include "iore.h"
#include "ioredef.h"

/**** DEFINITIONS ****/
#define IOR_RDONLY 1
#define IOR_WRONLY 2
#define IOR_RDWR 4
#define IOR_APPEND 8
#define IOR_CREAT 16
#define IOR_TRUNC 32
#define IOR_EXCL 64
#define IOR_DIRECT 128

#define IOR_IRWXU 1 /* 700 */
#define IOR_IRUSR 2 /* 400 */
#define IOR_IWUSR 4 /* 200 */
#define IOR_IXUSR 8 /* 100 */
#define IOR_IRWXG 16 /* 070 */
#define IOR_IRGRP 32 /* 040 */
#define IOR_IWGRP 64 /* 020 */
#define IOR_IXGRP 128 /* 010 */
#define IOR_IRWXO 256 /* 007 */
#define IOR_IROTH 512 /* 004 */
#define IOR_IWOTH 1024 /* 002 */
#define IOR_IXOTH 2048 /* 001 */

/**** CUSTOM TYPES ****/
typedef struct iore_aiorei {
  char *name;
  void *(*create)(char *, IORE_param_t *);
  void *(*open)(char *, IORE_param_t *);
  IORE_offset_t (*xfer)(int, void *, IORE_size_t *, IORE_offset_t,
			IORE_param_t *);
  void (*close)(void *, IORE_param_t *);
  void (*delete)(char *, IORE_param_t *);
  void (*fsync)(void *, IORE_param_t *);
  IORE_offset_t (*get_file_size)(IORE_param_t *, MPI_Comm, char *);
  /* TODO: check this function usage */
  /*
  void (*set_version)(IORE_param_t *);
  */
} iore_aiorei_t;

iore_aiorei_t posix_aiorei;
/* TODO: reserved for future extensions */
/*
iore_aiorei_t mpiio_aiorei;
iore_aiorei_t hdf5_aiorei;
iore_aiorei_t ncmpi_aiorei;
*/

#endif /*_AIOREI_H */
