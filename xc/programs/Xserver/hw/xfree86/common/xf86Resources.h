/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Resources.h,v 1.11 2000/07/11 16:14:57 tsi Exp $ */

#ifndef _XF86_RESOURCES_H

#define _XF86_RESOURCES_H

#include "xf86str.h"

#define _END {ResEnd,0,0}

#define _VGA_EXCLUSIVE \
		{ResExcMemBlock | ResBios | ResBus, 0x000A0000, 0x000AFFFF},\
		{ResExcMemBlock | ResBios | ResBus, 0x000B0000, 0x000B7FFF},\
		{ResExcMemBlock | ResBios | ResBus, 0x000B8000, 0x000BFFFF},\
		{ResExcIoBlock  | ResBios | ResBus,     0x03B0,     0x03BB},\
		{ResExcIoBlock  | ResBios | ResBus,     0x03C0,     0x03DF}

#define _VGA_SHARED \
		{ResShrMemBlock | ResBios | ResBus, 0x000A0000, 0x000AFFFF},\
		{ResShrMemBlock | ResBios | ResBus, 0x000B0000, 0x000B7FFF},\
		{ResShrMemBlock | ResBios | ResBus, 0x000B8000, 0x000BFFFF},\
		{ResShrIoBlock  | ResBios | ResBus,     0x03B0,     0x03BB},\
		{ResShrIoBlock  | ResBios | ResBus,     0x03C0,     0x03DF}

#define _VGA_SHARED_MEM \
		{ResShrMemBlock | ResBios | ResBus, 0x000A0000, 0x000AFFFF},\
		{ResShrMemBlock | ResBios | ResBus, 0x000B0000, 0x000B7FFF},\
 		{ResShrMemBlock | ResBios | ResBus, 0x000B8000, 0x000BFFFF}

#define _VGA_SHARED_IO \
		{ResShrIoBlock  | ResBios | ResBus,     0x03B0,     0x03BB},\
		{ResShrIoBlock  | ResBios | ResBus,     0x03C0,     0x03DF}

/*
 * Exclusive unused VGA:  resources unneeded but cannot be disabled.
 * Like old Millennium.
 */
#define _VGA_EXCLUSIVE_UNUSED \
	{ResExcUusdMemBlock | ResBios | ResBus, 0x000A0000, 0x000AFFFF},\
	{ResExcUusdMemBlock | ResBios | ResBus, 0x000B0000, 0x000B7FFF},\
	{ResExcUusdMemBlock | ResBios | ResBus, 0x000B8000, 0x000BFFFF},\
	{ResExcUusdIoBlock  | ResBios | ResBus,     0x03B0,     0x03BB},\
	{ResExcUusdIoBlock  | ResBios | ResBus,     0x03C0,     0x03DF}

/*
 * Shared unused VGA:  resources unneeded but cannot be disabled
 * independently.  This is used to determine if a device needs RAC.
 */
#define _VGA_SHARED_UNUSED \
	{ResShrUusdMemBlock | ResBios | ResBus, 0x000A0000, 0x000AFFFF},\
	{ResShrUusdMemBlock | ResBios | ResBus, 0x000B0000, 0x000B7FFF},\
	{ResShrUusdMemBlock | ResBios | ResBus, 0x000B8000, 0x000BFFFF},\
	{ResShrUusdIoBlock  | ResBios | ResBus,     0x03B0,     0x03BB},\
	{ResShrUusdIoBlock  | ResBios | ResBus,     0x03C0,     0x03DF}

/*
 * Sparse versions of the above for those adapters that respond to all ISA
 * aliases of VGA ports.
 */
#define _VGA_EXCLUSIVE_SPARSE \
	{ResExcMemBlock | ResBios | ResBus, 0x000A0000, 0x000AFFFF},\
	{ResExcMemBlock | ResBios | ResBus, 0x000B0000, 0x000B7FFF},\
	{ResExcMemBlock | ResBios | ResBus, 0x000B8000, 0x000BFFFF},\
	{ResExcIoSparse | ResBios | ResBus,     0x03B0,     0x03F8},\
	{ResExcIoSparse | ResBios | ResBus,     0x03B8,     0x03FC},\
	{ResExcIoSparse | ResBios | ResBus,     0x03C0,     0x03E0}

#define _VGA_SHARED_SPARSE \
	{ResShrMemBlock | ResBios | ResBus, 0x000A0000, 0x000AFFFF},\
	{ResShrMemBlock | ResBios | ResBus, 0x000B0000, 0x000B7FFF},\
	{ResShrMemBlock | ResBios | ResBus, 0x000B8000, 0x000BFFFF},\
	{ResShrIoSparse | ResBios | ResBus,     0x03B0,     0x03F8},\
	{ResShrIoSparse | ResBios | ResBus,     0x03B8,     0x03FC},\
	{ResShrIoSparse | ResBios | ResBus,     0x03C0,     0x03E0}

#define _8514_EXCLUSIVE \
	{ResExcIoSparse | ResBios | ResBus, 0x02E8, 0x03F8}

#define _8514_SHARED \
	{ResShrIoSparse | ResBios | ResBus, 0x02E8, 0x03F8}

/* predefined resources */
extern resRange resVgaExclusive[];
extern resRange resVgaShared[];
extern resRange resVgaIoShared[];
extern resRange resVgaMemShared[];
extern resRange resVgaUnusedExclusive[];
extern resRange resVgaUnusedShared[];
extern resRange resVgaSparseExclusive[];
extern resRange resVgaSparseShared[];
extern resRange res8514Exclusive[];
extern resRange res8514Shared[];

/* Less misleading aliases for xf86SetOpratingState() */
#define resVgaMem resVgaMemShared
#define resVgaIo  resVgaIoShared

/* old style names */
#define RES_EXCLUSIVE_VGA   resVgaExclusive
#define RES_SHARED_VGA      resVgaShared
#define RES_EXCLUSIVE_8514  res8514Exclusive
#define RES_SHARED_8514     res8514Shared

#define _PCI_AVOID_PC_STYLE \
	{ResExcIoSparse | ResBus, 0x0100, 0x0300},\
	{ResExcIoSparse | ResBus, 0x0200, 0x0200},\
        {ResExcMemBlock | ResBus, 0xA0000,0xFFFFF}

extern resRange PciAvoid[];

#define RES_UNDEFINED NULL
#endif
