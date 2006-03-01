/****************************************************************

  M D C T . C

****************************************************************/

#include <stdlib.h>
#include <math.h>

#include "mdct.h"

static int global_buffers_initialized = 0;
static double *sin_win_buffer[MDCT_MAX_EXP2 + 1];
static double *cos_buffer[MDCT_MAX_EXP2 + 1];

//#define DEBUG
//#define MDCT_DEBUG_PEAK

#ifdef DEBUG
#include <stdio.h>
#endif
 
/****************************************************************

  mdct_init

  Initialisiert die Puffer f�r eine Dimension, die durch eine
  Zweierpotenz gegeben ist.

  Parameter: exp2 - Zweierexponent der Dimension

  R�ckgabe: 0 bei Erfolg, sonst negativer Fehlercode

****************************************************************/

int mdct_init(unsigned int exp2)
{
  unsigned int i, j, dim;

  /* Dimension muss im g�ltigen Bereich liegen! */
  if (exp2 < MDCT_MIN_EXP2 || exp2 > MDCT_MAX_EXP2) return -1;

  /* Dimension berechnen */
  dim = 1 << exp2;

#ifdef DEBUG
  printf("MDCT: init %d\n", dim);
#endif

  /* Beim ersten Aufruf die globalen Puffer initialisieren */
  if (!global_buffers_initialized)
  {
#ifdef DEBUG
    printf("MDCT: initializing global buffers\n");
#endif

    for (i = MDCT_MIN_EXP2; i <= MDCT_MAX_EXP2; i++)
    {
      sin_win_buffer[i] = 0;
      cos_buffer[i] = 0;
    }

    global_buffers_initialized = 1;
  }

  if (!sin_win_buffer[exp2]) /* Puffer mit Fensterfunktion noch nicht erzeugt */
  {
#ifdef DEBUG
  printf("MDCT: creating sin buffer for dim %d\n", dim);
#endif

    /* Speicher f�r Sinus-Fensterfunktion reservieren */
    if ((sin_win_buffer[exp2] = (double *) malloc(dim * sizeof(double))) == NULL) return -3;

    /* Speicher mit Sinus-Fensterfunktion f�llen */
    for (i = 0; i < dim; i++)
    {
      sin_win_buffer[exp2][i] = sin(M_PI * (i + 0.5) / dim);
    }
  }

  if (!cos_buffer[exp2]) /* Puffer mit Cosinuswerten noch nicht erzeugt */
  {
#ifdef DEBUG
  printf("MDCT: creating cos buffer for dim %d\n", dim);
#endif

    /* Speicher f�r Cosinuswerte reservieren */
    if ((cos_buffer[exp2] = (double *) malloc(dim * dim / 2 * sizeof(double))) == NULL) return -4;

    /* Speicher mit vorberechneten Cosinus-Werten f�llen */
    for (i = 0; i < dim / 2; i++)
    { 
      for (j = 0; j < dim; j++)
      {
        cos_buffer[exp2][i * dim + j] = cos(M_PI * (2.0 * j + dim / 2 + 1.0) * (2.0 * i + 1.0) / 2.0 / dim);
      }
    }
  }

  return 0;
}

/****************************************************************

  mdct_cleanup

  L�scht alle allokierten Speicherbereiche.

  Diese Funktion sollte nur vor beendigung des Prozesses
  aufgerufen werden.

****************************************************************/

void mdct_cleanup()
{
  unsigned int exp2;

  if (!global_buffers_initialized) return;

#ifdef DEBUG
  printf("MDCT: cleaning global buffers\n");
#endif

  for (exp2 = MDCT_MIN_EXP2; exp2 < MDCT_MAX_EXP2; exp2++)
  {
    if (sin_win_buffer[exp2])
    {
#ifdef DEBUG
      printf("MDCT: cleaning sin buffer %d\n", exp2);
#endif

      free(sin_win_buffer[exp2]);
    }

    if (cos_buffer[exp2])
    {
#ifdef DEBUG
      printf("MDCT: cleaning cos buffer %d\n", exp2);
#endif

      free(cos_buffer[exp2]);
    }
  }

  global_buffers_initialized = 0;
}

/****************************************************************

  mdct

  F�hrt eine einzelne MDC-Transformation aus.

  Parameter: exp2   - Zweierexponent der Dimension
             input  - Speicher mit 2^(exp2) double-Werten
             output - Speicher f�r 2^(exp2)/2 double-Werte

****************************************************************/

void mdct(unsigned int exp2, const double *input, double *output)
{
  unsigned int i, j, dim;
  double koeff;

  dim = 1 << exp2;

#ifdef DEBUG
  printf("MDCT: transformation with dim %d\n", dim);
#endif
  
  for (i = 0; i < dim / 2; i++)
  {
    koeff = 0;
      
    for (j = 0; j < dim; j++)
    {
      koeff += sin_win_buffer[exp2][j] * input[j] * cos_buffer[exp2][i * dim + j];
    }

    output[i] = koeff;
  }
}

/****************************************************************

  imdct

  F�hrt eine inverse MDC-Transformation aus.

  Parameter: exp2   - Zweierexponent der Dimension
             input  - Speicher mit 2^(exp2)/2 double-Koeffizienten
             output - Speicher f�r 2^(exp2) double-Werte

****************************************************************/

void imdct(unsigned int exp2, const double *input, double *output)
{
  unsigned int i, j, dim;
  double value;

  /* Dimension errechnen */
  dim = 1 << exp2;

#ifdef DEBUG
  printf("MDCT: inverse transformation with dim %d\n", dim);
#endif

  for (i = 0; i < dim; i++)
  {
    value = 0;

    for (j = 0; j < dim / 2; j++)
    {
      value += input[j] * cos_buffer[exp2][i + j * dim];
    }

    output[i] = value * sin_win_buffer[exp2][i] * 4 / dim;

#ifdef MDCT_DEBUG_PEAK
    if (i == 0 || i == dim / 2) output[i] = 3.14;
#endif
  }
}

/***************************************************************/
