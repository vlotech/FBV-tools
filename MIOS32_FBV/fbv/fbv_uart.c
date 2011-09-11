//! \defgroup FBV_UART
//!
//! FBV USART functions for MIOS32
//!
//! Applications shouldn't call these functions directly, instead please use \ref FBV layer functions
//!
//! \{

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "fbv_uart.h"

#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

/////////////////////////////////////////////////////////////////////////////
// Pin definitions and USART mappings
/////////////////////////////////////////////////////////////////////////////

#define FBV_UART_TX_PORT     GPIOA
#define FBV_UART_TX_PIN      GPIO_Pin_2
#define FBV_UART_RX_PORT     GPIOA
#define FBV_UART_RX_PIN      GPIO_Pin_3
#define FBV_UART             USART2
#define FBV_UART_IRQ_CHANNEL USART2_IRQn
#define FBV_UART_IRQHANDLER_FUNC void USART2_IRQHandler(void)
#define FBV_UART_REMAP_FUNC  {}


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 rx_buffer[FBV_UART_RX_BUFFER_SIZE];
static volatile u8 rx_buffer_tail;
static volatile u8 rx_buffer_head;
static volatile u8 rx_buffer_size;

static u8 tx_buffer[FBV_UART_TX_BUFFER_SIZE];
static volatile u8 tx_buffer_tail;
static volatile u8 tx_buffer_head;
static volatile u8 tx_buffer_size;


