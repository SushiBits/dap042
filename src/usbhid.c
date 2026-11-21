/**
 * @file usbhid.c
 * 
 * Implementation for USB HID interface.
 */


#include "usbhid.h"

#include "stm32f0xx.h"
#include "usb.h"
#include "usbdefs.h"
#include "usbmain.h"

#include <string.h>

const size_t hid_size = USB_EPD_SIZE;
uint8_t rx_data[USB_EPD_SIZE], tx_data[USB_EPD_SIZE];
volatile bool rx_drdy, tx_drdy;

void hid_init(void) {
    memset(rx_data, 0, USB_EPD_SIZE);
    memset(tx_data, 0, USB_EPD_SIZE);
    rx_drdy = tx_drdy = false;
}

void hid_deinit(void) { rx_drdy = tx_drdy = false; }

usbd_respond hid_class_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
    switch (req->bRequest) {
        case USB_HID_SETIDLE:
            return usbd_ack;
        case USB_HID_GETREPORT:
            dev->status.data_ptr = &tx_data;
            dev->status.data_count = sizeof(tx_data);
            return usbd_ack;
        default:
            return usbd_fail;
        }
    return usbd_fail; // Stub
}

void hid_data_xfer(usbd_device *dev, uint8_t event, uint8_t ep) {
    switch (event) {
        case usbd_evt_eprx:
            hid_data_rx();
            break;

        case usbd_evt_eptx:
            hid_data_tx();
            break;

        default: break;
        }
}

void hid_data_rx(void) {
    if (rx_drdy) return;

    uint32_t pm = __get_PRIMASK();
    __disable_irq();
    int32_t rb = usbd_ep_read(&udev, USB_HID_RX_EP, rx_data, USB_EPD_SIZE);
    if (rb > 0) rx_drdy = true;
    __set_PRIMASK(pm);
}

void hid_data_tx(void) {
    if (!tx_drdy) return;

    uint32_t pm = __get_PRIMASK();
    __disable_irq();
    int32_t wb = usbd_ep_write(&udev, USB_HID_TX_EP, tx_data, USB_EPD_SIZE);
    if (wb > 0) tx_drdy = false;
    __set_PRIMASK(pm);
}
