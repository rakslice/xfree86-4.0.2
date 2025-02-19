/* No AVIDEO interface Regs */
/* No SCART interface Regs */
/* No Hi-Vision TV Regs */

typedef	struct	_SiS301Reg  {
	CARD8	*VBPart1;
	CARD8	*VBPart2;
	CARD8	*VBPart3;
	CARD8	*VBPart4;
} SiS301RegRec, SiS301RegPtr;

/* NTSC 640x480 */
static	unsigned char Reg301_0_640_480_P1[] = {
		0x90, 0xF6, 0x49, 0x0B, 0x00, 0x00, 0x00, 0x50,
		0x43, 0x30, 0x8C, 0xBD, 0x22, 0x1F, 0x0A, 0xDF,
		0xF5, 0x18, 0x0A, 0x08, 0x00, 0x00, 0x00, 0x21,
		0x03, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x61, 0x61,
		0x12};
static	unsigned char Reg301_0_640_480_P2[] = {
		0x38, 0x17, 0x1D, 0x03, 0x09, 0x05, 0x06, 0x0C,
		0x0C, 0x94, 0x49, 0x01, 0x0A, 0x06, 0x0D, 0x04,
		0x0A, 0x06, 0x14, 0x0D, 0x04, 0x0A, 0x00, 0x85,
		0x1B, 0x0C, 0x50, 0xB3, 0x99, 0x06, 0xEC, 0x4A,
		0x17, 0x88, 0x10, 0x4B, 0xA5, 0x30, 0xE2, 0x3C,
		0x62, 0xD3, 0x4A, 0x65, 0x9D, 0xF8, 0x14, 0xDA,
		0x13, 0x21, 0xED, 0x8A, 0x08, 0xF1, 0x05, 0x1F,
		0x16, 0x92, 0x8F, 0x40, 0x60, 0x80, 0x14, 0x90,
		0x8C, 0x60, 0x14, 0x50, 0x21, 0x50};
