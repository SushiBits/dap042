/**
 * @file usbdefs.h
 * 
 * Static metadata used for USB peripheral.
 */

#ifndef _USBDEFS_H_
#define _USBDEFS_H_

#include "stm32f0xx.h"
#include "usb_std.h"
#include "usb_hid.h"
#include "usb_cdc.h"

#define USB_VID 0x0002 // Ingram, deprecated VID.
#define USB_PID 0xDA42

#define USB_STRING_VENDOR "SushiBits"
#define USB_STRING_PRODUCT "Embedded DAP042 CMSIS-DAP"

#define USB_EPC_SIZE 8 // Control EP
#define USB_EPD_SIZE 64 // Data EP
#define USB_HID_IF 0
#define USB_HID_RX_EP (USB_EPDIR_IN | 0x01)
#define USB_HID_TX_EP (USB_EPDIR_OUT | 0x01)
#define USB_CDC_IF 1
#define USB_CDC_SB_EP (USB_EPDIR_OUT | 0x02)
#define USB_CDC_DATA_IF 2
#define USB_CDC_RX_EP (USB_EPDIR_IN | 0x03)
#define USB_CDC_TX_EP (USB_EPDIR_OUT | 0x03)

#if defined(_USBMAIN_C_)

static const uint8_t hid_report_desc[] = {
    HID_RI_USAGE_PAGE(16, 0x00FF),
    HID_USAGE(1),
    HID_COLLECTION(HID_APPLICATION_COLLECTION),
        HID_LOGICAL_MINIMUM(0),
        HID_RI_LOGICAL_MAXIMUM(16, 0x00FF),
        HID_REPORT_SIZE(8),
        HID_REPORT_COUNT(64),
        HID_USAGE(1),
        HID_INPUT(HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
        HID_REPORT_COUNT(64),
        HID_USAGE(1),
        HID_OUTPUT(HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
    HID_END_COLLECTION
};

static const struct usb_device_descriptor device_desc = {
    .bLength            = sizeof(struct usb_device_descriptor),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = VERSION_BCD(2,0,0),
    .bDeviceClass       = USB_CLASS_IAD,
    .bDeviceSubClass    = USB_SUBCLASS_IAD,
    .bDeviceProtocol    = USB_PROTO_IAD,
    .bMaxPacketSize0    = USB_EPC_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = VERSION_BCD(1,0,0),
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = INTSERIALNO_DESCRIPTOR,
    .bNumConfigurations = 1,
};

static const struct usb_config {
    struct usb_config_descriptor        config;
    struct usb_iad_descriptor           hid_iad;
    struct usb_interface_descriptor     hid;
    struct usb_hid_descriptor           hid_desc;
    struct usb_endpoint_descriptor      hid_ep[2];
    struct usb_iad_descriptor           comm_iad;
    struct usb_interface_descriptor     comm;
    struct usb_cdc_header_desc          cdc_hdr;
    struct usb_cdc_call_mgmt_desc       cdc_mgmt;
    struct usb_cdc_acm_desc             cdc_acm;
    struct usb_cdc_union_desc           cdc_union;
    struct usb_endpoint_descriptor      comm_ep[1];
    struct usb_interface_descriptor     data;
    struct usb_endpoint_descriptor      data_ep[2];
} config_desc = {
    .config = {
        .bLength                = sizeof(struct usb_config_descriptor),
        .bDescriptorType        = USB_DTYPE_CONFIGURATION,
        .wTotalLength           = sizeof(struct usb_config),
        .bNumInterfaces         = 3,
        .bConfigurationValue    = 1,
        .iConfiguration         = NO_DESCRIPTOR,
        .bmAttributes           = USB_CFG_ATTR_RESERVED | USB_CFG_ATTR_SELFPOWERED,
        .bMaxPower              = USB_CFG_POWER_MA(500),
    },
    .hid_iad = {
        .bLength = sizeof(struct usb_iad_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFASEASSOC,
        .bFirstInterface        = USB_HID_IF,
        .bInterfaceCount        = 1,
        .bFunctionClass         = USB_CLASS_HID,
        .bFunctionSubClass      = USB_HID_SUBCLASS_NONBOOT,
        .bFunctionProtocol      = USB_HID_PROTO_NONBOOT,
        .iFunction              = NO_DESCRIPTOR,
    },
    .hid = {
        .bLength                = sizeof(struct usb_interface_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = USB_HID_IF,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = USB_CLASS_HID,
        .bInterfaceSubClass     = USB_HID_SUBCLASS_NONBOOT,
        .bInterfaceProtocol     = USB_HID_PROTO_NONBOOT,
        .iInterface             = NO_DESCRIPTOR,
    },
    .hid_desc = {
        .bLength                = sizeof(struct usb_hid_descriptor),
        .bDescriptorType        = USB_DTYPE_HID,
        .bcdHID                 = VERSION_BCD(1,0,0),
        .bCountryCode           = USB_HID_COUNTRY_NONE,
        .bNumDescriptors        = 1,
        .bDescriptorType0       = USB_DTYPE_HID_REPORT,
        .wDescriptorLength0     = sizeof(hid_report_desc),
    },
    .hid_ep = {
        {
            .bLength                = sizeof(struct usb_endpoint_descriptor),
            .bDescriptorType        = USB_DTYPE_ENDPOINT,
            .bEndpointAddress       = USB_HID_RX_EP,
            .bmAttributes           = USB_EPTYPE_INTERRUPT,
            .wMaxPacketSize         = USB_EPD_SIZE,
            .bInterval              = 1,
        },
        {
            .bLength                = sizeof(struct usb_endpoint_descriptor),
            .bDescriptorType        = USB_DTYPE_ENDPOINT,
            .bEndpointAddress       = USB_HID_TX_EP,
            .bmAttributes           = USB_EPTYPE_INTERRUPT,
            .wMaxPacketSize         = USB_EPD_SIZE,
            .bInterval              = 1,
        },
    },
    .comm_iad = {
        .bLength = sizeof(struct usb_iad_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFASEASSOC,
        .bFirstInterface        = USB_CDC_IF,
        .bInterfaceCount        = 2,
        .bFunctionClass         = USB_CLASS_CDC,
        .bFunctionSubClass      = USB_CDC_SUBCLASS_ACM,
        .bFunctionProtocol      = USB_CDC_PROTO_NONE,
        .iFunction              = NO_DESCRIPTOR,
    },
    .comm = {
        .bLength                = sizeof(struct usb_interface_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = USB_CDC_IF,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = USB_CLASS_CDC,
        .bInterfaceSubClass     = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol     = USB_CDC_PROTO_NONE,
        .iInterface             = NO_DESCRIPTOR,
    },
    .cdc_hdr = {
        .bFunctionLength        = sizeof(struct usb_cdc_header_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_HEADER,
        .bcdCDC                 = VERSION_BCD(1,1,0),
    },
    .cdc_mgmt = {
        .bFunctionLength        = sizeof(struct usb_cdc_call_mgmt_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_CALL_MANAGEMENT,
        .bmCapabilities         = 0,
        .bDataInterface         = USB_CDC_DATA_IF,
    },
    .cdc_acm = {
        .bFunctionLength        = sizeof(struct usb_cdc_acm_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_ACM,
        .bmCapabilities         = 0,
    },
    .cdc_union = {
        .bFunctionLength        = sizeof(struct usb_cdc_union_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_UNION,
        .bMasterInterface0      = USB_CDC_IF,
        .bSlaveInterface0       = USB_CDC_DATA_IF,
    },
    .comm_ep = {{
        .bLength                = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = USB_CDC_SB_EP,
        .bmAttributes           = USB_EPTYPE_INTERRUPT,
        .wMaxPacketSize         = USB_EPC_SIZE,
        .bInterval              = 0xFF,
    }},
    .data = {
        .bLength                = sizeof(struct usb_interface_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = USB_CDC_DATA_IF,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = USB_CLASS_CDC_DATA,
        .bInterfaceSubClass     = USB_SUBCLASS_NONE,
        .bInterfaceProtocol     = USB_PROTO_NONE,
        .iInterface             = NO_DESCRIPTOR,
    },
    .data_ep = {
        {
            .bLength                = sizeof(struct usb_endpoint_descriptor),
            .bDescriptorType        = USB_DTYPE_ENDPOINT,
            .bEndpointAddress       = USB_CDC_RX_EP,
            .bmAttributes           = USB_EPTYPE_BULK,
            .wMaxPacketSize         = USB_EPD_SIZE,
            .bInterval              = 0x01,
        },
        {
            .bLength                = sizeof(struct usb_endpoint_descriptor),
            .bDescriptorType        = USB_DTYPE_ENDPOINT,
            .bEndpointAddress       = USB_CDC_TX_EP,
            .bmAttributes           = USB_EPTYPE_BULK,
            .wMaxPacketSize         = USB_EPD_SIZE,
            .bInterval              = 0x01,
        }
    },
};

static const struct usb_string_descriptor lang_desc     = USB_ARRAY_DESC(USB_LANGID_ENG_US);
static const struct usb_string_descriptor manuf_desc_en = USB_STRING_DESC(USB_STRING_VENDOR);
static const struct usb_string_descriptor prod_desc_en  = USB_STRING_DESC(USB_STRING_PRODUCT);
static const struct usb_string_descriptor *const usb_string_table[] = {
    &lang_desc,
    &manuf_desc_en,
    &prod_desc_en,
};

#endif // defined(_USBMAIN_C_)

#endif // defined(_USBDEFS_H_)
