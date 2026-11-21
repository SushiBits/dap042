/**
 * @file usbcdc.c
 * 
 * Implementation of USB CDC with hardware UART
 */

#include "usbcdc.h"

#include "stm32f0xx.h"
#include "usb.h"
#include "usb_cdc.h"

#include <string.h>

void cdc_init(void) { /* stub */ }
void cdc_deinit(void) { /* stub */ }

static struct usb_cdc_line_coding cdc_line = {
    .dwDTERate          = 38400,
    .bCharFormat        = USB_CDC_1_STOP_BITS,
    .bParityType        = USB_CDC_NO_PARITY,
    .bDataBits          = 8,
};

usbd_respond cdc_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
    switch (req->bRequest) {
        case USB_CDC_SET_CONTROL_LINE_STATE:
            return usbd_ack;
        case USB_CDC_SET_LINE_CODING:
            memcpy(&cdc_line, req->data, sizeof(cdc_line));
            return usbd_ack;
        case USB_CDC_GET_LINE_CODING:
            dev->status.data_ptr = &cdc_line;
            dev->status.data_count = sizeof(cdc_line);
            return usbd_ack;
        default:
            return usbd_fail;
    }
    return usbd_fail; // stub
}

void cdc_ctrl_xfer(usbd_device *dev, uint8_t event, uint8_t ep) { /* stub */ }
void cdc_data_xfer(usbd_device *dev, uint8_t event, uint8_t ep) { /* stub */ }
