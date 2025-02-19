/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo C�sar Pereira de Andrade <pcpa@conectiva.com.br>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/xf86cfg/card-cfg.c,v 1.4 2000/12/01 23:27:54 paulo Exp $
 */

#include "xf86config.h"
#include "mouse-cfg.h"
#include "cards.h"
#include "card-cfg.h"
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Viewport.h>
#ifdef USE_MODULES
#include "loader.h"
#endif

/*
 * Prototypes
 */
static Bool CardConfigCheck(void);
static void CardModelCallback(Widget, XtPointer, XtPointer);
#ifdef USE_MODULES
static void DriverCallback(Widget, XtPointer, XtPointer);
#endif

/*
 * Initialization
 */
static CardsEntry *card_entry;
static XF86ConfDevicePtr current_device;
static Widget filter, list, driver, busid;
static char **cards = NULL;
static int ncards;
#ifdef USE_MODULES
static char *driver_str;
#endif

/*
 * Implementation
 */
/*ARGSUSED*/
XtPointer
CardConfig(XtPointer config)
{
    XF86ConfDevicePtr card = (XF86ConfDevicePtr)config;
/*    XF86OptionPtr option;*/
    char card_name[32];
    Arg args[1];
    char *bus, *drv_nam;

    xf86info.cur_list = CARD;
    XtSetSensitive(back, xf86info.lists[CARD].cur_function > 0);
    XtSetSensitive(next, xf86info.lists[CARD].cur_function <
			 xf86info.lists[CARD].num_functions - 1);
    (xf86info.lists[CARD].functions[xf86info.lists[CARD].cur_function])
	(&xf86info);

    card_entry = NULL;
    current_device = card;
    XawListUnhighlight(list);
    XtSetArg(args[0], XtNstring, "");
    XtSetValues(filter, args, 1);

    if (card != NULL) {
	if (card->dev_card != NULL) {
	    int i;

	    for (i = 0; i < ncards; i++) {
		if (strcasecmp(cards[i], card->dev_card) == 0) {
		    card_entry = LookupCard(cards[i]);
		    XawListHighlight(list, i);
		    XtSetArg(args[0], XtNstring, cards[i]);
		    XtSetValues(filter, args, 1);
		    break;
		}
	    }
	}
	XtSetArg(args[0], XtNstring, card->dev_identifier);
	XtSetValues(ident_widget, args, 1);
	XtSetArg(args[0], XtNstring, card->dev_busid);
	XtSetValues(busid, args, 1);
#ifdef USE_MODULES
	XtSetArg(args[0], XtNlabel, driver_str = XtNewString(card->dev_driver));
#else
	XtSetArg(args[0], XtNstring, card->dev_driver);
#endif
	XtSetValues(driver, args, 1);
    }
    else {
	XF86ConfDevicePtr device = XF86Config->conf_device_lst;
	int ndevices = 0;

	while (device != NULL) {
	    ++ndevices;
	    device = (XF86ConfDevicePtr)(device->list.next);
	}
	do {
	    ++ndevices;
	    XmuSnprintf(card_name, sizeof(card_name), "Card%d", ndevices);
	} while (xf86findDevice(card_name,
		 XF86Config->conf_device_lst));

	XtSetArg(args[0], XtNstring, card_name);
	XtSetValues(ident_widget, args, 1);
	XtSetArg(args[0], XtNstring, "");
	XtSetValues(busid, args, 1);
#ifdef USE_MODULES	
	XtSetArg(args[0], XtNlabel, driver_str = XtNewString("vga"));
#else
	XtSetArg(args[0], XtNstring, "vga");
#endif
	XtSetValues(driver, args, 1);
    }

    if (ConfigLoop(CardConfigCheck) == True) {
	if (card_entry != NULL && card_entry->driver == NULL) {
	    fprintf(stderr, "No driver field in Cards database.\n"
		    "Please make sure you have the correct files installed.\n");
	    exit(1);
	}
	if (card == NULL) {
	    card = (XF86ConfDevicePtr)XtCalloc(1, sizeof(XF86ConfDeviceRec));
	    card->dev_identifier = XtNewString(ident_string);
	    card->dev_driver = XtNewString(card_entry->driver);
	    card->dev_card = XtNewString(card_entry->name);
	    if (card_entry->chipset)
		card->dev_chipset = XtNewString(card_entry->chipset);
	    if (card_entry->ramdac)
		card->dev_ramdac = XtNewString(card_entry->ramdac);
	    if (card_entry->clockchip)
		card->dev_clockchip = XtNewString(card_entry->clockchip);
	}
	else if (card_entry != NULL) {
	    XtFree(card->dev_driver);
	    card->dev_driver = XtNewString(card_entry->driver);
	    if (card_entry->chipset) {
		XtFree(card->dev_chipset);
		card->dev_chipset = XtNewString(card_entry->chipset);
	    }
	    if (card_entry->ramdac) {
		XtFree(card->dev_ramdac);
		card->dev_ramdac = XtNewString(card_entry->ramdac);
	    }
	    if (card_entry->clockchip) {
		XtFree(card->dev_clockchip);
		card->dev_clockchip = XtNewString(card_entry->clockchip);
	    }
	}
	if (strcasecmp(card->dev_identifier, ident_string))
	    xf86renameDevice(XF86Config, card, ident_string);
	XtSetArg(args[0], XtNstring, &bus);
	XtGetValues(busid, args, 1);
	XtFree(card->dev_busid);
	card->dev_busid = XtNewString(bus);

#ifdef USE_MODULES
	drv_nam = driver_str;
#else
	XtSetArg(args[0], XtNstring, &drv_nam);
	XtGetValues(driver, args, 1);
#endif

	XtFree(card->dev_driver);
	card->dev_driver = XtNewString(drv_nam);

#ifdef USE_MODULES
	XtFree(driver_str);
#endif

	return ((XtPointer)card);
    }
#ifdef USE_MODULES
    XtFree(driver_str);
#endif

    return (NULL);
}

