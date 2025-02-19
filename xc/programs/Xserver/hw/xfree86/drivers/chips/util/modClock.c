/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/chips/util/modClock.c,v 1.4 2000/09/22 11:35:49 alanh Exp $ */

#ifdef __NetBSD__
#  include <sys/types.h>
#  include <machine/pio.h>
#  include <machine/sysarch.h>
#else
#  ifdef SVR4 
#    include <sys/types.h>
#    ifdef NCR
       /* broken NCR <sys/sysi86.h> */
#      define __STDC
#      include <sys/sysi86.h>
#      undef __STDC
#    else
#      include <sys/sysi86.h>
#    endif
#    ifdef SVR4
#      if !defined(sun)
#        include <sys/seg.h>
#      endif
#    endif
#    include <sys/v86.h>
#    if defined(sun)
#      include <sys/psw.h>
#    endif
#  endif
#  include "AsmMacros.h"
#endif /* NetBSD */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef Lynx
#include <fnmatch.h>
#endif

#ifdef __NetBSD__
#  define SET_IOPL() i386_iopl(3)
#  define RESET_IOPL() i386_iopl(0)
#else
#  ifdef SVR4
#    ifndef SI86IOPL
#      define SET_IOPL() sysi86(SI86V86,V86SC_IOPL,PS_IOPL)
#      define RESET_IOPL() sysi86(SI86V86,V86SC_IOPL,0)
#    else
#      define SET_IOPL() sysi86(SI86IOPL,3)
#      define RESET_IOPL() sysi86(SI86IOPL,0)
#    endif
#  else
#    ifndef Lynx
#      define SET_IOPL() iopl(3)
#      define RESET_IOPL() iopl(0)
#    else
#      define SET_IOPL() 0
#      define RESET_IOPL() 0
#    endif
#  endif
#endif

#define tolerance 0.01 /* +/- 1% */

int compute_clock (
		   double target,
		   double Fref,
		   unsigned int ClkMaxN,
		   unsigned int ClkMaxM,
		   unsigned int PStart,
		   unsigned int *bestM,
		   unsigned int *bestN,
		   unsigned int *bestP,
		   unsigned int *bestPSN) {

  unsigned int M, N, P, PSN, PSNx;

  double bestError, abest = 42, bestFout;

  double Fvco, Fout;
  double error, aerror;

  unsigned int M_min = 3;
  unsigned int M_max = ClkMaxM;

  if (target < 1e6){
    fprintf (stderr, "MHz assumed, changed to %g MHz\n", target);
    target *= 1e6;
  }

  if (target > 220.0e6) {
    fprintf (stderr, "too large\n");
    return 1;
  }

  /* Other parameters available onthe 65548 but not the 65545, and
     not documented in the Clock Synthesizer doc in rev 1.0 of the
     65548 datasheet:

     + XR30[4] = 0, VCO divider loop uses divide by 4 (same as 65545)
		 1, VCO divider loop uses divide by 16

     + XR30[5] = 1, reference clock is divided by 5

     I haven't put in any support for those here.  For simplicity,
     they should be set to 0 on the 65548, and left untouched on
     earlier chips.  */

  for (PSNx = 0; PSNx <= 1; PSNx++) {
    unsigned int low_N, high_N;
    double Fref4PSN;

    PSN = PSNx ? 1 : 4;

    low_N = 3;
    high_N = ClkMaxN;

    while (Fref / (PSN * low_N) > 2.0e6)
      low_N++;
    while (Fref / (PSN * high_N) < 150.0e3)
      high_N--;

    Fref4PSN = Fref * 4 / PSN;
    for (N = low_N; N <= high_N; N++) {
      double tmp = Fref4PSN / N;
      for (P = PStart; P <= 5; P++) {
	double Fvco_desired = target * (1 << P);
	double M_desired = Fvco_desired / tmp;
	/* Which way will M_desired be rounded?  Do all three just to
	   be safe.  */
	unsigned int M_low = M_desired - 1;
	unsigned int M_hi = M_desired + 1;

	if (M_hi < M_min || M_low > M_max)
	  continue;

	if (M_low < M_min)
	  M_low = M_min;
	if (M_hi > M_max)
	  M_hi = M_max;

	for (M = M_low; M <= M_hi; M++) {
	  Fvco = tmp * M;
	  if (Fvco <= 48.0e6)
	    continue;
	  if (Fvco > 220.0e6)
	    break;

	  Fout = Fvco / (1 << P);

	  error = (target - Fout) / target;

	  aerror = (error < 0) ? -error : error;
	  if (aerror < abest) {
	    abest = aerror;
	    bestError = error;
	    *bestM = M;
	    *bestN = N;
	    *bestP = P;
	    *bestPSN = PSN;
	    bestFout = Fout;
	  }
	}
      }
    }
  }

  if (abest < tolerance) {
    unsigned char tmp, idx;
    printf ("best: M=%d N=%d P=%d PSN=%d\n", *bestM, *bestN, *bestP, *bestPSN);

    if (bestFout > 1.0e6)
      printf ("Fout = %g MHz", bestFout / 1.0e6);
    else if (bestFout > 1.0e3)
      printf ("Fout = %g kHz", bestFout / 1.0e3);
    else
      printf ("Fout = %g Hz", bestFout);
    printf (", error = %g\n", bestError);
    return 0;
  }
  printf ("can't do it with less than %g error\n", bestError);
  return 1;
}

