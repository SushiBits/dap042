/**
 * @file usbhid.c
 * 
 * Implementation for USB HID interface.
 */


#include "usbhid.h"

#include "stm32f0xx.h"
#include "usb.h"

usbd_respond hid_class_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
    return usbd_fail; // Stub
}

void hid_data_xfer(usbd_device *dev, uint8_t event, uint8_t ep) { /* stub */ }
