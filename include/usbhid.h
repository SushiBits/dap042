/**
 * @file usbhid.h
 * 
 * Functions used in implementing the USB HID interface.
 */

#ifndef _USBHID_H_
#define _USBHID_H_

#include "stm32f0xx.h"
#include "usb.h"
#include <stdbool.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

extern const size_t hid_size;
extern uint8_t rx_data[], tx_data[];
extern volatile bool rx_drdy, tx_drdy;

void hid_init(void);
void hid_deinit(void);

usbd_respond hid_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback);
usbd_respond hid_class_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback);
void hid_data_xfer(usbd_device *dev, uint8_t event, uint8_t ep);
void hid_data_rx(void);
void hid_data_tx(void);

__END_DECLS

#endif // defined(_USBHID_H_)