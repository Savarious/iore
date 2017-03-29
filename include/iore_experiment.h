#ifndef _IORE_EXPERIMENT_H
#define _IORE_EXPERIMENT_H

#include "iore_run.h"

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

/* experiment queue */
typedef struct iore_experiment
{
  iore_run_t *front; /* first run */
  iore_run_t *rear; /* last run */
  int size; /* number of runs */
  verbosity_t verbosity; /* verbosity level */
} iore_experiment_t;

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

/*
 * Create an experiment from command line arguments.
 */
iore_experiment_t *new_experiment (int, char **);

#endif /* _IORE_EXPERIMENT_H */
