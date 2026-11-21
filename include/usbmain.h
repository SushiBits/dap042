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

/* Bring-up harness: see the block comment in usbmain.c. */
extern volatile uint32_t usb_evt_count;
extern volatile uint32_t usb_istr_last;
extern volatile uint32_t usb_desc_count;
extern volatile uint16_t usb_desc_len_last;
extern volatile uint32_t usb_config_count;
extern volatile uint16_t usb_log[16][4];
extern volatile uint8_t  usb_log_idx;
extern volatile uint8_t  usb_log_of;

void usb_init(void);

__END_DECLS

#endif // defined(_USBMAIN_H_)