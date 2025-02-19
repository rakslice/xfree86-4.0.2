/* 
Copyright (c) 2000 by Juliusz Chroboczek
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions: 
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software. 

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/* $XFree86: xc/programs/Xserver/hw/kdrive/vesa/vesainit.c,v 1.5 2000/12/08 21:40:29 keithp Exp $ */

#include "vesa.h"

const KdCardFuncs vesaFuncs = {
    vesaCardInit,               /* cardinit */
    vesaScreenInit,             /* scrinit */
    vesaInitScreen,             /* initScreen */
    vesaPreserve,               /* preserve */
    vesaEnable,                 /* enable */
    0,                          /* dpms */
    vesaDisable,                /* disable */
    vesaRestore,                /* restore */
    vesaScreenFini,             /* scrfini */
    vesaCardFini,               /* cardfini */
    
    0,                          /* initCursor */
    0,                          /* enableCursor */
    0,                          /* disableCursor */
    0,                          /* finiCursor */
    0,                          /* recolorCursor */
    
    0,                          /* initAccel */
    0,                          /* enableAccel */
    0,                          /* syncAccel */
    0,                          /* disableAccel */
    0,                          /* finiAccel */
    
    vesaGetColors,              /* getColors */
    vesaPutColors,              /* putColors */
};

void
InitCard(char *name)
{
    KdCardAttr attr;
    KdCardInfoAdd((KdCardFuncs *) &vesaFuncs, &attr, 0);
}

void
InitOutput (ScreenInfo *pScreenInfo, int argc, char **argv)
{
    KdInitOutput (pScreenInfo, argc, argv);
}

void
InitInput (int argc, char **argv)
{
    KdInitInput(&Ps2MouseFuncs, &LinuxKeyboardFuncs);
}

int
ddxProcessArgument (int argc, char **argv, int i)
{
    int	ret;
    
    if (!(ret = vesaProcessArgument (argc, argv, i)))
	ret = KdProcessArgument(argc, argv, i);
    return ret;
}
