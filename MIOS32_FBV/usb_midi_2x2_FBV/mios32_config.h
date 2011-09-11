// $Id: mios32_config.h 662 2009-07-25 22:40:36Z tk $
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

#define MIOS32_DONT_USE_SPI
#define MIOS32_DONT_USE_SPI0
#define MIOS32_DONT_USE_SPI1
#define MIOS32_DONT_USE_SPI2
#define MIOS32_DONT_USE_SRIO
#define MIOS32_DONT_USE_DIN
#define MIOS32_DONT_USE_DOUT
#define MIOS32_DONT_USE_ENC
#define MIOS32_DONT_USE_AIN
#define MIOS32_DONT_USE_MF
#define MIOS32_DONT_USE_LCD
#define MIOS32_DONT_USE_OSC
#define MIOS32_DONT_USE_IIC
#define MIOS32_DONT_USE_IIC_MIDI
#define MIOS32_DONT_USE_SDCARD
#define MIOS32_DONT_USE_ENC28J60

// Using the first 2 uarts based on the MIOS32 code the 3rd will be based on FBV code
#define MIOS32_UART_NUM 2


// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "USB MIDI 2x2 + FBV"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2009 VLoTech"

// define a unique VID/PID for this application
#define MIOS32_USB_VENDOR_ID    0x16c0        // sponsored by voti.nl! see http://www.voti.nl/pids
#define MIOS32_USB_VENDOR_STR   "VLoTech.nl" // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  "USB MIDI 2x2 + FBV" // you will see this in the MIDI device list
#define MIOS32_USB_PRODUCT_ID   1005          // 1000..1009 are free for lab use

#define MIOS32_USB_MIDI_NUM_PORTS 2           // we provide 2 USB ports


#endif /* _MIOS32_CONFIG_H */
