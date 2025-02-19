/* $XFree86: xc/programs/Xserver/hw/xfree86/SuperProbe/PCI.h,v 3.36 2000/11/28 20:59:16 dawes Exp $ */ 
/*
 * PCI Probe
 *
 * Copyright 1995  The XFree86 Project, Inc.
 *
 * A lot of this comes from Robin Cutshaw's scanpci
 *
 */

#ifndef _PCI_H
#define _PCI_H

/*
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holder(s)
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holder(s) make(s) no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM(S) ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

struct pci_config_reg {
    /* start of official PCI config space header */
    union {
        unsigned long device_vendor;
	struct {
	    unsigned short vendor;
	    unsigned short device;
	} dv;
    } dv_id;
#define _device_vendor dv_id.device_vendor
#define _vendor dv_id.dv.vendor
#define _device dv_id.dv.device
    union {
        unsigned long status_command;
	struct {
	    unsigned short command;
	    unsigned short status;
	} sc;
    } stat_cmd;
#define _status_command stat_cmd.status_command
#define _command stat_cmd.sc.command
#define _status  stat_cmd.sc.status
    union {
        unsigned long class_revision;
	struct {
	    unsigned char rev_id;
	    unsigned char prog_if;
	    unsigned char sub_class;
	    unsigned char base_class;
	} cr;
    } class_rev;
#define _class_revision class_rev.class_revision
#define _rev_id     class_rev.cr.rev_id
#define _prog_if    class_rev.cr.prog_if
#define _sub_class  class_rev.cr.sub_class
#define _base_class class_rev.cr.base_class
    union {
        unsigned long bist_header_latency_cache;
	struct {
	    unsigned char cache_line_size;
	    unsigned char latency_timer;
	    unsigned char header_type;
	    unsigned char bist;
	} bhlc;
    } bhlc;
#define _bist_header_latency_cache bhlc.bist_header_latency_cache
#define _cache_line_size bhlc.bhlc.cache_line_size
#define _latency_timer   bhlc.bhlc.latency_timer
#define _header_type     bhlc.bhlc.header_type
#define _bist            bhlc.bhlc.bist
    union {
	struct {
	    unsigned long dv_base0;
	    unsigned long dv_base1;
	    unsigned long dv_base2;
	    unsigned long dv_base3;
	    unsigned long dv_base4;
	    unsigned long dv_base5;
	} dv;
	struct {
	    unsigned long bg_rsrvd[2];
	    unsigned char primary_bus_number;
	    unsigned char secondary_bus_number;
	    unsigned char subordinate_bus_number;
	    unsigned char secondary_latency_timer;
	    unsigned char io_base;
	    unsigned char io_limit;
	    unsigned short secondary_status;
	    unsigned short mem_base;
	    unsigned short mem_limit;
	    unsigned short prefetch_mem_base;
	    unsigned short prefetch_mem_limit;
	} bg;
    } bc;
#define	_base0				bc.dv.dv_base0
#define	_base1				bc.dv.dv_base1
#define	_base2				bc.dv.dv_base2
#define	_base3				bc.dv.dv_base3
#define	_base4				bc.dv.dv_base4
#define	_base5				bc.dv.dv_base5
#define	_primary_bus_number		bc.bg.primary_bus_number
#define	_secondary_bus_number		bc.bg.secondary_bus_number
#define	_subordinate_bus_number		bc.bg.subordinate_bus_number
#define	_secondary_latency_timer	bc.bg.secondary_latency_timer
#define _io_base			bc.bg.io_base
#define _io_limit			bc.bg.io_limit
#define _secondary_status		bc.bg.secondary_status
#define _mem_base			bc.bg.mem_base
#define _mem_limit			bc.bg.mem_limit
#define _prefetch_mem_base		bc.bg.prefetch_mem_base
#define _prefetch_mem_limit		bc.bg.prefetch_mem_limit
    unsigned long rsvd1;
    unsigned long rsvd2;
    unsigned long _baserom;
    unsigned long rsvd3;
    unsigned long rsvd4;
    union {
        unsigned long max_min_ipin_iline;
	struct {
	    unsigned char int_line;
	    unsigned char int_pin;
	    unsigned char min_gnt;
	    unsigned char max_lat;
	} mmii;
    } mmii;
