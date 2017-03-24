#ifndef _IORE_PARAMS_H
#define _IORE_PARAMS_H

#include <sys/param.h> /* MAXPATHLEN */

#include "util.h"

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

/* parameters passed by the user to define an experiment */
typedef struct iore_params
{
  int ref_num; /* custom experiment reference number */

  int num_tasks; /* number of parallel MPI tasks */
  char api[MAX_STR_LEN]; /* name of the API used for I/O */
  sharing_policy_t sharing_policy; /* policy for file sharing among tasks */
} iore_params_t;

#endif /* _IORE_PARAMS_H */