static	unsigned char Reg301_0_640_480_P3[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x10, 0xF6, 0xBF, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static	unsigned char Reg301_0_640_480_P4[] = {
		0x01, 0x01, 0x01, 0x07, 0x00, 0x00, 0x00, 0xFF,
		0x7F, 0x55, 0x52, 0x2A, 0x40, 0x1F, 0x8A, 0x0F,
		0x80, 0x00, 0x08, 0x4C, 0x8F, 0x1A, 0x43, 0x0A,
		0xE0, 0x40, 0x5D, 0x18};


/* NTSC 800x600 */
static	unsigned char Reg301_0_800_600_P1[] = {
		0x90, 0xF6, 0x49, 0x0D, 0x00, 0x00, 0x00, 0x64,
		0x1F, 0x40, 0x2C, 0x6C, 0x33, 0xEC, 0x82, 0x57,
		0x6D, 0x20, 0x12, 0x08, 0x00, 0x00, 0x00, 0x21,
		0x03, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xB5, 0x65,
		0x92};
static	unsigned char Reg301_0_800_600_P2[] = {
		0x38, 0x17, 0x1D, 0x03, 0x09, 0x05, 0x06, 0x0C,
		0x0C, 0x94, 0xC9, 0x01, 0x0A, 0x06, 0x0D, 0x04,
		0x0A, 0x06, 0x14, 0x0D, 0x04, 0x0A, 0x00, 0x85,
		0x1B, 0x0C, 0x50, 0xB3, 0x99, 0x06, 0xEC, 0x4A,
		0x17, 0x88, 0x10, 0x4B, 0xA5, 0x30, 0xE2, 0x3C,
		0x62, 0xD3, 0x4A, 0x65, 0x9D, 0xF8, 0x14, 0xDA,
		0x13, 0x21, 0xED, 0x8A, 0x08, 0xF4, 0x10, 0x1C,
		0x00, 0x92, 0x8F, 0x40, 0x60, 0x80, 0x14, 0x90,
		0x8C, 0x60, 0x14, 0x50, 0x29, 0x54};
static	unsigned char Reg301_0_800_600_P3[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x10, 0xF6, 0xBF, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static	unsigned char Reg301_0_800_600_P4[] = {
		0x01, 0x01, 0x01, 0x07, 0x00, 0x00, 0x00, 0xFF,
		0x7F, 0x55, 0x52, 0x2A, 0x40, 0x1F, 0x8A, 0x0F,
		0x80, 0x00, 0x08, 0x78, 0x8F, 0x22, 0x1F, 0x82,
		0x20, 0x52, 0x74, 0x5E};


/* PAL 640x480 */
static	unsigned char Reg301_1_640_480_P1[] = {
		0x90, 0xF6, 0x49, 0x0B, 0x00, 0x00, 0x00, 0x50,
		0x4F, 0x30, 0x8C, 0xC0, 0x22, 0x28, 0x0F, 0xDF,
		0xF8, 0x1C, 0x0A, 0x08, 0x00, 0x00, 0x00, 0x21,
		0x03, 0xF0, 0x00, 0x00, 0x20, 0x00, 0x02, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x33, 0x2D,
		0x91};
static	unsigned char Reg301_1_640_480_P2[] = {
		0x28, 0x19, 0x52, 0x35, 0x6E, 0x04, 0x38, 0x3D,
		0x70, 0x94, 0x49, 0x01, 0x12, 0x06, 0x3E, 0x35,
		0x6D, 0x06, 0x14, 0x3E, 0x35, 0x6D, 0x00, 0x45,
		0x2B, 0x70, 0x50, 0xBF, 0x97, 0x06, 0xD7, 0x5D,
		0x17, 0x88, 0x70, 0x45, 0xA5, 0x30, 0xE8, 0x48,
		0x62, 0xDD, 0x00, 0x68, 0xB0, 0x8B, 0x2D, 0x07,
		0x53, 0x2A, 0x05, 0xD3, 0x00, 0xF5, 0xFB, 0x1B,
		0x2A, 0x92, 0x8F, 0x40, 0x60, 0x80, 0x14, 0x90,
		0x8C, 0x60, 0x14, 0x63, 0x21, 0x50};
static	unsigned char Reg301_1_640_480_P3[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0xFA, 0xC8, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00};
static	unsigned char Reg301_1_640_480_P4[] = {
		0x01, 0x01, 0x01, 0x07, 0x00, 0x00, 0x00, 0xFF,
		0x7F, 0x55, 0x52, 0x2A, 0x40, 0x1F, 0x8A, 0x0F,
		0x80, 0x00, 0x08, 0x04, 0x09, 0x1A, 0x4F, 0x0F,
		0x32, 0x30, 0x9F, 0x66};


/* PAL 800x600 */
static	unsigned char Reg301_1_800_600_P1[] = {
		0x90, 0xF6, 0x49, 0x0D, 0x00, 0x00, 0x00, 0x64,
		0x23, 0x40, 0x2C, 0x6D, 0x33, 0xEF, 0x87, 0x57,
		0x70, 0x24, 0x12, 0x08, 0x00, 0x00, 0x00, 0x21,
		0x03, 0x30, 0x00, 0x00, 0x20, 0x00, 0x02, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x27, 0x75,
		0x82};
static	unsigned char Reg301_1_800_600_P2[] = {
		0x28, 0x19, 0x52, 0x35, 0x6E, 0x04, 0x38, 0x3D,
		0x70, 0x94, 0x49, 0x01, 0x12, 0x06, 0x3E, 0x35,
		0x6D, 0x06, 0x14, 0x3E, 0x35, 0x6D, 0x00, 0x45,
		0x2B, 0x70, 0x50, 0xBF, 0x97, 0x06, 0xD7, 0x5D,
		0x17, 0x88, 0x70, 0x45, 0xA5, 0x30, 0xE8, 0x48,
		0x62, 0xDD, 0x00, 0x68, 0xB0, 0x8B, 0x2D, 0x07,
		0x53, 0x2A, 0x05, 0xD3, 0x00, 0xEB, 0x05, 0x25,
		0x16, 0x92, 0x8F, 0x40, 0x60, 0x80, 0x14, 0x90,
		0x8C, 0x60, 0x14, 0x63, 0x29, 0x54};
static	unsigned char Reg301_1_800_600_P3[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0xFA, 0xC8, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00};
static	unsigned char Reg301_1_800_600_P4[] = {
		0x01, 0x01, 0x01, 0x07, 0x00, 0x00, 0x00, 0xFF,
		0x7F, 0x55, 0x52, 0x2A, 0x40, 0x1F, 0x8A, 0x0F,
		0x80, 0x00, 0x08, 0x19, 0x24, 0x22, 0x23, 0x87,
		0xD6, 0x41, 0x87, 0x3F};


SiS301RegRec	sis301_NTSC[] = {
		{Reg301_0_640_480_P1, Reg301_0_640_480_P2,
		 Reg301_0_640_480_P3, Reg301_0_640_480_P4},
		{Reg301_0_800_600_P1, Reg301_0_800_600_P2,
		 Reg301_0_800_600_P3, Reg301_0_800_600_P4}};

SiS301RegRec	sis301_PAL[] = {
		{Reg301_1_640_480_P1, Reg301_1_640_480_P2,
		 Reg301_1_640_480_P3, Reg301_1_640_480_P4},
		{Reg301_1_800_600_P1, Reg301_1_800_600_P2,
		 Reg301_1_800_600_P3, Reg301_1_800_600_P4}};
