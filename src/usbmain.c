/**
 * @file usbmain.c
 * 
 * Main file for the USB loop.
 */

#define _USBMAIN_C_

#include "usbmain.h"
#include "usbdefs.h"

#include "stm32f0xx.h"
#include "stm32f0xx_it.h"
#include "usb.h"

#include "usbcdc.h"
#include "usbhid.h"

usbd_device udev;
uint32_t	ubuf[0x20];

static usbd_respond usb_getdesc(usbd_ctlreq *req, void **address, uint16_t *length) {
    const uint8_t dtype = req->wValue >> 8;
    const uint8_t dnumber = req->wValue & 0xFF;
    const void* desc;
    uint16_t len = 0;
    switch (dtype) {
    case USB_DTYPE_DEVICE:
        desc = &device_desc;
        break;
    case USB_DTYPE_CONFIGURATION:
        desc = &config_desc;
        len = sizeof(config_desc);
        break;
    case USB_DTYPE_STRING:
        if (dnumber < (sizeof(usb_string_table) / sizeof(usb_string_table[0]))) {
            desc = usb_string_table[dnumber];
        } else {
            return usbd_fail;
        }
        break;
    default:
        return usbd_fail;
    }
    if (len == 0) {
        len = ((struct usb_header_descriptor*)desc)->bLength;
    }
    *address = (void*)desc;
    *length = len;
    return usbd_ack;
}

static usbd_respond usb_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
    if ((req->bmRequestType & USB_REQ_RECIPIENT) == USB_REQ_INTERFACE) {
        switch (req->wIndex) {
        case USB_HID_IF:
            return hid_control(dev, req, callback);
        case USB_CDC_IF:
        case USB_CDC_DATA_IF: 
            return cdc_control(dev, req, callback);
        default:
            return usbd_fail;
        }
    }
    return usbd_fail;
}

static usbd_respond usb_setconf(usbd_device *dev, uint8_t cfg) {
    switch (cfg) {
    case 0:
        /* deconfiguring device */
        usbd_ep_deconfig(dev, USB_HID_RX_EP);
        usbd_ep_deconfig(dev, USB_HID_TX_EP);
        usbd_reg_endpoint(dev, USB_HID_RX_EP, NULL);
        usbd_reg_endpoint(dev, USB_HID_TX_EP, NULL);
        usbd_ep_deconfig(dev, USB_CDC_SB_EP);
        usbd_reg_endpoint(dev, USB_CDC_SB_EP, NULL);
        usbd_ep_deconfig(dev, USB_CDC_TX_EP);
        usbd_ep_deconfig(dev, USB_CDC_RX_EP);
        usbd_reg_endpoint(dev, USB_CDC_TX_EP, NULL);
        usbd_reg_endpoint(dev, USB_CDC_RX_EP, NULL);
        return usbd_ack;
    case 1:
        /* configuring device */
        usbd_ep_config(dev, USB_HID_RX_EP, USB_EPTYPE_INTERRUPT, USB_EPD_SIZE);
        usbd_ep_config(dev, USB_HID_TX_EP, USB_EPTYPE_INTERRUPT, USB_EPD_SIZE);
        usbd_reg_endpoint(dev, USB_HID_RX_EP, hid_data_xfer);
        usbd_reg_endpoint(dev, USB_HID_TX_EP, hid_data_xfer);
        usbd_ep_write(dev, USB_HID_TX_EP, 0, 0);
        usbd_ep_config(dev, USB_CDC_RX_EP, USB_EPTYPE_BULK /*| USB_EPTYPE_DBLBUF*/, USB_EPD_SIZE);
        usbd_ep_config(dev, USB_CDC_TX_EP, USB_EPTYPE_BULK /*| USB_EPTYPE_DBLBUF*/, USB_EPD_SIZE);
        usbd_ep_config(dev, USB_CDC_SB_EP, USB_EPTYPE_INTERRUPT, USB_EPC_SIZE);
        usbd_reg_endpoint(dev, USB_CDC_RX_EP, cdc_data_xfer);
        usbd_reg_endpoint(dev, USB_CDC_TX_EP, cdc_data_xfer);
        usbd_reg_endpoint(dev, USB_CDC_SB_EP, cdc_ctrl_xfer);
        usbd_ep_write(dev, USB_CDC_TX_EP, 0, 0);
        return usbd_ack;
    default:
        return usbd_fail;
    }
}

void usb_init(void) {
    usbd_init(&udev, &usbd_hw, USB_EPC_SIZE, ubuf, sizeof(ubuf));
    usbd_reg_config(&udev, usb_setconf);
    usbd_reg_control(&udev, usb_control);
    usbd_reg_descr(&udev, usb_getdesc);
    NVIC_EnableIRQ(USB_IRQn);
    usbd_enable(&udev, true);
    usbd_connect(&udev, true);
}

void USB_IRQHandler(void) {
    usbd_poll(&udev);
}

// Unfortunately this function has to be here due to data dependency. Otherwise it should have been in
// usbhid.c
usbd_respond hid_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
    switch (req->bmRequestType & USB_REQ_TYPE) {
    case USB_REQ_CLASS:
        return hid_class_control(dev, req, callback);

    case USB_REQ_STANDARD:
        if (req->bRequest == USB_STD_GET_DESCRIPTOR) {
            switch (req->wValue >> 8) {
                case USB_DTYPE_HID:
                    dev->status.data_ptr = (uint8_t*)&(config_desc.hid_desc);
                    dev->status.data_count = sizeof(config_desc.hid_desc);
                    return usbd_ack;
                case USB_DTYPE_HID_REPORT:
                    dev->status.data_ptr = (uint8_t*)hid_report_desc;
                    dev->status.data_count = sizeof(hid_report_desc);
                    return usbd_ack;
                default:
                    return usbd_fail;
            }
        } else { return usbd_fail; }
    
    default:
        return usbd_fail;
    }
}