#define _max_min_ipin_iline mmii.max_min_ipin_iline
#define _int_line mmii.mmii.int_line
#define _int_pin  mmii.mmii.int_pin
#define _min_gnt  mmii.mmii.min_gnt
#define _max_lat  mmii.mmii.max_lat
    /* I don't know how accurate or standard this is (DHD) */
    union {
	unsigned long user_config;
	struct {
	    unsigned char user_config_0;
	    unsigned char user_config_1;
	    unsigned char user_config_2;
	    unsigned char user_config_3;
	} uc;
    } uc;
#define _user_config uc.user_config
#define _user_config_0 uc.uc.user_config_0
#define _user_config_1 uc.uc.user_config_1
#define _user_config_2 uc.uc.user_config_2
#define _user_config_3 uc.uc.user_config_3
    /* end of official PCI config space header */
    unsigned long _pcibusidx;
    unsigned long _pcinumbus;
    unsigned long _pcibuses[16];
    unsigned short _configtype;   /* config type found                   */
    unsigned short _ioaddr;       /* config type 1 - private I/O addr    */
    unsigned long _cardnum;       /* config type 2 - private card number */
    unsigned short _funcnum;	  /* private function number */
};

#define PCI_EN 0x80000000
#define MAX_PCI_DEVICES 64
#define PCI_MULTIFUNC_DEV 0x80

/* Registers */
#define PCI_REG_USERCONFIG 0x40

/* Base Classes */
#define PCI_CLASS_PREHISTORIC		0x00
#define PCI_CLASS_DISPLAY		0x03

/* Sub Classes */
#define PCI_SUBCLASS_PREHISTORIC_VGA	0x01
#define PCI_SUBCLASS_DISPLAY_VGA	0x00
#define PCI_SUBCLASS_DISPLAY_OTHER	0x80

/* PCI Configuration address */
#define	PCI_MODE1_ADDRESS_REG		0xCF8
#define	PCI_MODE1_DATA_REG		0xCFC

#define	PCI_MODE2_ENABLE_REG		0xCF8
#ifdef PC98
#define	PCI_MODE2_FORWARD_REG		0xCF9
#else
#define	PCI_MODE2_FORWARD_REG		0xCFA
#endif

extern struct pci_config_reg *pci_devp[];

#define PCI_VENDOR_NCR_1	0x1000
#define PCI_VENDOR_ATI		0x1002
#define PCI_VENDOR_AVANCE	0x1005
#define PCI_VENDOR_TSENG	0x100C
#define PCI_VENDOR_WEITEK	0x100E
#define PCI_VENDOR_DIGITAL	0x1011
#define PCI_VENDOR_CIRRUS	0x1013
#define PCI_VENDOR_NCR_2	0x101A
#define PCI_VENDOR_TRIDENT	0x1023
#define PCI_VENDOR_MATROX	0x102B
#define PCI_VENDOR_CHIPSTECH	0x102C
#define PCI_VENDOR_SIS		0x1039
#define PCI_VENDOR_SGS          0x104A
#define PCI_VENDOR_NUMNINE	0x105D
#define PCI_VENDOR_UMC		0x1060
#define PCI_VENDOR_NVIDIA       0x10DE
#define PCI_VENDOR_ALLIANCE	0x1142
#define PCI_VENDOR_TRITECH	0x1292
#define PCI_VENDOR_NVIDIA_SGS   0x12D2
#define PCI_VENDOR_RENDITION	0x1163
#define PCI_VENDOR_SIGMADESIGNS	0x1236
#define PCI_VENDOR_S3		0x5333
#define PCI_VENDOR_ARK		0xEDD8
#define PCI_VENDOR_3DLABS	0x3D3D
#define PCI_VENDOR_INTERGRAPHICS	0x10EA
#define PCI_VENDOR_SMI		0x126F

