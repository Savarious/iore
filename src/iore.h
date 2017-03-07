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

#ifndef _IORE_H
#define _IORE_H

/* TODO: check the need for this code snippet */
/* 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
*/

#include "ioredef.h"

/**** DECLARATIONS ****/
/* TODO: check the need for this code snippet */
/*
extern int numTasksWorld;
extern int rank;
extern int rankOffset;
extern int tasksPerNode;
extern int verbose;
extern MPI_Comm testComm;
*/

/**** CUSTOM TYPES ****/
/* TODO: should these typedefs be in ioredef.h? */
typedef struct
{
  /* TODO: include attributes by usage */
} IORE_param_t;

/* 
 * each pointer is to an array, each of length equal to the number of 
 * repetitions in the test
 */
typedef struct
{
  double *writeTime;
  double *readTime;
  IORE_offset_t *aggFileSizeFromStat;
  IORE_offset_t *aggFileSizeFromXfer;
  IORE_offset_t *aggFileSizeForBW;
} IORE_results_t;

/* queueing structure for test parameters */
typedef struct IORE_test_t
{
  IORE_param_t params;
  IORE_results_t *results;
  struct IORE_test_t *next;
} IORE_test_t;

/**** FUNCTIONS ****/
IORE_test_t *create_test(IORE_param_t *init_params, int test_num);
void alloc_results(IORE_test_t *test);
void get_platform_name(char *);
void init_IORE_Param_t(IORE_param_t *params);

#endif /* _IORE_H */
