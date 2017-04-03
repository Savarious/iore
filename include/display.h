#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "iore_params.h"

/*****************************************************************************
 * P R O T O T Y P E S                                                       
 *****************************************************************************/

void display_version();
void display_usage(char **);
void display_splash();
void display_expt_header(int, char **);
void display_run_info(int, iore_params_t *);
void display_params(iore_params_t *);
void display_fs_info(char *);
void display_rep_header();
void display_test_results (access_t, int);
void display_per_task_results (access_t, int);

#endif /* _DISPLAY_H */