/* Matrox */
#define PCI_CHIP_MGA2085PX	0x0518
#define PCI_CHIP_MGA2064W	0x0519
#define PCI_CHIP_MGA1064SG	0x051A
#define PCI_CHIP_MGA2164W	0x051B
#define PCI_CHIP_MGA2164W_AGP	0x051F
#define PCI_CHIP_MGAG100_PCI	0x1000
#define PCI_CHIP_MGAG100_AGP	0x1001
#define PCI_CHIP_MGAG200_PCI	0x0520
#define PCI_CHIP_MGAG200_AGP	0x0521

/* ATI */
#define PCI_CHIP_MACH32		0x4158
	/* Other ATI ChipID's deleted;  not used by SuperProbe */

/* Avance Logic */
#define PCI_CHIP_ALG2301	0x2301

/* Tseng */
#define PCI_CHIP_ET4000_W32P_A	0x3202
#define PCI_CHIP_ET4000_W32P_B	0x3205
#define PCI_CHIP_ET4000_W32P_D	0x3206
#define PCI_CHIP_ET4000_W32P_C	0x3207
#define PCI_CHIP_ET6000		0x3208
#define PCI_CHIP_ET6300		0x4702

/* Weitek */
#define PCI_CHIP_P9000		0x9001
#define PCI_CHIP_P9100		0x9100

/* Cirrus Logic */
#define PCI_CHIP_GD7548		0x0038
#define PCI_CHIP_GD5430		0x00A0
#define PCI_CHIP_GD5434_4	0x00A4
#define PCI_CHIP_GD5434_8	0x00A8
#define PCI_CHIP_GD5436		0x00AC
#define PCI_CHIP_GD5446		0x00B8
#define PCI_CHIP_GD5480		0x00BC
#define PCI_CHIP_GD5462		0x00D0
#define PCI_CHIP_GD5464		0x00D4
#define PCI_CHIP_GD5464BD       0x00D5
#define PCI_CHIP_GD5465		0x00D6
#define PCI_CHIP_GD7542		0x1200
#define PCI_CHIP_GD7543		0x1202
#define PCI_CHIP_GD7541		0x1204

/* Trident */
#define PCI_CHIP_9320		0x9320
#define PCI_CHIP_9420		0x9420
#define PCI_CHIP_9440		0x9440
#define PCI_CHIP_9660		0x9660
#define PCI_CHIP_9680		0x9680
#define PCI_CHIP_9682		0x9682
/* Bill Mair */
#define PCI_CHIP_939A		0x939A
#define PCI_CHIP_9388		0x9388
#define PCI_CHIP_9397		0x9397
#define PCI_CHIP_9520		0x9520
#define PCI_CHIP_9750		0x9750
#define PCI_CHIP_9850		0x9850

/* Chips & Tech */
#define PCI_CHIP_65545		0x00D8
#define PCI_CHIP_65548          0x00DC
#define PCI_CHIP_65550          0x00E0
#define PCI_CHIP_65554          0x00E4
#define PCI_CHIP_65555          0x00E5
#define PCI_CHIP_68554          0x00F4

/* SiS */
#define PCI_CHIP_SG86C201	0x0001
#define PCI_CHIP_SG86C202	0x0002
#define PCI_CHIP_SG86C205	0x0205
#define PCI_CHIP_SG86C215	0x0215
#define PCI_CHIP_SG86C225	0x0225
#define PCI_CHIP_SIS5598	0x0200	
#define PCI_CHIP_SIS5597	0x0200
#define PCI_CHIP_SIS6326	0x6326
#define PCI_CHIP_SIS530		0x6306
#define PCI_CHIP_SIS620		0x6306
#define PCI_CHIP_SIS300		0x0300
#define PCI_CHIP_SIS630		0x6300
#define PCI_CHIP_SIS540		0x5300

