/*
 * Header file for FBV UART functions
 */

#ifndef _FBV_UART_H
#define _FBV_UART_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_BOARD_MBHP_CORE_STM32)


// Tx buffer size (1..256)
#ifndef FBV_UART_TX_BUFFER_SIZE
#define FBV_UART_TX_BUFFER_SIZE 256
#endif

// Rx buffer size (1..256)
#ifndef FBV_UART_RX_BUFFER_SIZE
#define FBV_UART_RX_BUFFER_SIZE 256
#endif

// Baudrate of UART first interface
#ifndef FBV_UART_BAUDRATE
#define FBV_UART_BAUDRATE 31250
#endif

// should UART0 Tx pin configured for open drain (default) or push-pull mode?
#ifndef FBV_UART_TX_OD // was 1
#define FBV_UART_TX_OD 0
#endif

// Interface assignment: 0 = disabled, 1 = FBV, 2 = COM
#ifndef FBV_UART_ASSIGNMENT
#define FBV_UART_ASSIGNMENT 1
#endif


#define FBV_BUTTON_RELEASED 0x00
#define FBV_BUTTON_PRESSED	0x01
#define FBV_LED_OFF			0x00
#define FBV_LED_ON			0x01

#define FBV_CHANNEL_USER	'U'
#define FBV_CHANNEL_FACTORY	'F'


#define FBV_ID_TAP 				0x61
#define FBV_ID_DELAY			0x51
#define FBV_ID_MODULATION		0x41
#define FBV_ID_PITCH 			0x31
#define FBV_ID_REVERB 			0x21
#define FBV_ID_AMP2				0x11
#define FBV_ID_AMP1				0x01
#define FBV_ID_CHAN_FAV			0x60
#define FBV_ID_CHAN_D			0x50
#define FBV_ID_CHAN_C			0x40
#define FBV_ID_CHAN_B			0x30
#define FBV_ID_CHAN_A			0x20
#define FBV_ID_BANK_UP			0x10
#define FBV_ID_BANK_DOWN		0x00
#define FBV_ID_STOMP3			0x32
#define FBV_ID_STOMP2			0x22
#define FBV_ID_STOMP1			0x12
#define FBV_ID_FX_LOOP			0x02
#define FBV_ID_FOOT_CTRL_V_LED	0x23
#define FBV_ID_FOOT_CTRL_P2_LED	0x33
#define FBV_ID_FOOT_CTRL_W_LED	0x03
#define FBV_ID_FOOT_CTRL_P1_LED	0x13
#define FBV_ID_FOOT_CTRL_V_BTN	0x53
#define FBV_ID_FOOT_CTRL_W_BTN	0x43
#define FBV_ID_FOOT_CTRL_V_VAL	0x01
#define FBV_ID_FOOT_CTRL_W_VAL	0x00

#define FBV_ID_NONE				0xFF



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////
typedef struct {
    u8 header;
    u8 size;
    u8 cmd;
    u8 data[64];
    u8 pos;
} mios32_fbv_message_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 FBV_UART_Init(u32 mode);

extern s32 FBV_UART_RxBufferFree(void);
extern s32 FBV_UART_RxBufferUsed(void);
extern s32 FBV_UART_RxBufferGet(void);
extern s32 FBV_UART_RxBufferPeek(void);
extern s32 FBV_UART_RxBufferPut(u8 b);
extern s32 FBV_UART_TxBufferFree(void);
extern s32 FBV_UART_TxBufferUsed(void);
extern s32 FBV_UART_TxBufferGet(void);
extern s32 FBV_UART_TxBufferPut_NonBlocking(u8 b);
extern s32 FBV_UART_TxBufferPut(u8 b);
extern s32 FBV_UART_TxBufferPutMore_NonBlocking(u8 *buffer, u16 len);
extern s32 FBV_UART_TxBufferPutMore(u8 *buffer, u16 len);

extern s32 FBV_UART_RxBufferReceiveMessage(mios32_fbv_message_t *msg);

extern s32 FBV_UART_TxBufferSendInit(void);
extern s32 FBV_UART_TxBufferSendLedCommand(u8 led, u8 status);
extern s32 FBV_UART_TxBufferSendChannelCommand(u8 group, u8 nr, u8 ch);
extern s32 FBV_UART_TxBufferSendDisplay(u8 *buf, u8 len);

extern s32 FBV_UART_TxBufferSendTuner(u8 note, u8 flat);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* MIOS32_BOARD_MBHP_CORE_STM32 */

#endif /* _FBV_UART_H */
