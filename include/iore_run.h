#ifndef _IORE_RUN_H
#define _IORE_RUN_H

#include "util.h"
#include "iore_params.h"
#include "iore_results.h"

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

/* linked list of experiment runs */
typedef struct iore_run
{
  int id; /* unique ID */
  iore_params_t params; /* test parameters */
  iore_results_t results; /* results of all repetitions */
  struct iore_run *next; /* next run */
} iore_run_t;

#endif /* _IORE_RUN_H */
