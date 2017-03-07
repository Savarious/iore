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

/* TODO: check the need for this code snippet */
/*
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>              /* tolower() */
#include <errno.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <sys/stat.h>           /* struct stat */
#include <time.h>
#include <sys/time.h>           /* gettimeofday() */
#include <sys/utsname.h>        /* uname() */
#include <assert.h>

#include "iore.h"
#include "aiorei.h"
#include "utilities.h"
#include "parse_options.h"
