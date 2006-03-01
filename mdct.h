/****************************************************************

  M D C T . H

****************************************************************/

#define MDCT_MIN_EXP2 4
#define MDCT_MAX_EXP2 10

int mdct_init(unsigned int);
void mdct_cleanup();
void mdct(unsigned int, const double *, double *);
void imdct(unsigned int, const double *, double *);

/***************************************************************/
