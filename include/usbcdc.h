/**
 * @file usbcdc.h
 * 
 * Functions used in implementing the USB CDC interface.
 */

#ifndef _USBCDC_H_
#define _USBCDC_H_

#include "stm32f0xx.h"
#include "usb.h"
#include <sys/cdefs.h>

__BEGIN_DECLS

usbd_respond cdc_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback);
void cdc_ctrl_xfer(usbd_device *dev, uint8_t event, uint8_t ep);
void cdc_data_xfer(usbd_device *dev, uint8_t event, uint8_t ep);

__END_DECLS

#endif // defined(_USBCDC_H_)