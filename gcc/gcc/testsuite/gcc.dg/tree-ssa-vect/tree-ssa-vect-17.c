/* { dg-do run { target powerpc*-*-* } } */
/* { dg-do compile { target i?86-*-* } } */
/* { dg-options "-O2 -ftree-vectorize -fdump-tree-vect-stats -maltivec" { target powerpc*-*-* } } */
/* { dg-options "-O2 -ftree-vectorize -fdump-tree-vect-stats -msse2" { target i?86-*-* } } */
  
#include <stdarg.h>
#include <signal.h>

#define N 64

int
main1 ()
{
  int i;
  int ia[N];
  int ib[N]= 
    {1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0};

  int ic[N] =
    {1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0};

  char ca[N];
  char cb[N] =
    {1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0};

  char cc[N] =
    {1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0};

  short sa[N];
  short sb[N] =
    {1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0};

  short sc[N] =
    {1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0,
     1,1,0,0,1,0,1,0};

  /* Check ints.  */

  for (i = 0; i < N; i++)
    {
      ia[i] = ib[i] & ic[i];
    }

  /* check results:  */
  for (i = 0; i <N; i++)
    {
      if (ia[i] != ib[i] & ic[i])
        abort ();
    }

  /* Check chars.  */

  for (i = 0; i < N; i++)
    {
      ca[i] = cb[i] & cc[i];
    }

  /* check results:  */
  for (i = 0; i <N; i++)
    {
      if (ca[i] != cb[i] & cc[i])
        abort ();
    }

  /* Check shorts.  */

  for (i = 0; i < N; i++)
    {
      sa[i] = sb[i] & sc[i];
    }

  /* check results:  */
  for (i = 0; i <N; i++)
    {
      if (sa[i] != sb[i] & sc[i])
        abort ();
    }

  return 0;
}

void
sig_ill_handler (int sig)
{
    exit(0);
}

int main (void)
{
  /* Exit on systems without altivec.  */
  signal (SIGILL, sig_ill_handler);
  /* Altivec instruction, 'vor %v0,%v0,%v0'.  */
  asm volatile (".long 0x10000484");
  signal (SIGILL, SIG_DFL);

  return main1 ();
}

/* { dg-final { scan-tree-dump-times "vectorized 3 loops" 1 "vect"} } */
