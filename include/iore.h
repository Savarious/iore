#ifndef _IORE_H
#define _IORE_H

/*****************************************************************************
 * T Y P E S   A N D   S T R U C T S                                         *
 *****************************************************************************/

typedef struct {
  
} IORE_param_t;

typedef struct {
  
} IORE_results_t;

typedef struct IORE_test_t {
  IORE_param_t params;
  IORE_results_t *results;
  struct IORE_test_t *next;
} IORE_test_t;

#endif /* _IORE_H */
