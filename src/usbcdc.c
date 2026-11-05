/**
 * @file usbcdc.c
 * 
 * Implementation of USB CDC with hardware UART
 */

#include "usbcdc.h"

#include "stm32f0xx.h"
#include "usb.h"

void cdc_init(void) { /* stub */ }
void cdc_deinit(void) { /* stub */ }

usbd_respond cdc_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
    return usbd_fail; // stub
}

void cdc_ctrl_xfer(usbd_device *dev, uint8_t event, uint8_t ep) { /* stub */ }
void cdc_data_xfer(usbd_device *dev, uint8_t event, uint8_t ep) { /* stub */ }
