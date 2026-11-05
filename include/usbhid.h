/**
 * @file usbhid.h
 * 
 * Functions used in implementing the USB HID interface.
 */

#ifndef _USBHID_H_
#define _USBHID_H_

#include "stm32f0xx.h"
#include "usb.h"
#include <sys/cdefs.h>

__BEGIN_DECLS

usbd_respond hid_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback);
usbd_respond hid_class_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback);
void hid_data_xfer(usbd_device *dev, uint8_t event, uint8_t ep);

__END_DECLS

#endif // defined(_USBHID_H_)