#define CT65520 0x1
#define CT65525 0x2
#define CT65530 0x3
#define CT64200 0x4

#define CT65535 0x11
#define CT65540 0x12
#define CT65545 0x13
#define CT65546 0x14
#define CT65548 0x15
#define CT64300 0x16

#define CT65550 0x31
#define CT65554 0x32
#define CT65555 0x33
#define CT68554 0x34
#define CT69000 0x35
#define CT69030 0x36

#define IS_Programmable(X) X&0x10
#define IS_HiQV(X) X&0x20

#define DotClk 0
#define MemClk 1
#define IS_MemClk(X) X&0x1

int set_clock(
	      unsigned int ChipType,
	      unsigned int ClockType,
	      unsigned int M,
	      unsigned int N,
	      unsigned int P,
	      unsigned int PSN) {

  unsigned int tmp, idx;

  SET_IOPL();

  idx = inb(0x3D6);
  if (IS_HiQV(ChipType)) {
    if (IS_MemClk(ClockType)) {
      printf ("XRCC = 0x%02X\n", M - 2);
      printf ("XRCD = 0x%02X\n", N - 2);
      printf ("XRCE = 0x%02X\n", (0x80 | P * 16 + (PSN == 1)));

      outb(0x3D6, 0xCE); /* Select Fix MClk before */
      tmp = inb(0x3D7);
      outb(0x3D7, tmp & 0x7F);
      outb(0x3D6, 0xCC);
      outb(0x3D7, (M - 2));
      outb(0x3D6, 0xCD);
      outb(0x3D7, (N - 2));
      outb(0x3D6, 0xCE);
      outb(0x3D7, (0x80 | (P * 16 + (PSN == 1))));
    } else {
      printf ("XRC8 = 0x%02X\n", M - 2);
      printf ("XRC9 = 0x%02X\n", N - 2);
      printf ("XRCA = 0x%02X\n", 0);
      printf ("XRCB = 0x%02X\n", P * 16 + (PSN == 1));

      outb(0x3D6, 0xC8);
      outb(0x3D7, (M - 2));
      outb(0x3D6, 0xC9);
      outb(0x3D7, (N - 2));
      outb(0x3D6, 0xCA);
      outb(0x3D7, 0x0);
      outb(0x3D6, 0xCB);
      outb(0x3D7, (P * 16 + (PSN == 1)));
    }
   } else {
    printf ("XR30 = 0x%02X\n", P * 2 + (PSN == 1));
    printf ("XR31 = 0x%02X\n", M - 2);
    printf ("XR32 = 0x%02X\n", N - 2);
    outb(0x3D6, 0x33);
    tmp = inb(0x3D7);
    if (IS_MemClk(ClockType)) {
      outb(0x3D7, tmp | 0x20);     
    } else {
      outb(0x3D7, tmp & ~0x20);
    }
    outb(0x3D6, 0x30);
    outb(0x3D7, (P * 2 + (PSN == 1)));
    outb(0x3D6, 0x31);
    outb(0x3D7, (M - 2));
    outb(0x3D6, 0x32);
    outb(0x3D7, (N - 2));
    outb(0x3D6, 0x33);
    outb(0x3D7, tmp);
  }
  outb(0x3D6, idx);
  RESET_IOPL();
  return 0;
}