static Bool
CardConfigCheck(void)
{
    XF86ConfDevicePtr device = XF86Config->conf_device_lst;
    char *drv_nam;

#ifdef USE_MODULES
    drv_nam = driver_str;
#else
    Arg args[1];

    XtSetArg(args[0], XtNstring, &drv_nam);
    XtGetValues(driver, args, 1);
#endif
    if (ident_string == NULL || strlen(ident_string) == 0 ||
	(current_device == NULL && card_entry == NULL) ||
	drv_nam == NULL || *drv_nam == '\0')
	return (False);

    while (device != NULL) {
	if (device != current_device &&
	    strcasecmp(ident_string, device->dev_identifier) == 0)
	    return (False);
	device = (XF86ConfDevicePtr)(device->list.next);
    }

    return (True);
}

static void
CardModelCallback(Widget w, XtPointer user_data, XtPointer call_data)
{
    Arg args[1];
    XawListReturnStruct *info = (XawListReturnStruct *)call_data;
    char tip[4096], *str;
    int len;
    static int first = 1;

    XtSetArg(args[0], XtNstring, info->string);
    XtSetValues(filter, args, 1);
    card_entry = LookupCard(info->string);

    len = XmuSnprintf(tip, sizeof(tip), "Name:      %s\n", card_entry->name);
    if (card_entry->flags & F_UNSUPPORTED)
	len += XmuSnprintf(tip + len, sizeof(tip) - len,
			   "**THIS CARD IS UNSUPPORTED**\n");
    if (card_entry->chipset != NULL)
	len += XmuSnprintf(tip + len, sizeof(tip) - len,
			   "Chipset:   %s\n", card_entry->chipset);
    if (card_entry->driver != NULL) {
#ifdef USE_MODULES
	XtFree(driver_str);
	driver_str = XtNewString(card_entry->driver);
	XtVaSetValues(driver, XtNlabel, driver_str, NULL, 0);
#endif
	len += XmuSnprintf(tip + len, sizeof(tip) - len,
			   "Driver:    %s\n", card_entry->driver);
    }
    if (card_entry->ramdac != NULL)
	len += XmuSnprintf(tip + len, sizeof(tip),
			   "Ramdac:    %s\n", card_entry->ramdac);
    if (card_entry->clockchip != NULL)
	len += XmuSnprintf(tip + len, sizeof(tip) - len,
			   "Clockchip: %s\n", card_entry->clockchip);
    if (card_entry->dacspeed != NULL)
	len += XmuSnprintf(tip + len, sizeof(tip) - len,
			   "Dacspeed:  %s\n", card_entry->dacspeed);
    if (card_entry->lines != NULL)
	len += XmuSnprintf(tip + len, sizeof(tip) - len,
			   "\n%s\n", card_entry->lines);

    /* the first tip memory, if any, cannot be released */
    if (!first) {
	XtSetArg(args[0], XtNtip, &str);
	XtGetValues(filter, args, 1);
	XtFree(str);
    }
    else
	first = 0;

#ifndef USE_MODULES
    XtSetArg(args[0], XtNstring, card_entry->driver ? card_entry->driver : "vga");
    XtSetValues(driver, args, 1);
#endif

    str = XtNewString(tip);
    XtSetArg(args[0], XtNtip, str);
    XtSetValues(filter, args, 1);
}

