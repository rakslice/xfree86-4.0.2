/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/configProcs.h,v 1.14 2000/11/02 19:58:20 anderson Exp $ */

/* Private procs.  Public procs are in xf86Parser.h and xf86Optrec.h */

/* Device.c */
XF86ConfDevicePtr xf86parseDeviceSection(void);
void xf86printDeviceSection(FILE *cf, XF86ConfDevicePtr ptr);
void xf86freeDeviceList(XF86ConfDevicePtr ptr);
int xf86validateDevice(XF86ConfigPtr p);
/* Files.c */
XF86ConfFilesPtr xf86parseFilesSection(void);
void xf86printFileSection(FILE *cf, XF86ConfFilesPtr ptr);
void xf86freeFiles(XF86ConfFilesPtr p);
/* Flags.c */
XF86ConfFlagsPtr xf86parseFlagsSection(void);
void xf86printServerFlagsSection(FILE *f, XF86ConfFlagsPtr flags);
void xf86freeFlags(XF86ConfFlagsPtr flags);
/* Input.c */
XF86ConfInputPtr xf86parseInputSection(void);
void xf86printInputSection(FILE *f, XF86ConfInputPtr ptr);
void xf86freeInputList(XF86ConfInputPtr ptr);
int xf86validateInput (XF86ConfigPtr p);
/* Keyboard.c */
XF86ConfInputPtr xf86parseKeyboardSection(void);
/* Layout.c */
XF86ConfLayoutPtr xf86parseLayoutSection(void);
void xf86printLayoutSection(FILE *cf, XF86ConfLayoutPtr ptr);
void xf86freeLayoutList(XF86ConfLayoutPtr ptr);
void xf86freeAdjacencyList(XF86ConfAdjacencyPtr ptr);
void xf86freeInputrefList(XF86ConfInputrefPtr ptr);
int xf86validateLayout(XF86ConfigPtr p);
/* Module.c */
XF86LoadPtr xf86parseModuleSubSection(XF86LoadPtr head, char *name);
XF86ConfModulePtr xf86parseModuleSection(void);
void xf86printModuleSection(FILE *cf, XF86ConfModulePtr ptr);
XF86LoadPtr xf86addNewLoadDirective(XF86LoadPtr head, char *name, int type, XF86OptionPtr opts);
void xf86freeModules(XF86ConfModulePtr ptr);
/* Monitor.c */
XF86ConfModeLinePtr xf86parseModeLine(void);
XF86ConfModeLinePtr xf86parseVerboseMode(void);
XF86ConfMonitorPtr xf86parseMonitorSection(void);
XF86ConfModesPtr xf86parseModesSection(void);
void xf86printMonitorSection(FILE *cf, XF86ConfMonitorPtr ptr);
void xf86printModesSection(FILE *cf, XF86ConfModesPtr ptr);
void xf86freeMonitorList(XF86ConfMonitorPtr ptr);
void xf86freeModesList(XF86ConfModesPtr ptr);
void xf86freeModeLineList(XF86ConfModeLinePtr ptr);
int xf86validateMonitor(XF86ConfigPtr p, XF86ConfScreenPtr screen);
/* Pointer.c */
XF86ConfInputPtr xf86parsePointerSection(void);
/* Screen.c */
XF86ConfDisplayPtr xf86parseDisplaySubSection(void);
XF86ConfScreenPtr xf86parseScreenSection(void);
void xf86printScreenSection(FILE *cf, XF86ConfScreenPtr ptr);
void xf86freeScreenList(XF86ConfScreenPtr ptr);
void xf86freeAdaptorLinkList(XF86ConfAdaptorLinkPtr ptr);
void xf86freeDisplayList(XF86ConfDisplayPtr ptr);
void xf86freeModeList(XF86ModePtr ptr);
int xf86validateScreen(XF86ConfigPtr p);
/* Vendor.c */
XF86ConfVendorPtr xf86parseVendorSection(void);
XF86ConfVendSubPtr xf86parseVendorSubSection (void);
void xf86freeVendorList(XF86ConfVendorPtr p);
void xf86printVendorSection(FILE * cf, XF86ConfVendorPtr ptr);
void xf86freeVendorSubList (XF86ConfVendSubPtr ptr);
/* Video.c */
XF86ConfVideoPortPtr xf86parseVideoPortSubSection(void);
XF86ConfVideoAdaptorPtr xf86parseVideoAdaptorSection(void);
void xf86printVideoAdaptorSection(FILE *cf, XF86ConfVideoAdaptorPtr ptr);
void xf86freeVideoAdaptorList(XF86ConfVideoAdaptorPtr ptr);
void xf86freeVideoPortList(XF86ConfVideoPortPtr ptr);
/* read.c */
int xf86validateConfig(XF86ConfigPtr p);
/* scan.c */
unsigned int xf86strToUL(char *str);
int xf86getToken(xf86ConfigSymTabRec *tab);
void xf86unGetToken(int token);
char *xf86tokenString(void);
void xf86parseError(char *format, ...);
void xf86parseWarning(char *format, ...);
void xf86validationError(char *format, ...);
void xf86setSection(char *section);
int xf86getStringToken(xf86ConfigSymTabRec *tab);
/* write.c */
/* DRI.c */
XF86ConfBuffersPtr xf86parseBuffers (void);
void xf86freeBuffersList (XF86ConfBuffersPtr ptr);
XF86ConfDRIPtr xf86parseDRISection (void);
void xf86printDRISection (FILE * cf, XF86ConfDRIPtr ptr);
void xf86freeDRI (XF86ConfDRIPtr ptr);

/* Externally provided functions */
void ErrorF(const char *f, ...);
void VErrorF(const char *f, va_list args);