unsigned int probe_chip(void) {

  unsigned int ChipType, temp;

  SET_IOPL();

  outb(0x3D6, 0x00);
  temp = inb(0x3D7);
  ChipType = 0;
  if (temp != 0xA5) {
    if ((temp & 0xF0) == 0x70) {
      ChipType = CT65520;
    }
    if ((temp & 0xF0) == 0x80) {  /* could also be a 65525 */
      ChipType = CT65530;
    }
    if ((temp & 0xF0) == 0xA0) {
      ChipType = CT64200;
    }
    if ((temp & 0xF0) == 0xB0) {
      ChipType = CT64300;
    }
    if ((temp & 0xF0) == 0xC0) {
      ChipType = CT65535;
    }
    if ((temp & 0xF8) == 0xD0) {
      ChipType = CT65540;
    }
    if ((temp & 0xF8) == 0xD8) {
      switch (temp & 0x07) {
      case 3:
	ChipType = CT65546;
	break;
      case 4:
	ChipType = CT65548;
	break;
      default:
	ChipType = CT65545;
      }
    }
  }
  /* At this point the chip could still be a HiQV, so check for
   * that. This test needs some looking at */
  if ((temp != 0) && (ChipType == 0)) {
    outb(0x3D6, 0x02);
    temp = inb(0x03D7);
    if (temp == 0xE0) {
      ChipType = CT65550;
    }
    if (temp == 0xE4) {
      ChipType = CT65554;
    }
    if (temp == 0xE5) {
      ChipType = CT65555;
    }
    if (temp == 0xF4) {
      ChipType = CT68554;
    }
    if (temp == 0xC0) {
      ChipType = CT69000;
    }
    if (temp == 0x30) {
      outb(0x3D6, 0x03);
      temp = inb(0x03D7);
      if (temp == 0x0C) ChipType = CT69030;
    }
  }
 
  RESET_IOPL();

  if (ChipType == 0) {      /* failure */
    fprintf(stderr, "Not a Chips and Technologies Chipset\n");
  }
  
  return ChipType;
}


int main (int argc, char *argv[]) {
  double target;
  double Fref = 14318180;
  unsigned int M, N, P, PSN, ChipType, ClockType;

  if (argc != 2) {
    fprintf (stderr, "usage: %s freq\n", argv[0]);
    return 1;
  }

  ClockType = DotClk;
#ifndef Lynx
  if (! fnmatch("*memClock",argv[0],FNM_PATHNAME)) {
#else
  if (strstr("memClock",argv[0]) != NULL) {
#endif
    ClockType = MemClk;
  }

  target = atof (argv[1]);

  ChipType = probe_chip();
  if (!ChipType) {
    return 1;
  }

  if (! IS_Programmable(ChipType)) {
    fprintf(stderr, "No programmable Clock!\n");
    return 1;
  }
  
  if (IS_HiQV(ChipType)) {
    if (! compute_clock(target, Fref, 63, 127, 1, &M, &N, &P, &PSN)) {
      return set_clock(ChipType, ClockType, M, N, P, PSN);
    } else {
      return 1;
    }
  } else {
    if (! compute_clock(target, Fref, 127, 127, 0, &M, &N, &P, &PSN)) {
      return set_clock(ChipType, ClockType, M, N, P, PSN);
    } else {
      return 1;
    }
  }
}