/*ARGSUSED*/
void
CardFilterAction(Widget w, XEvent *ev, String *params, Cardinal *num_params)
{
    char **cards, *pattern, **old_cards;
    int ncards, old_ncards;
    Arg args[2];

    XtSetArg(args[0], XtNstring, &pattern);
    XtGetValues(w, args, 1);

    XtSetArg(args[0], XtNlist, &old_cards);
    XtSetArg(args[1], XtNnumberStrings, &old_ncards);
    XtGetValues(list, args, 2);

    cards = FilterCardNames(pattern, &ncards);

    if (ncards == 0) {
	cards = (char**)XtMalloc(sizeof(char*));
	cards[0] = XtNewString("");
	ncards = 1;
    }

    XtSetArg(args[0], XtNlist, cards);
    XtSetArg(args[1], XtNnumberStrings, ncards);
    XtSetValues(list, args, 2);

    if (old_ncards > 1 || (XtName(list) != old_cards[0])) {
	while (--old_ncards > -1)
	    XtFree(old_cards[old_ncards]);
	XtFree((char*)old_cards);
    }

    /* force relayout */
    XtUnmanageChild(list);
    XtManageChild(list);
}

#ifdef USE_MODULES
/*ARGSUSED*/
static void
DriverCallback(Widget w, XtPointer user_data, XtPointer call_data)
{
    Arg args[1];

    XtFree(driver_str);
    driver_str = XtNewString(XtName(w));
    XtSetArg(args[0], XtNlabel, driver_str);
    XtSetValues(driver, args, 1);
}
#endif

void
CardModel(XF86SetupInfo *info)
{
    static int first = 1;
    static Widget model;

    if (first) {
	Widget label, viewport;

	first = 0;

	cards = GetCardNames(&ncards);

	model = XtCreateWidget("cardModel", formWidgetClass,
			       configp, NULL, 0);
	label = XtCreateManagedWidget("label", labelWidgetClass,
				      model, NULL, 0);
	filter = XtVaCreateManagedWidget("filter", asciiTextWidgetClass,
					 model,
					 XtNeditType, XawtextEdit,
					 NULL, 0);
	viewport = XtCreateManagedWidget("viewport", viewportWidgetClass,
					 model, NULL, 0);
	list = XtVaCreateManagedWidget("list", listWidgetClass,
				       viewport,
				       XtNlist, cards,
				       XtNnumberStrings, ncards,
				       NULL, 0);
	XtAddCallback(list, XtNcallback, CardModelCallback,
		      (XtPointer)info);
	XtCreateManagedWidget("driverL", labelWidgetClass, model, NULL, 0);
#ifdef USE_MODULES
	driver = XtVaCreateManagedWidget("driver", menuButtonWidgetClass,
					 model,
					 XtNmenuName, "driverM",
					 NULL, 0);
	{
	    Widget menu, sme;
	    xf86cfgDriverOptions *opts = video_driver_info;

	    menu = XtCreatePopupShell("driverM", simpleMenuWidgetClass,
				      driver, NULL, 0);
	    while (opts) {
		sme = XtCreateManagedWidget(opts->name, smeBSBObjectClass,
					    menu, NULL, 0);
		XtAddCallback(sme, XtNcallback, DriverCallback, NULL);
		opts = opts->next;
	    }
	}
#else
	driver = XtVaCreateManagedWidget("driver", asciiTextWidgetClass,
					 model,
					 XtNeditType, XawtextEdit,
					 NULL, 0);
#endif
	XtCreateManagedWidget("busidL", labelWidgetClass, model, NULL, 0);
	busid = XtVaCreateManagedWidget("busid", asciiTextWidgetClass,
					 model,
					 XtNeditType, XawtextEdit,
					 NULL, 0);

	XtRealizeWidget(model);
    }
    XtChangeManagedSet(&current, 1, NULL, NULL, &model, 1);
    current = model;
}
