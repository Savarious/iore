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

#endif /* _DISPLAY_H */