/* SGS */
#define PCI_CHIP_STG2000        0x0008
#define PCI_CHIP_STG1764        0x0009

/* NVIDIA */
#define PCI_CHIP_NV1            0x0008
#define PCI_CHIP_DAC64          0x0009

/* NVIDIA & SGS */
#define PCI_CHIP_RIVA128        0x0018

/* Number Nine */
#define PCI_CHIP_I128		0x2309
#define PCI_CHIP_I128_2		0x2339
#define PCI_CHIP_I128_3		0x493D

/* Alliance Semiconductor */
#define PCI_CHIP_PM6410		0x3210
#define PCI_CHIP_PM6422		0x6422
#define PCI_CHIP_PMAT24		0x6424
#define PCI_CHIP_PMAT3D		0x643D

/* Rendition */
#define PCI_CHIP_V1000		0x0001
#define PCI_CHIP_V2000		0x0002 /* just guessing */


/* SIGMA DESIGNS */
#define PCI_CHIP_SD_REALMAGIG64GX	0x6401

/* S3 */
#define PCI_CHIP_TRIO		0x8811
#define PCI_CHIP_AURORA64VP	0x8812
#define PCI_CHIP_TRIO64UVP	0x8814
#define PCI_CHIP_TRIO64V2_DXGX	0x8901
#define PCI_CHIP_PLATO_PX	0x8902
#define PCI_CHIP_868		0x8880
#define PCI_CHIP_928		0x88B0
#define PCI_CHIP_864_0		0x88C0
#define PCI_CHIP_864_1		0x88C1
#define PCI_CHIP_964_0		0x88D0
#define PCI_CHIP_964_1		0x88D1
#define PCI_CHIP_968		0x88F0
#define PCI_CHIP_ViRGE		0x5631
#define PCI_CHIP_ViRGE_VX	0x883D
#define PCI_CHIP_ViRGE_DXGX	0x8A01
#define PCI_CHIP_ViRGE_GX2	0x8A10
#define PCI_CHIP_ViRGE_MX	0x8C01
#define PCI_CHIP_ViRGE_MXP	0x8C03
#define PCI_CHIP_TRIO3D_B	0x8903
#define PCI_CHIP_TRIO3D		0x8904
#define PCI_CHIP_TRIO3D_2X	0x8A13
#define PCI_CHIP_SAVAGE3D	0x8A20
#define PCI_CHIP_SAVAGE3D_M	0x8A21
#define PCI_CHIP_SAVAGE4	0x8A22
#define PCI_CHIP_SAVAGE2000	0x9102

/* ARK Logic */
#define PCI_CHIP_1000PV		0xA091
#define PCI_CHIP_2000PV		0xA099
#define PCI_CHIP_2000MT		0xA0A1
#define PCI_CHIP_2000MI		0xA0A9

/* 3Dlabs */
#define PCI_CHIP_3DLABS_300SX      0x0001
#define PCI_CHIP_3DLABS_500TX      0x0002
#define PCI_CHIP_3DLABS_DELTA      0x0003
#define PCI_CHIP_3DLABS_PERMEDIA   0x0004

/* Tritech Microelectronics */
#define PCI_CHIP_TR25202	0xFC02

/* Intergraphics */
#define PCI_CHIP_INTERG_1680	0x1680
#define PCI_CHIP_INTERG_1682	0x1682

/* Silicon Motion */
#define PCI_CHIP_SMI_910	0x0910
#define PCI_CHIP_SMI_810	0x0810
#define PCI_CHIP_SMI_820	0x0820
#define PCI_CHIP_SMI_710	0x0710
#define PCI_CHIP_SMI_712	0x0712
#define PCI_CHIP_SMI_720	0x0720

/* Increase this as required */
#define MAX_DEV_PER_VENDOR 18
   
#endif /* _PCI_H */
