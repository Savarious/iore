#ifndef _PARSE_FILE_H
#define _PARSE_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "iore_experiment.h"
#include "iore_run.h"
#include "iore_params.h"
#include "util.h"

#include "json.h"

/******************************************************************************
 * P R O T O T Y P E S
 ******************************************************************************/

void parse_file (char *, iore_experiment_t *);

#endif /* _PARSE_FILE_H */