/////////////////////////////////////////////////////////////////////////////
//! Initializes UART interfaces
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_Init(u32 mode)
{

	USART_DeInit(FBV_UART);

	GPIO_InitTypeDef GPIO_InitStructure;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // map UART pins
  FBV_UART_REMAP_FUNC;

  // configure UART pins
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

  // outputs as open-drain
  GPIO_InitStructure.GPIO_Pin = FBV_UART_TX_PIN;
#if FBV_UART_TX_OD
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
#else
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
#endif
  GPIO_Init(FBV_UART_TX_PORT, &GPIO_InitStructure);

  // inputs with internal pull-up
  //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Pin = FBV_UART_RX_PIN;
  GPIO_Init(FBV_UART_RX_PORT, &GPIO_InitStructure);

  // enable all USART clocks
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

  // USART configuration
  USART_InitTypeDef USART_InitStructure;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_InitStructure.USART_BaudRate = FBV_UART_BAUDRATE;
  USART_Init(FBV_UART, &USART_InitStructure);

  // configure and enable UART interrupts
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_InitStructure.NVIC_IRQChannel = FBV_UART_IRQ_CHANNEL;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MIOS32_IRQ_UART_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  USART_ITConfig(FBV_UART, USART_IT_RXNE, ENABLE);
  //USART_ITConfig(FBV_UART, USART_IT_TXE, ENABLE);

  // clear buffer counters
  rx_buffer_tail = rx_buffer_head = rx_buffer_size = 0;
  tx_buffer_tail = tx_buffer_head = tx_buffer_size = 0;

  // enable UARTs
  USART_Cmd(FBV_UART, ENABLE);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of free bytes in receive buffer
//! \param[in] uart UART number (0..1)
//! \return uart number of free bytes
//! \return 1: uart available
//! \return 0: uart not available
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_RxBufferFree(void)
{
    return FBV_UART_RX_BUFFER_SIZE - rx_buffer_size;
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of used bytes in receive buffer
//! \param[in] uart UART number (0..1)
//! \return > 0: number of used bytes
//! \return 0 if uart not available
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_RxBufferUsed(void)
{
    return rx_buffer_size;
}


/////////////////////////////////////////////////////////////////////////////
//! gets a byte from the receive buffer
//! \return -1 if no new byte available
//! \return >= 0: number of received bytes
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_RxBufferGet(void)
{
   if( !rx_buffer_size )
    return -1; // nothing new in buffer

  // get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  u8 b = rx_buffer[rx_buffer_tail];
  if( ++rx_buffer_tail >= FBV_UART_RX_BUFFER_SIZE )
    rx_buffer_tail = 0;
  --rx_buffer_size;
  MIOS32_IRQ_Enable();

  return b; // return received byte
}


/////////////////////////////////////////////////////////////////////////////
//! returns the next byte of the receive buffer without taking it
//! \return -1 if no new byte available
//! \return >= 0: number of received bytes
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_RxBufferPeek(void)
{
  if( !rx_buffer_size )
    return -1; // nothing new in buffer

  // get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  u8 b = rx_buffer[rx_buffer_tail];
  MIOS32_IRQ_Enable();

  return b; // return received byte
}


/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the receive buffer
//! \param[in] b byte which should be put into Rx buffer
//! \return 0 if no error
//! \return -1 if buffer full (retry)
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_RxBufferPut( u8 b)
{

  if( rx_buffer_size >= FBV_UART_RX_BUFFER_SIZE )
    return -1; // buffer full (retry)

  // copy received byte into receive buffer
  // this operation should be atomic!
  MIOS32_IRQ_Disable();
  rx_buffer[rx_buffer_head] = b;
  if( ++rx_buffer_head >= FBV_UART_RX_BUFFER_SIZE )
    rx_buffer_head = 0;
  ++rx_buffer_size;
  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of free bytes in transmit buffer
//! \return number of free bytes
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_TxBufferFree(void)
{
    return FBV_UART_TX_BUFFER_SIZE - tx_buffer_size;
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of used bytes in transmit buffer
//! \return number of used bytes
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_TxBufferUsed(void)
{
    return tx_buffer_size;
}


/////////////////////////////////////////////////////////////////////////////
//! gets a byte from the transmit buffer
//! \return -1 if no new byte available
//! \return >= 0: transmitted byte
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_TxBufferGet(void)
{

  if( !tx_buffer_size )
    return -1; // nothing new in buffer

  // get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  u8 b = tx_buffer[tx_buffer_tail];
  if( ++tx_buffer_tail >= FBV_UART_TX_BUFFER_SIZE )
    tx_buffer_tail = 0;
  --tx_buffer_size;
  MIOS32_IRQ_Enable();

  return b; // return transmitted byte
}


/////////////////////////////////////////////////////////////////////////////
//! puts more than one byte onto the transmit buffer (used for atomic sends)
//! \param[in] *buffer pointer to buffer to be sent
//! \param[in] len number of bytes to be sent
//! \return 0 if no error
//! \return -1 if buffer full or cannot get all requested bytes (retry)
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_TxBufferPutMore_NonBlocking( u8 *buffer, u16 len)
{

  if( (tx_buffer_size+len) >= FBV_UART_TX_BUFFER_SIZE )
    return -1; // buffer full or cannot get all requested bytes (retry)

  // copy bytes to be transmitted into transmit buffer
  // this operation should be atomic!
  MIOS32_IRQ_Disable();

  u16 i;
  for(i=0; i<len; ++i) {
    tx_buffer[tx_buffer_head] = *buffer++;

    if( ++tx_buffer_head >= FBV_UART_TX_BUFFER_SIZE )
      tx_buffer_head = 0;

    // enable Tx interrupt if buffer was empty
    if( ++tx_buffer_size == 1 ) {
        FBV_UART->CR1 |= (1 << 7); // enable TXE interrupt (TXEIE=1)
    }
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! puts more than one byte onto the transmit buffer (used for atomic sends)<BR>
//! (blocking function)
//! \param[in] *buffer pointer to buffer to be sent
//! \param[in] len number of bytes to be sent
//! \return 0 if no error
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_TxBufferPutMore(u8 *buffer, u16 len)
{
  s32 error;

  while( (error=FBV_UART_TxBufferPutMore_NonBlocking(buffer, len)) == -1 );

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the transmit buffer
//! \param[in] b byte which should be put into Tx buffer
//! \return 0 if no error
//! \return -1 if buffer full (retry)
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_TxBufferPut_NonBlocking(u8 b)
{
  // for more comfortable usage...
  // -> just forward to FBV_UART_TxBufferPutMore
  return FBV_UART_TxBufferPutMore_NonBlocking(&b, 1);
}


/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the transmit buffer<BR>
//! (blocking function)
//! \param[in] b byte which should be put into Tx buffer
//! \return 0 if no error
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_TxBufferPut(u8 b)
{
  s32 error;

  while( (error=FBV_UART_TxBufferPutMore(&b, 1)) == -1 );

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! returns the size of the next message in the receive buffer without taking it
//! \return -1 if no new byte available
//! \return 0 if no new message available
//! \return > 0: size of received message
//! \note Applications shouldn't call these functions directly, instead please use \ref FBV
/////////////////////////////////////////////////////////////////////////////
s32 FBV_UART_RxBufferReceiveMessage(mios32_fbv_message_t *target)
{
  if( !rx_buffer_size )
    return -1; // nothing new in buffer

  static mios32_fbv_message_t msg = {0};

  s32 status = 0;
  s32 retval = -1;
  u8 b = 0x00;

	if((status = FBV_UART_RxBufferGet()) >= 0) {
		b = (u8)status;

		//DEBUG_MSG("input: %02X\n", (u8)b);


		if(b == 0xF0) {
			bzero(&msg,sizeof(mios32_fbv_message_t));
		}

		if( msg.header != 0xF0) {
		  if( b == 0xF0) {
			  msg.header = b;
		  }
		} else if (msg.size == 0) {
		  msg.size = b;
		  if(msg.size == (msg.pos + 0))
			  retval = 0;
		} else if (msg.cmd == 0) {
		  msg.cmd = b;
		  if(msg.size == (msg.pos + 1))
			  retval = 0;
		} else {
		  msg.data[msg.pos++] = (u8)status;
		  if(msg.size == (msg.pos + 1))
			  retval = 0;
		}

	}

  if(retval == 0) {
	  memcpy(target,&msg,sizeof(mios32_fbv_message_t));
//	  bzero(&msg,sizeof(mios32_fbv_message_t))
	  DEBUG_MSG("\n\nFBV Message DONE\n\n");
  }
  return retval; // return received byte
}

s32 FBV_UART_TxBufferSendInit(void)
{
	FBV_UART_TxBufferPut(0xF0); //header
	FBV_UART_TxBufferPut(0x02); //size
	FBV_UART_TxBufferPut(0x01); // version cmd?!?
	FBV_UART_TxBufferPut(0x00);  // version 0?!?

	return 0;
}


s32 FBV_UART_TxBufferSendLedCommand(u8 led, u8 status)
{
	u8 led_inner = led;
	if( led == FBV_ID_FOOT_CTRL_W_BTN) led_inner = FBV_ID_FOOT_CTRL_P1_LED;
	if( led == FBV_ID_FOOT_CTRL_V_BTN) led_inner = FBV_ID_FOOT_CTRL_V_LED;

	FBV_UART_TxBufferPut(0xF0); //header
	FBV_UART_TxBufferPut(0x03); //size
	FBV_UART_TxBufferPut(0x04); // Led command
	FBV_UART_TxBufferPut(led_inner);  // led ID
	FBV_UART_TxBufferPut(status); // led state

	return 0;
}

s32 FBV_UART_TxBufferSendChannelCommand(u8 group, u8 nr, u8 ch)
{
	FBV_UART_TxBufferPut(0xF0); //header
	FBV_UART_TxBufferPut(0x05); //size
	FBV_UART_TxBufferPut(0x08); // Channel command
	FBV_UART_TxBufferPut(group);  // group (F/U)
	FBV_UART_TxBufferPut(0x20);  // 'space'
	FBV_UART_TxBufferPut(nr); // Channel number
	FBV_UART_TxBufferPut(ch); // Channel char
	FBV_UART_TxBufferPut(0xF0); //header
	FBV_UART_TxBufferPut(0x02); //size
	FBV_UART_TxBufferPut(0x20); // Channel command
	FBV_UART_TxBufferPut(0x00);  // 0 = off, 1 = on

	return 0;
}

s32 FBV_UART_TxBufferSendDisplay(u8 *buf, u8 len)
{
	FBV_UART_TxBufferPut(0xF0); //header
	FBV_UART_TxBufferPut(0x13); //size
	FBV_UART_TxBufferPut(0x10); // Channel command
	FBV_UART_TxBufferPut(0x00);  // ???
	FBV_UART_TxBufferPut(0x10);  // ???
	int i;
	for(i=0; i < len && i < 16; i++)
		FBV_UART_TxBufferPut(buf[i]); // chars
	for(i=0; i < 16; i++)
		FBV_UART_TxBufferPut(0x20); // 'space'
	return 0;
}

s32 FBV_UART_TxBufferSendTuner(u8 note, u8 flat)
{
	FBV_UART_TxBufferPut(0xF0); //header
	FBV_UART_TxBufferPut(0x05); //size
	FBV_UART_TxBufferPut(0x08); // Channel command
	FBV_UART_TxBufferPut(0x20);  // group (F/U)
	FBV_UART_TxBufferPut(0x20);  // 'space'
	FBV_UART_TxBufferPut(0x20); // Channel number
	FBV_UART_TxBufferPut(0x20); // Channel char
	FBV_UART_TxBufferPut(0xF0); //header
	FBV_UART_TxBufferPut(0x02); //size
	FBV_UART_TxBufferPut(0x0C); // note
	FBV_UART_TxBufferPut(note);  // ASCII
	FBV_UART_TxBufferPut(0xF0); //header
	FBV_UART_TxBufferPut(0x02); //size
	FBV_UART_TxBufferPut(0x20); // Channel command
	FBV_UART_TxBufferPut(flat);  // 0 = off, 1 = on
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for fbv UART
/////////////////////////////////////////////////////////////////////////////
FBV_UART_IRQHANDLER_FUNC
{
  if( FBV_UART->SR & (1 << 5) ) { // check if RXNE flag is set
    u8 b = FBV_UART->DR;

    //s32 status = MIOS32_MIDI_SendByteToRxCallback(UART0, b);

    //if( status == 0 && FBV_UART_RxBufferPut(0, b) < 0 ) {
    if( FBV_UART_RxBufferPut( b) < 0 ) {

    	// here we could add some error handling
    	//DEBUG_MSG("errin:\n");
    } else {
    	//DEBUG_MSG("input: %02X\n", (u8)b);
    }

  }

  if( FBV_UART->SR & (1 << 7) ) { // check if TXE flag is set
    if( FBV_UART_TxBufferUsed() > 0 ) {
      s32 b = FBV_UART_TxBufferGet();

      if( b < 0 ) {
    	  // here we could add some error handling
    	  FBV_UART->DR = 0xff;
    	  //DEBUG_MSG("errout: %02X\n", (u8)b);
      } else {
    	  FBV_UART->DR = (u8)b;
    	  //DEBUG_MSG("output: %02X\n", (u8)b);
      }
    } else {
      FBV_UART->CR1 &= ~(1 << 7); // disable TXE interrupt (TXEIE=0)
    }
  }
}


