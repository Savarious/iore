#ifndef _IORE_RESULTS_H
#define _IORE_RESULTS_H

#include "util.h"

/******************************************************************************
 * D E F I N I T I O N S
 ******************************************************************************/

/* results of all repetitions of an experiment */
typedef struct iore_results
{
  iore_time_t **write_time; /* time of the write test for each task */
  iore_time_t **read_time; /* time of the read test for each task */
  iore_time_t *write_total_time; /* time for all tasks to complete write test */
  iore_time_t *read_total_time; /* time for all tasks to complete read test */
  iore_size_t **data_written; /* amount of data written by each task */
  iore_size_t **data_read; /* amount of data read by each task */
  iore_size_t *total_data_written; /* total amount of data written */
  iore_size_t *total_data_read; /* total amount of data read */
} iore_results_t;

#endif /* _IORE_RESULTS_H */
