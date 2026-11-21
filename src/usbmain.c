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

/*- Bring-up harness --------------------------------------------------------
 *
 * Records how far enumeration gets so a wedged device can be diagnosed from
 * the debugger instead of a scope. Read the block as:
 *
 *   usb_evt_count      > 0  host is driving the bus (SOFM fires at 1 kHz)
 *   usb_istr_last           ISTR as seen by the last interrupt
 *   usb_desc_count     > 0  host asked for a descriptor
 *   usb_desc_len_last       bytes handed back, USB_DESC_STALL if we refused
 *   usb_config_count   > 0  SET_CONFIGURATION reached usb_setconf()
 *   usb_log[]               last USB_LOG_SIZE {kind, a, b, c} tuples, oldest
 *                           first once usb_log_of is set; kind 0 = control
 *                           (bmRequestType<<8|bRequest, wValue, wLength),
 *                           1 = descriptor (wValue, wIndex, wLength),
 *                           2 = configuration (cfg)
 *
 * Serial-number strings are answered by the driver before usb_getdesc() runs,
 * so they do not appear in the log.
 */

#define USB_LOG_SIZE 16
#define USB_DESC_STALL 0xFFFF

enum {
    USB_LOG_CONTROL = 0,
    USB_LOG_DESC    = 1,
    USB_LOG_CONFIG  = 2,
};

volatile uint32_t usb_evt_count;
volatile uint32_t usb_istr_last;
volatile uint32_t usb_desc_count;
volatile uint16_t usb_desc_len_last;
volatile uint32_t usb_config_count;
volatile uint16_t usb_log[USB_LOG_SIZE][4];
volatile uint8_t  usb_log_idx;
volatile uint8_t  usb_log_of;

/* Interrupt-source histogram. Index is the first armed bit of ISTR, in the same
 * order evt_poll() checks them, so a storm names itself:
 *   0 CTR  1 RESET  2 SOF  3 WKUP  4 SUSP  5 ERR  6 ESOF  7 none-set
 * Index 7 climbing means the ISR is being entered with nothing pending at all.
 */
volatile uint32_t usb_evt_hist[8];
volatile uint16_t usb_istr_ring[8];
volatile uint8_t  usb_istr_idx;

static void usb_evt_tally(uint16_t istr) {
    if (istr & USB_ISTR_CTR)        usb_evt_hist[0]++;
    else if (istr & USB_ISTR_RESET) usb_evt_hist[1]++;
    else if (istr & USB_ISTR_SOF)   usb_evt_hist[2]++;
    else if (istr & USB_ISTR_WKUP)  usb_evt_hist[3]++;
    else if (istr & USB_ISTR_SUSP)  usb_evt_hist[4]++;
    else if (istr & USB_ISTR_ERR)   usb_evt_hist[5]++;
    else if (istr & USB_ISTR_ESOF)  usb_evt_hist[6]++;
    else                            usb_evt_hist[7]++;
}

static void usb_log_add(uint8_t kind, uint16_t a, uint16_t b, uint16_t c) {
    uint8_t i = usb_log_idx;
    usb_log[i][0] = kind;
    usb_log[i][1] = a;
    usb_log[i][2] = b;
    usb_log[i][3] = c;
    i = (i + 1) & (USB_LOG_SIZE - 1);
    usb_log_idx = i;
    if (i == 0) usb_log_of = 1;
}

static usbd_respond usb_getdesc(usbd_ctlreq *req, void **address, uint16_t *length) {
    const uint8_t dtype = req->wValue >> 8;
    const uint8_t dnumber = req->wValue & 0xFF;
    const void* desc;
    uint16_t len = 0;

    usb_desc_count++;
    usb_desc_len_last = USB_DESC_STALL;
    usb_log_add(USB_LOG_DESC, req->wValue, req->wIndex, req->wLength);

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
    usb_desc_len_last = len;
    *address = (void*)desc;
    *length = len;
    return usbd_ack;
}

static usbd_respond usb_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
    usb_log_add(USB_LOG_CONTROL, ((uint16_t)req->bmRequestType << 8) | req->bRequest,
                req->wValue, req->wLength);

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

static void usb_teardown(usbd_device *dev) {
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
}

static usbd_respond usb_setconf(usbd_device *dev, uint8_t cfg) {
    usb_config_count++;
    usb_log_add(USB_LOG_CONFIG, cfg, 0, 0);

    usb_teardown(dev);
    switch (cfg) {
    case 0:
        /* deconfiguring device */
        hid_deinit();
        cdc_deinit();
        return usbd_ack;
    case 1:
        /* configuring device */
        cdc_init();
        hid_init();
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
    uint16_t istr = USB->ISTR;

    usb_evt_count++;
    usb_istr_last = istr;
    usb_istr_ring[usb_istr_idx++ & 7] = istr;
    usb_evt_tally(istr);

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