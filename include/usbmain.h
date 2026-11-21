/**
 * @file usbmain.h
 * 
 * Main header for USB on dap042.
 */

#ifndef _USBMAIN_H_
#define _USBMAIN_H_

#include <sys/cdefs.h>
#include "stm32f0xx.h"
#include "usb.h"

__BEGIN_DECLS

extern usbd_device udev;

void usb_init(void);

__END_DECLS

#endif // defined(_USBMAIN_H_)