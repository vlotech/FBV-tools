// $Id: app.c 722 2009-09-13 11:25:43Z tk $
/*
 * MIDI USB 2x2 Interface Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app.h"
#include "axefx_info.h"

#include "fbv_uart.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
// should be enabled for this application!
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#define RACK_MIDI_CHN	Chn1

#define AXEFX_PORT	UART1

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////
#define PRIORITY_TASK_FBV_CHECK		( tskIDLE_PRIORITY + 2 )


/////////////////////////////////////////////////////////////////////////////
// AxeFX SysEx stuff
/////////////////////////////////////////////////////////////////////////////
#define SYSEX_HEADER          0
#define SYSEX_CMD_STATE_BEGIN 1
#define SYSEX_CMD_STATE_CONT  2
#define SYSEX_CMD_STATE_END   3
#define SYSEX_MAX_LEN		  256

static u8 sysex_state = SYSEX_HEADER;
static u8 sysex_count = 0;
static u8 sysex_axefx_type = 0;
static u8 sysex_cmd;


const u8 sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7d, 0x01 };
u8 sysex_buffer[SYSEX_MAX_LEN];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 ms_counter;

static  u8 midi_channel = 0x01;
static  u8 midi_bank_size = 0x04;
static  u8 midi_bank = 0x00;
static const u8 bank_ids[10] = {FBV_ID_CHAN_A, FBV_ID_CHAN_B, FBV_ID_CHAN_C, FBV_ID_CHAN_D, FBV_ID_CHAN_FAV,
								  FBV_ID_REVERB, FBV_ID_PITCH, FBV_ID_MODULATION, FBV_ID_DELAY, FBV_ID_TAP};
enum {
	FBV_ID_TYPE_BTN_LED,
	FBV_ID_TYPE_BTN_ONLY,
	FBV_ID_TYPE_LED_ONLY,
	FBV_ID_TYPE_FOOT_CTRL,
	FBV_ID_TYPE_TEMPO,
	FBV_ID_TYPE_BANK,  // cc = direction 0 = down, 1 = up
	FBV_ID_TYPE_PRESET, // cc = offset on preset
	FBV_ID_TYPE_TEMPO_TUNER,
	FBV_ID_TYPE_NONE
};

enum {
	FBV_ID_TAP_i,
	FBV_ID_DELAY_i,
	FBV_ID_MODULATION_i,
	FBV_ID_PITCH_i,
	FBV_ID_REVERB_i,
	FBV_ID_AMP2_i,
	FBV_ID_AMP1_i,
	FBV_ID_CHAN_FAV_i,
	FBV_ID_CHAN_D_i,
	FBV_ID_CHAN_C_i,
	FBV_ID_CHAN_B_i,
	FBV_ID_CHAN_A_i,
	FBV_ID_BANK_UP_i,
	FBV_ID_BANK_DOWN_i,
	FBV_ID_STOMP3_i,
	FBV_ID_STOMP2_i,
	FBV_ID_STOMP1_i,
	FBV_ID_FX_LOOP_i,
//	FBV_ID_FOOT_CTRL_V_LED_i,  //in lower layers the led is translated to the button
//	FBV_ID_FOOT_CTRL_P2_LED_i, // not in use!!
//	FBV_ID_FOOT_CTRL_W_LED_i,  //in lower layers the led is translated to the button
//	FBV_ID_FOOT_CTRL_P1_LED_i, // not in use!!
	FBV_ID_FOOT_CTRL_V_BTN_i,
	FBV_ID_FOOT_CTRL_W_BTN_i,
	FBV_ID_MAX_INDEX
};

enum {
	FBV_ID_FOOT_CTRL_V_VAL_i,
	FBV_ID_FOOT_CTRL_W_VAL_i
};

enum {
	FBV_ID_OFF,
	FBV_ID_ON,
};

#define FBV_ID_MAX_BLOCKS 8

typedef struct {
	u8 fbv_id;
	u8 type;
	u8 cc;
	u8 status;
	u8 len;
	axefx_block_status_struct blocks[FBV_ID_MAX_BLOCKS];
} fbv_ctrl_t;

typedef struct {
	u8 fbv_id_foot;
	u8 fbv_id_btn;
	u8 fbv_id_led1;
	u8 fbv_id_led2;
	u8 cc;
	u8 cc_value1;
	u8 cc_value2;
	u8 status;
	u8 len;
	axefx_block_status_struct blocks[FBV_ID_MAX_BLOCKS];
} fbv_footctrl_t;

#define FBV_ID_MAX_FOOT_INDEX 2

fbv_ctrl_t FBV_ctrls[FBV_ID_MAX_INDEX] = {};
fbv_footctrl_t FBV_ctrls_cont[FBV_ID_MAX_FOOT_INDEX] = {};




static const u8 block_to_id[] = {
		FBV_ID_FX_LOOP_i,//    ID_COMP1 = 100,
		FBV_ID_NONE,//    ID_COMP2,
		FBV_ID_NONE,//    ID_GRAPHEQ1,
		FBV_ID_NONE,//    ID_GRAPHEQ2,
		FBV_ID_NONE,//    ID_PARAEQ1,
		FBV_ID_NONE,//    ID_PARAEQ2,
		FBV_ID_NONE,//    ID_AMP1,
		FBV_ID_NONE,//    ID_AMP2,
		FBV_ID_NONE,//    ID_CAB1,
		FBV_ID_NONE,//    ID_CAB2,
		FBV_ID_REVERB_i,//    ID_REVERB1,
		FBV_ID_NONE,//    ID_REVERB2,
		FBV_ID_DELAY_i, //    ID_DELAY1,
		FBV_ID_NONE, //    ID_DELAY2,
		FBV_ID_NONE, //    ID_MULTITAP1,
		FBV_ID_NONE, //    ID_MULTITAP2,
		FBV_ID_STOMP2_i,//    ID_CHORUS1,
		FBV_ID_NONE,//    ID_CHORUS2,
		FBV_ID_STOMP1_i,//    ID_FLANGER1,
		FBV_ID_NONE,//    ID_FLANGER2,
		FBV_ID_MODULATION_i,//    ID_ROTARY1,
		FBV_ID_NONE,//    ID_ROTARY2,
		FBV_ID_AMP1_i,//    ID_PHASER1,
		FBV_ID_NONE,//    ID_PHASER2,
		FBV_ID_FOOT_CTRL_W_BTN_i,//    ID_WAH1,
		FBV_ID_NONE,//    ID_WAH2,
		FBV_ID_NONE,//    ID_FORMANT1,
		FBV_ID_FOOT_CTRL_V_BTN_i,//    ID_VOLUME1,
		FBV_ID_PITCH_i,//    ID_TREMOLO1,
		FBV_ID_NONE,//    ID_TREMOLO2,
		FBV_ID_AMP2_i,//    ID_PITCH1,
		FBV_ID_CHAN_FAV_i,//    ID_FILTER1,
		FBV_ID_NONE,//    ID_FILTER2,
		FBV_ID_STOMP3_i,//    ID_DRIVE1,
		FBV_ID_NONE,//    ID_DRIVE2,
		FBV_ID_NONE,//    ID_ENHANCER1,
		FBV_ID_NONE,//    ID_LOOP1,
		FBV_ID_NONE,//    ID_MIXER1,
		FBV_ID_NONE,//    ID_MIXER2,
		FBV_ID_NONE,//    ID_NOISEGATE1,
		FBV_ID_NONE,//    ID_OUT1,
		FBV_ID_NONE,//    ID_CONTROL,
		FBV_ID_NONE,//    ID_FBSEND,
		FBV_ID_NONE,//    ID_FBRETURN,
		FBV_ID_NONE,//    ID_SYNTH1,
		FBV_ID_NONE,//    ID_SYNTH2,
		FBV_ID_NONE,//    ID_VOCODER1,
		FBV_ID_NONE, //    ID_MEGATAP1,
		FBV_ID_NONE,//    ID_CROSSOVER1,
		FBV_ID_NONE,//    ID_CROSSOVER2,
		FBV_ID_NONE,//    ID_GATE1,
		FBV_ID_NONE,//    ID_GATE2,
		FBV_ID_NONE,//    ID_RINGMOD1,
		FBV_ID_NONE,//    ID_PITCH2,
		FBV_ID_NONE,//    ID_MULTICOMP1,
		FBV_ID_NONE,//    ID_MULTICOMP2,
		FBV_ID_NONE,//    ID_QUADCHORUS1,
		FBV_ID_NONE,//    ID_QUADCHORUS2,
		FBV_ID_NONE,//    ID_RESONATOR1,
		FBV_ID_NONE,//    ID_RESONATOR2,
		FBV_ID_NONE,//    ID_GRAPHEQ3,
		FBV_ID_NONE,//    ID_GRAPHEQ4,
		FBV_ID_NONE,//    ID_PARAEQ3,
		FBV_ID_NONE,//    ID_PARAEQ4,
		FBV_ID_NONE,//    ID_FILTER3,
		FBV_ID_NONE,//    ID_FILTER4,
		FBV_ID_NONE,//    ID_VOLUME2,
		FBV_ID_NONE,//    ID_VOLUME3,
		FBV_ID_NONE,//    ID_VOLUME4,
};

void do_init_info(void) {

	FBV_ctrls[FBV_ID_TAP_i].fbv_id = FBV_ID_TAP;
	//FBV_ctrls[FBV_ID_TAP_i].type = FBV_ID_TYPE_TEMPO;
	FBV_ctrls[FBV_ID_TAP_i].type = FBV_ID_TYPE_TEMPO_TUNER;
	//FBV_ctrls[FBV_ID_TAP_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_TAP_i].cc = 14;
	FBV_ctrls[FBV_ID_TAP_i].status = 107;


	FBV_ctrls[FBV_ID_DELAY_i].fbv_id = FBV_ID_DELAY;
	FBV_ctrls[FBV_ID_DELAY_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_DELAY_i].cc = 28;

	FBV_ctrls[FBV_ID_MODULATION_i].fbv_id = FBV_ID_MODULATION;
	FBV_ctrls[FBV_ID_MODULATION_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_MODULATION_i].cc = 50;

	FBV_ctrls[FBV_ID_PITCH_i].fbv_id = FBV_ID_PITCH;
	FBV_ctrls[FBV_ID_PITCH_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_PITCH_i].cc = 113;

	FBV_ctrls[FBV_ID_REVERB_i].fbv_id = FBV_ID_REVERB;
	FBV_ctrls[FBV_ID_REVERB_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_REVERB_i].cc = 36;

	FBV_ctrls[FBV_ID_AMP2_i].fbv_id = FBV_ID_AMP2;
	FBV_ctrls[FBV_ID_AMP2_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_AMP2_i].cc = 112;

	FBV_ctrls[FBV_ID_AMP1_i].fbv_id = FBV_ID_AMP1;
	FBV_ctrls[FBV_ID_AMP1_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_AMP1_i].cc = 111;

	FBV_ctrls[FBV_ID_CHAN_FAV_i].fbv_id = FBV_ID_CHAN_FAV;
	FBV_ctrls[FBV_ID_CHAN_FAV_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_CHAN_FAV_i].cc = 52;

	FBV_ctrls[FBV_ID_CHAN_D_i].fbv_id = FBV_ID_CHAN_D;
	FBV_ctrls[FBV_ID_CHAN_D_i].type = FBV_ID_TYPE_PRESET;
	FBV_ctrls[FBV_ID_CHAN_D_i].cc = 3;

	FBV_ctrls[FBV_ID_CHAN_C_i].fbv_id = FBV_ID_CHAN_C;
	FBV_ctrls[FBV_ID_CHAN_C_i].type = FBV_ID_TYPE_PRESET;
	FBV_ctrls[FBV_ID_CHAN_C_i].cc = 2;

	FBV_ctrls[FBV_ID_CHAN_B_i].fbv_id = FBV_ID_CHAN_B;
	FBV_ctrls[FBV_ID_CHAN_B_i].type = FBV_ID_TYPE_PRESET;
	FBV_ctrls[FBV_ID_CHAN_B_i].cc = 1;

	FBV_ctrls[FBV_ID_CHAN_A_i].fbv_id = FBV_ID_CHAN_A;
	FBV_ctrls[FBV_ID_CHAN_A_i].type = FBV_ID_TYPE_PRESET;
	FBV_ctrls[FBV_ID_CHAN_A_i].cc = 0;

	FBV_ctrls[FBV_ID_BANK_UP_i].fbv_id = FBV_ID_BANK_UP;
	FBV_ctrls[FBV_ID_BANK_UP_i].type = FBV_ID_TYPE_BANK;
	FBV_ctrls[FBV_ID_BANK_UP_i].cc = 1;

	FBV_ctrls[FBV_ID_BANK_DOWN_i].fbv_id = FBV_ID_BANK_DOWN;
	FBV_ctrls[FBV_ID_BANK_DOWN_i].type = FBV_ID_TYPE_BANK;
	FBV_ctrls[FBV_ID_BANK_DOWN_i].cc = 0;

	FBV_ctrls[FBV_ID_STOMP3_i].fbv_id = FBV_ID_STOMP3;
	FBV_ctrls[FBV_ID_STOMP3_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_STOMP3_i].cc = 110;

	FBV_ctrls[FBV_ID_STOMP2_i].fbv_id = FBV_ID_STOMP2;
	FBV_ctrls[FBV_ID_STOMP2_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_STOMP2_i].cc = 109;

	FBV_ctrls[FBV_ID_STOMP1_i].fbv_id = FBV_ID_STOMP1;
	FBV_ctrls[FBV_ID_STOMP1_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_STOMP1_i].cc = 25;

	FBV_ctrls[FBV_ID_FX_LOOP_i].fbv_id = FBV_ID_FX_LOOP;
	FBV_ctrls[FBV_ID_FX_LOOP_i].type = FBV_ID_TYPE_BTN_LED;
	FBV_ctrls[FBV_ID_FX_LOOP_i].cc = 107;

	FBV_ctrls[FBV_ID_FOOT_CTRL_V_BTN_i].fbv_id = FBV_ID_FOOT_CTRL_V_BTN;
	FBV_ctrls[FBV_ID_FOOT_CTRL_V_BTN_i].type = FBV_ID_TYPE_FOOT_CTRL;
	FBV_ctrls[FBV_ID_FOOT_CTRL_V_BTN_i].cc = FBV_ID_FOOT_CTRL_V_VAL_i;

	FBV_ctrls[FBV_ID_FOOT_CTRL_W_BTN_i].fbv_id = FBV_ID_FOOT_CTRL_W_BTN;
	FBV_ctrls[FBV_ID_FOOT_CTRL_W_BTN_i].type = FBV_ID_TYPE_FOOT_CTRL;
	FBV_ctrls[FBV_ID_FOOT_CTRL_W_BTN_i].cc = FBV_ID_FOOT_CTRL_W_VAL_i;

	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_V_VAL_i].fbv_id_foot = FBV_ID_FOOT_CTRL_V_VAL;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_V_VAL_i].fbv_id_btn= FBV_ID_FOOT_CTRL_V_BTN;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_V_VAL_i].fbv_id_led1 = FBV_ID_FOOT_CTRL_P2_LED;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_V_VAL_i].fbv_id_led2 = FBV_ID_FOOT_CTRL_V_LED;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_V_VAL_i].cc = 105;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_V_VAL_i].cc_value1 = 125;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_V_VAL_i].cc_value2 = 7;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_V_VAL_i].status = FBV_ID_OFF;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_V_VAL_i].len = 0;


	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].fbv_id_foot = FBV_ID_FOOT_CTRL_W_VAL;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].fbv_id_btn= FBV_ID_FOOT_CTRL_W_BTN;

	//FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].fbv_id_led1 = FBV_ID_FOOT_CTRL_P1_LED;
	//FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].fbv_id_led2 = FBV_ID_FOOT_CTRL_W_LED;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].fbv_id_led1 = FBV_ID_FOOT_CTRL_W_LED;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].fbv_id_led2 = FBV_ID_FOOT_CTRL_P1_LED;

	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].cc = 43;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].cc_value1 = 126;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].cc_value2 = 2;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].status = FBV_ID_OFF;
	FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i].len = 0;

}


typedef struct {
	u8 status;
	u8 led_count;
	u16 btn_count;
} FBV_tempo_tuner_info_struct;

FBV_tempo_tuner_info_struct FBV_tempo_tuner_info = {0};

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void APP_Periodic_100uS(void);
static void TASK_FBV_Check(void *pvParameters);
static s32 AxeFX_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
static void AxeFX_SYSEX_Handle_Package(void);

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{

#if DEBUG_VERBOSE_LEVEL >= 1
  // print welcome message on MIOS terminal
  DEBUG_MSG("\n");
  DEBUG_MSG("====================\n");
  DEBUG_MSG("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  DEBUG_MSG("====================\n");
  DEBUG_MSG("\n");
#endif

  // clear mS counter
  ms_counter = 0;

  // init MIDImon
  //MIDIMON_Init(0);

  // initialize status LED
  MIOS32_BOARD_LED_Init(0x1);
  MIOS32_BOARD_LED_Set(1, 0);

  FBV_UART_Init(0);

  do_init_info();

  midi_channel = 0;
  midi_bank_size = 4;
  midi_bank = 0;

  FBV_UART_TxBufferSendChannelCommand(FBV_CHANNEL_USER,'0' + (midi_bank/10),'0' + (midi_bank%10)); //ascii code for numbers
  FBV_UART_TxBufferSendLedCommand(FBV_ID_CHAN_A, FBV_LED_ON);

  FBV_UART_TxBufferSendDisplay("VLoTech FBV ctrl",16);

  MIOS32_MIDI_SendProgramChange(USB1, RACK_MIDI_CHN, midi_channel);
  MIOS32_MIDI_SendProgramChange(UART1, RACK_MIDI_CHN, midi_channel);

  MIOS32_MIDI_SysExCallback_Init(AxeFX_SYSEX_Parser);
  MIOS32_MIDI_SendSysEx(AXEFX_PORT,axefx_request_version_sysex,axefx_request_version_length);
  MIOS32_MIDI_SendSysEx(AXEFX_PORT,axefx_request_blocks_sysex,axefx_request_blocks_length);
  MIOS32_MIDI_SendSysEx(AXEFX_PORT,axefx_request_patch_name_sysex,axefx_request_patch_name_length);

  // install timer function which is called each 100 uS
  MIOS32_TIMER_Init(0, 100, APP_Periodic_100uS, MIOS32_IRQ_PRIO_MID);

  // start BLM check task
  xTaskCreate(TASK_FBV_Check, (signed portCHAR *)"FBV_Check", MIOS32_MINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_FBV_CHECK, NULL);

}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
//  // init LCD
//  MIOS32_LCD_Clear();
//  MIOS32_LCD_CursorSet(0, 0);
//  MIOS32_LCD_PrintString("see README.txt   ");
//  MIOS32_LCD_CursorSet(0, 1);
//  MIOS32_LCD_PrintString("for details     ");

  // endless loop
  while( 1 ) {
    // do nothing
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // forward packages USBx->UARTx and UARTx->USBx
  switch( port ) {
    case USB0:
      MIOS32_MIDI_SendPackage(UART0, midi_package);
      break;

    case USB1:
      MIOS32_MIDI_SendPackage(UART1, midi_package);
      break;

    case UART0:
      MIOS32_MIDI_SendPackage(USB0, midi_package);
      break;

    case UART1:
      MIOS32_MIDI_SendPackage(USB1, midi_package);
      break;
  }
  // forward to MIDI Monitor
  // SysEx messages have to be filtered for USB0 and UART0 to avoid data corruption
  // (the SysEx stream would interfere with monitor messages)
//  u8 filter_sysex_message = (port == USB0) || (port == UART0);
//  MIDIMON_Receive(port, midi_package, ms_counter, filter_sysex_message);
}


void APP_SRIO_ServicePrepare(void) { }
void APP_SRIO_ServiceFinish(void) { }
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value) { }
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer) { }
void APP_AIN_NotifyChange(u32 pin, u32 pin_value) { }

/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for SysEx messages
/////////////////////////////////////////////////////////////////////////////
s32 AxeFX_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  if( port != AXEFX_PORT )
    return 0; // forward package to APP_MIDI_NotifyPackage()

  // branch depending on state
  if( sysex_state == SYSEX_HEADER ) {
    if( midi_in != sysex_header[sysex_count]) {
      // incoming byte doesn't match
    	sysex_count = 0;
    	sysex_cmd = 0;
    	DEBUG_MSG("AxeFX other byte found than header\n");
    } else {
      sysex_buffer[sysex_count++] = midi_in;
      if( sysex_count == sizeof(sysex_header) ) {
		// complete header received, waiting for data
		sysex_state = SYSEX_CMD_STATE_BEGIN;
		sysex_count = 0;
		sysex_axefx_type = sysex_buffer[4];

		DEBUG_MSG("AxeFX %s header found\n", sysex_axefx_type==0?"Standard":"Ultra");
      }
    }
  } else {
	    // check for end of SysEx message or invalid status byte
	    if( midi_in >= 0x80 ) {
	      if( midi_in == 0xf7 && sysex_state == SYSEX_CMD_STATE_CONT ) {
	    	  sysex_state = SYSEX_CMD_STATE_END;
	    	  DEBUG_MSG("AxeFX data done\n");
	      } else {
	    	sysex_state = SYSEX_HEADER;
	      	sysex_count = 0;
	      	sysex_cmd = 0;
	      	DEBUG_MSG("AxeFX data broken\n");
	      }
	    } else {
	      // check if command byte has been received
	  	  if (sysex_state == SYSEX_CMD_STATE_BEGIN) {
	  		  sysex_cmd = midi_in;
	  		  sysex_state = SYSEX_CMD_STATE_CONT;
	  		  sysex_count = 0;
	  		  DEBUG_MSG("AxeFX CMD received %02X\n", sysex_cmd);
	  	  } else {
	  		  if(sysex_count<SYSEX_MAX_LEN)
	  		    sysex_buffer[sysex_count++] = midi_in;
	  		  else
	  			sysex_count++;  // ignore additional bytes...
	  	  }
	    }
	    if (sysex_state == SYSEX_CMD_STATE_END) {
	    	//process the package !!
	    	AxeFX_SYSEX_Handle_Package();
	    	sysex_state = SYSEX_HEADER;
	    	sysex_count = 0;
	    	sysex_cmd = 0;
	    }
  }

  return 0; // don't forward package to APP_MIDI_NotifyPackage()
}

void AxeFX_SYSEX_Handle_Package(void) {

	if( sysex_state != SYSEX_CMD_STATE_END)
		return;

	int i;

	DEBUG_MSG("AxeFX handling package with CMD: %02X\n", sysex_cmd);
	DEBUG_MSG("AxeFX handling package with LEN: %02X\n", sysex_count);

	switch(sysex_cmd) {
		case 0x0d:
			// tuner info
			{
				//u8 string = '1' + sysex_buffer[1]; // string number
				u8 needle[17] = "                ";

				if(sysex_buffer[2] >=0x10 || sysex_buffer[2] <0x70) {

					switch(sysex_buffer[0]) {
						case 0:FBV_UART_TxBufferSendTuner('A',0);break;
						case 1:FBV_UART_TxBufferSendTuner('B',1);break;
						case 2:FBV_UART_TxBufferSendTuner('B',0);break;
						case 3:FBV_UART_TxBufferSendTuner('C',0);break;
						case 4:FBV_UART_TxBufferSendTuner('D',1);break;
						case 5:FBV_UART_TxBufferSendTuner('D',0);break;
						case 6:FBV_UART_TxBufferSendTuner('E',1);break;
						case 7:FBV_UART_TxBufferSendTuner('E',0);break;
						case 8:FBV_UART_TxBufferSendTuner('F',0);break;
						case 9:FBV_UART_TxBufferSendTuner('G',1);break;
						case 10:FBV_UART_TxBufferSendTuner('G',0);break;
						case 11:FBV_UART_TxBufferSendTuner('A',1);break;
						default:FBV_UART_TxBufferSendTuner(' ',0);break;
					}

					// sysex_buffer[2] == pos from 0x10 to 0x6F
					if(sysex_buffer[2] < 0x1A) {
						needle[0] = ')';
					}
					if(sysex_buffer[2] < 0x20) {
						needle[1] = ')';
					}
					if(sysex_buffer[2] < 0x26) {
						needle[2] = ')';
					}
					if(sysex_buffer[2] < 0x2C) {
						needle[3] = ')';
					}
					if(sysex_buffer[2] < 0x32) {
						needle[4] = ')';
					}
					if(sysex_buffer[2] < 0x38) {
						needle[5] = ')';
					}
					if(sysex_buffer[2] < 0x3E) {
						needle[6] = ')';
					}
					if(sysex_buffer[2] < 0x42) {
						needle[7] = ')';
					}
					if(sysex_buffer[2] > 0x3E) {
						needle[8] = '(';
					}
					if(sysex_buffer[2] > 0x41) {
						needle[9] = '(';
					}
					if(sysex_buffer[2] > 0x47) {
						needle[10] = '(';
					}
					if(sysex_buffer[2] > 0x4D) {
						needle[11] = '(';
					}
					if(sysex_buffer[2] > 0x53) {
						needle[12] = '(';
					}
					if(sysex_buffer[2] > 0x59) {
						needle[13] = '(';
					}
					if(sysex_buffer[2] > 0x5F) {
						needle[14] = '(';
					}
					if(sysex_buffer[2] > 0x65) {
						needle[15] = '(';
					}
					if(sysex_buffer[2] > 0x3E && sysex_buffer[2] < 0x42) {
						needle[6] = '-';
						needle[7] = '*';
						needle[8] = '*';
						needle[9] = '-';
					}


				FBV_UART_TxBufferSendDisplay(needle,16);

				} else {
					FBV_UART_TxBufferSendTuner(' ',0);
					FBV_UART_TxBufferSendDisplay("                ",16);
				}

			}
			break;

		case 0x0e:
			// block status result
			DEBUG_MSG("AxeFX block status result\n");
			for(i = 0; i<FBV_ID_MAX_INDEX;i++)
				FBV_ctrls[i].len = 0;
			for(i = 0; i<FBV_ID_MAX_FOOT_INDEX;i++)
				FBV_ctrls_cont[i].len = 0;


			for(i = 0; i<sysex_count && sysex_count<SYSEX_MAX_LEN; i+=5) {
				u16 fx_id = sysex_buffer[i] + (sysex_buffer[i+1]*0x10);
				u16 fx_cc = sysex_buffer[i+2] + (sysex_buffer[i+3]*0x10);
				u8 status = sysex_buffer[i+4]; //status
				int index = block_to_id[fx_id-100];
				if(index < FBV_ID_MAX_INDEX ) {
					fbv_ctrl_t *ctrl = &(FBV_ctrls[index]);
					if(ctrl->type == FBV_ID_TYPE_BTN_LED) {
						ctrl->blocks[ctrl->len].cc = fx_cc;
						ctrl->blocks[ctrl->len].id = fx_id;
						ctrl->blocks[ctrl->len].status = status;

						if(ctrl->len == 0)
							ctrl->status = status;

						FBV_UART_TxBufferSendLedCommand(ctrl->fbv_id, status);

						ctrl->len++;
					} else if(ctrl->type == FBV_ID_TYPE_FOOT_CTRL) {
						fbv_footctrl_t *foot = &(FBV_ctrls_cont[ctrl->cc]);

						foot->blocks[foot->len].cc = fx_cc;
						foot->blocks[foot->len].id = fx_id;
						foot->blocks[foot->len].status = status;

						if(foot->len == 0) {
							foot->status = status;
							if(status== FBV_ID_OFF) {
								FBV_UART_TxBufferSendLedCommand(foot->fbv_id_led1, FBV_ID_ON);
								FBV_UART_TxBufferSendLedCommand(foot->fbv_id_led2, FBV_ID_OFF);
							} else {
								FBV_UART_TxBufferSendLedCommand(foot->fbv_id_led1, FBV_ID_OFF);
								FBV_UART_TxBufferSendLedCommand(foot->fbv_id_led2, FBV_ID_ON);
							}
						}


						foot->len++;
					}
					DEBUG_MSG("------  for FBV: %02X\n", ctrl->fbv_id);
				} else {
					DEBUG_MSG("------  NOT USED !!\n");
				}
				DEBUG_MSG("AxeFX fx-ID: %02X\n", fx_id);
				DEBUG_MSG("AxeFX fx-CC: %02X\n", fx_cc);
				DEBUG_MSG("AxeFX fx-status: %02X\n", status);
			}
			break;
		case 0x0f:
			// patch name result
			DEBUG_MSG("AxeFX patch name status result\n");
			DEBUG_MSG(sysex_buffer);
			FBV_UART_TxBufferSendDisplay(sysex_buffer,16);
			break;
		case 0x08:
			// version status result
			DEBUG_MSG("AxeFX version status result\n");
			u8 axefx_major = sysex_buffer[i];
			u8 axefx_minor = sysex_buffer[i+1];
			DEBUG_MSG("AxeFX major: %02i\n", axefx_major);
			DEBUG_MSG("AxeFX minor: %02i\n", axefx_minor);

			if(sysex_axefx_type==0x0) {
				u8 buf[17] = "Axe-FX Std v0.00";
				if(axefx_major>9)
					buf[11] = '0'+ axefx_major/10;
				buf[12] = '0'+ axefx_major%10;
				buf[14] = '0'+ axefx_minor/10;
				buf[15] = '0'+ axefx_minor%10;
				//FBV_UART_TxBufferSendDisplay(buf,16);
			} else if(sysex_axefx_type==0x1) {
				u8 buf[17] = "Axe-FX Ult v0.00";
				if(axefx_major>9)
					buf[11] = '0'+ axefx_major/10;
				buf[12] = '0'+ axefx_major%10;
				buf[14] = '0'+ axefx_minor/10;
				buf[15] = '0'+ axefx_minor%10;
				//FBV_UART_TxBufferSendDisplay(buf,16);
			}

			break;
		case 0x10:
			// tempo tap info
			FBV_tempo_tuner_info.led_count = 0x01;
			for(i = 0; i<FBV_ID_MAX_INDEX;i++) {
				if( FBV_ctrls[i].type == FBV_ID_TYPE_TEMPO_TUNER || FBV_ctrls[i].type == FBV_ID_TYPE_TEMPO ) {
					FBV_UART_TxBufferSendLedCommand(FBV_ctrls[i].fbv_id, FBV_LED_ON);
					break;
				}
			}
			break;

		default:
			// (yet) unknown command
			break;

	}
}


/////////////////////////////////////////////////////////////////////////////
// This timer function is periodically called each 100 uS
/////////////////////////////////////////////////////////////////////////////
static void APP_Periodic_100uS(void)
{

  static u32 flash_cnt = 0;
//  mios32_fbv_message_t msg = {0};
  int i,j;
  if((flash_cnt) == 0) {
	  for(i = 0; i<FBV_ID_MAX_INDEX;i++) {
		  if( FBV_ctrls[i].type == FBV_ID_TYPE_BTN_LED && FBV_ctrls[i].len > 0 ) {
			  for(j=0; j< FBV_ctrls[i].len; j++ ) {
				  if(FBV_ctrls[i].blocks[j].status == 0) {
					  FBV_UART_TxBufferSendLedCommand(FBV_ctrls[i].fbv_id, FBV_LED_ON);
					  break;
				  }
			  }

		  }
	  }

	  if((midi_channel)/midi_bank_size != midi_bank ) {
		  for(i = 0;i<midi_bank_size;i++) {
			  if(midi_channel%midi_bank_size == i)
				  FBV_UART_TxBufferSendLedCommand(bank_ids[i], FBV_LED_ON);
			  else
				  FBV_UART_TxBufferSendLedCommand(bank_ids[i], FBV_LED_OFF);
		  }
	  }
  } else if((flash_cnt) == 0x400) {
	  for(i = 0; i<FBV_ID_MAX_INDEX;i++) {
		  if( FBV_ctrls[i].type == FBV_ID_TYPE_BTN_LED && FBV_ctrls[i].len > 0 ) {
			  for(j=0; j< FBV_ctrls[i].len; j++ ) {
				  if(FBV_ctrls[i].blocks[j].status == 0) {
					  FBV_UART_TxBufferSendLedCommand(FBV_ctrls[i].fbv_id, FBV_LED_OFF);
					  break;
				  }
			  }

		  }
	  }


  } else if((flash_cnt) == 0x1000) {
	  if((midi_channel)/midi_bank_size != midi_bank ) {
		  for(i = 0;i<midi_bank_size;i++)
			  FBV_UART_TxBufferSendLedCommand(bank_ids[i], FBV_LED_OFF);
	  }
  }

  if((flash_cnt&0x0F) == 0) {
	  if(FBV_tempo_tuner_info.status == FBV_BUTTON_PRESSED) {
		  if(FBV_tempo_tuner_info.btn_count >= 1875)  {// 3 sec.
			  for(i = 0; i<FBV_ID_MAX_INDEX;i++) {
				  if( FBV_ctrls[i].type == FBV_ID_TYPE_TEMPO_TUNER  ) {
					  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, FBV_ctrls[i].status, 127); // status == tuner-cc == non-latching
					  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, FBV_ctrls[i].status, 127); // status == tuner-cc == non-latching
					  break;
				  }
			  }
			  FBV_tempo_tuner_info.status = FBV_BUTTON_RELEASED; // prevent endless loop
			  FBV_tempo_tuner_info.btn_count = 0;
			  FBV_UART_TxBufferSendChannelCommand('-','-','-');
		  } else {
			 FBV_tempo_tuner_info.btn_count++;
		  }
	  }

	  if(FBV_tempo_tuner_info.led_count != 0x00) {
		  if(FBV_tempo_tuner_info.led_count == 0x0F) {
			  for(i = 0; i<FBV_ID_MAX_INDEX;i++) {
				  if( FBV_ctrls[i].type == FBV_ID_TYPE_TEMPO_TUNER || FBV_ctrls[i].type == FBV_ID_TYPE_TEMPO ) {
					FBV_UART_TxBufferSendLedCommand(FBV_ctrls[i].fbv_id, FBV_LED_OFF);
					break;
				  }
			  }

		  }
		  FBV_tempo_tuner_info.led_count++;
	  }


  }

  flash_cnt++;
  if(flash_cnt >=0x2000)
	  flash_cnt = 0;

}


static void TASK_FBV_Check(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    mios32_fbv_message_t msg = {0};

    while(FBV_UART_RxBufferReceiveMessage(&msg) == 0) {
  //	  DEBUG_MSG("FBV Message:\n");
  //	  DEBUG_MSG("header: %08X\n", msg.header);
  //	  DEBUG_MSG("size:   %08X\n", msg.size);
  //	  DEBUG_MSG("cmd:    %08X\n", msg.cmd);
  //	  if(msg.pos == 1)
  //		  DEBUG_MSG("data:    %08X\n", msg.data[0]);
  //	  else if(msg.pos > 1) {
  //		  DEBUG_MSG("data:\n");
  //		  MIOS32_MIDI_SendDebugHexDump(msg.data,msg.pos);
  //	  }
  //	  DEBUG_MSG("------------\n");

  	  if(msg.cmd == 0x90) { //INIT ?!?

  		  int i;

  		  FBV_UART_TxBufferSendInit();

  		  FBV_UART_TxBufferSendChannelCommand(FBV_CHANNEL_USER,'0' + (midi_bank/10),'0' + (midi_bank%10)); //ascii code for numbers
  		  FBV_UART_TxBufferSendLedCommand(FBV_ID_CHAN_A, FBV_LED_ON);

  		  for(i = 0; i < midi_bank_size; i++)
  		    FBV_UART_TxBufferSendLedCommand(bank_ids[i], FBV_LED_OFF);
  		  FBV_UART_TxBufferSendLedCommand(bank_ids[(midi_channel%midi_bank_size)], FBV_LED_ON);

  		  MIOS32_MIDI_SendSysEx(AXEFX_PORT,axefx_request_version_sysex,axefx_request_version_length);
  		  MIOS32_MIDI_SendSysEx(AXEFX_PORT,axefx_request_blocks_sysex,axefx_request_blocks_length);
  		  MIOS32_MIDI_SendSysEx(AXEFX_PORT,axefx_request_patch_name_sysex,axefx_request_patch_name_length);

  	  }

  	  else if(msg.cmd == 0x81 && msg.data[1] == FBV_BUTTON_PRESSED ) { //BUTTON -> PRESSED
  		  int i,j,k;
  		  u8 handled = 0;
  		  //FBV_UART_TxBufferSendLedCommand(msg.data[0], msg.data[1]);

  		  DEBUG_MSG("FBV button pressed:\n");
  		  DEBUG_MSG("FBV button  %08X\n",msg.data[0]);
  		  for(i = 0; i<FBV_ID_MAX_INDEX;i++) {
  			DEBUG_MSG("FBV checking    %08X\n",FBV_ctrls[i].fbv_id);
  			  if( FBV_ctrls[i].fbv_id == msg.data[0] ) {
  				  fbv_ctrl_t *ctrl = &(FBV_ctrls[i]);

  				  DEBUG_MSG("FBV block found for %08X\n",ctrl->fbv_id);
  				  DEBUG_MSG("type is %08X\n",ctrl->type);


  				  if(ctrl->type == FBV_ID_TYPE_BTN_LED) {
			          //if(ctrl->len == 0) {
					    if(ctrl->status == FBV_ID_OFF) {
						  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, ctrl->cc, 127);
						  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, ctrl->cc, 127);
						  FBV_UART_TxBufferSendLedCommand(ctrl->fbv_id, FBV_LED_ON);
						  ctrl->status = FBV_ID_ON;
						  DEBUG_MSG("to ON    %i\n",ctrl->cc);
					    } else {
						  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, ctrl->cc, 0);
						  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, ctrl->cc, 0);
						  FBV_UART_TxBufferSendLedCommand(ctrl->fbv_id, FBV_LED_OFF);
						  ctrl->status = FBV_ID_OFF;
						  DEBUG_MSG("to OFF    %i\n",ctrl->cc);
					    }
			          //} else {
					    for(j=0; j< ctrl->len; j++ ) {
						  if(ctrl->blocks[j].status == FBV_ID_OFF) {
							  if(ctrl->blocks[j].cc != 128) {
								  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, ctrl->blocks[j].cc, 127);
								  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, ctrl->blocks[j].cc, 127);
							  } else {
								  // TODO: Add SysEx control
							  }
							  //FBV_UART_TxBufferSendLedCommand(ctrl->fbv_id, FBV_LED_ON);
							  ctrl->blocks[j].status = FBV_ID_ON;
							  //ctrl->status = FBV_ID_ON;
							  DEBUG_MSG("to AxeFX block ON   %i\n",ctrl->cc);
						  } else {
							  if(ctrl->blocks[j].cc != 128) {
								  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, ctrl->blocks[j].cc, 0);
								  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, ctrl->blocks[j].cc, 0);
							  } else {
								  // TODO: Add SysEx control
							  }
							  //FBV_UART_TxBufferSendLedCommand(ctrl->fbv_id, FBV_LED_OFF);
							  DEBUG_MSG("to AxeFX block OFF   %i\n",ctrl->cc);
							  ctrl->blocks[j].status = FBV_ID_OFF;
							  //ctrl->status = FBV_ID_OFF;
						  }
					    }
			          //}
				  } else if(ctrl->type == FBV_ID_TYPE_BANK) { // TODO: handle banks above preset 128
					  if(ctrl->cc == 0) {
						  //down
						  if(midi_bank==0) midi_bank = 20; else midi_bank -= 1;
					  } else {
						  if(midi_bank==20) midi_bank = 0; else midi_bank += 1;
					  }
					  FBV_UART_TxBufferSendChannelCommand(FBV_CHANNEL_USER,'0' + (midi_bank/10),'0' + (midi_bank%10));
				  } else if(ctrl->type == FBV_ID_TYPE_PRESET) { // TODO: handle banks above preset 128
					  midi_channel = midi_bank*midi_bank_size + ctrl->cc;
					  for(k = 0; k < midi_bank_size; k++)
						  FBV_UART_TxBufferSendLedCommand(bank_ids[k], FBV_LED_OFF);
					  FBV_UART_TxBufferSendLedCommand(ctrl->fbv_id, FBV_LED_ON);
					  for(k = 0; k < FBV_ID_MAX_INDEX; k++) {
						  if(FBV_ctrls[k].type == FBV_ID_TYPE_BTN_LED) {
							  FBV_UART_TxBufferSendLedCommand(FBV_ctrls[k].fbv_id, FBV_LED_OFF);
							  FBV_ctrls[k].status = FBV_ID_OFF;
						  }
					  }
					  MIOS32_MIDI_SendProgramChange(USB1, RACK_MIDI_CHN, midi_channel);
					  MIOS32_MIDI_SendProgramChange(UART1, RACK_MIDI_CHN, midi_channel);
					  MIOS32_MIDI_SendSysEx(AXEFX_PORT,axefx_request_blocks_sysex,axefx_request_blocks_length);
					  MIOS32_MIDI_SendSysEx(AXEFX_PORT,axefx_request_patch_name_sysex,axefx_request_patch_name_length);
				  } else if (ctrl->type == FBV_ID_TYPE_TEMPO || ctrl->type == FBV_ID_TYPE_TEMPO_TUNER ) {
					  // send tap tempo CC
					  FBV_tempo_tuner_info.status = FBV_BUTTON_PRESSED;
					  FBV_tempo_tuner_info.btn_count = 0;

					  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, ctrl->cc, 127);
					  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, ctrl->cc, 127);
				  } else if(ctrl->type == FBV_ID_TYPE_FOOT_CTRL) {
					  fbv_footctrl_t *foot = &FBV_ctrls_cont[ctrl->cc];
			          //if(ctrl->len == 0) {
					    if(foot->status == FBV_ID_OFF) {
					      MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, foot->cc, 127);
						  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, foot->cc, 127);
						  FBV_UART_TxBufferSendLedCommand(foot->fbv_id_led1, FBV_LED_OFF);
						  FBV_UART_TxBufferSendLedCommand(foot->fbv_id_led2, FBV_LED_ON);
						  foot->status = FBV_ID_ON;
						  DEBUG_MSG("to ON    %i\n",foot->cc);
					    } else {
						  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, foot->cc, 0);
						  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, foot->cc, 0);
						  FBV_UART_TxBufferSendLedCommand(foot->fbv_id_led1, FBV_LED_ON);
						  FBV_UART_TxBufferSendLedCommand(foot->fbv_id_led2, FBV_LED_OFF);
						  foot->status = FBV_ID_OFF;
						  DEBUG_MSG("to OFF    %i\n",ctrl->cc);
					    }
			          //} else {
					    for(j=0; j< foot->len; j++ ) {
						  if(foot->blocks[j].status == FBV_ID_OFF) {
							  if(foot->blocks[j].cc != 128) {
								  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, foot->blocks[j].cc, 127);
								  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, foot->blocks[j].cc, 127);
							  } else {
								  // TODO: Add SysEx control
							  }
							  //FBV_UART_TxBufferSendLedCommand(ctrl->fbv_id, FBV_LED_ON);
							  foot->blocks[j].status = FBV_ID_ON;
							  //ctrl->status = FBV_ID_ON;
							  DEBUG_MSG("to AxeFX block ON   %i\n",foot->cc);
						  } else {
							  if(foot->blocks[j].cc != 128) {
								  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, foot->blocks[j].cc, 0);
								  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, foot->blocks[j].cc, 0);
							  } else {
								  // TODO: Add SysEx control
							  }
							  //FBV_UART_TxBufferSendLedCommand(ctrl->fbv_id, FBV_LED_OFF);
							  DEBUG_MSG("to AxeFX block OFF   %i\n",foot->cc);
							  foot->blocks[j].status = FBV_ID_OFF;
							  //ctrl->status = FBV_ID_OFF;
						  }
					    }
			          //}
				  }
  				  break;
  			  }
  		  }
  	  }

  	  else if(msg.cmd == 0x81 && msg.data[1] == FBV_BUTTON_RELEASED ) { //BUTTON -> RELEASED
  		  int i;
  		  //FBV_UART_TxBufferSendLedCommand(msg.data[0], msg.data[1]);

  		  DEBUG_MSG("FBV button released:\n");
  		  DEBUG_MSG("FBV button  %08X\n",msg.data[0]);
  		  for(i = 0; i<FBV_ID_MAX_INDEX;i++) {
  			DEBUG_MSG("FBV checking    %08X\n",FBV_ctrls[i].fbv_id);
  			  if( FBV_ctrls[i].fbv_id == msg.data[0] ) {
  				if (FBV_ctrls[i].type == FBV_ID_TYPE_TEMPO || FBV_ctrls[i].type == FBV_ID_TYPE_TEMPO_TUNER ) {
					  // send tap tempo CC (release because tap tempo is non-latching)

					  //MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, FBV_ctrls[i].cc, 0);
					  //MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, FBV_ctrls[i].cc, 0);

					  if (FBV_tempo_tuner_info.status == FBV_BUTTON_RELEASED) {
						  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, FBV_ctrls[i].status, 0); // status == tuner-cc == non-latching
						  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, FBV_ctrls[i].status, 0); // status == tuner-cc == non-latching

						  FBV_UART_TxBufferSendChannelCommand(FBV_CHANNEL_USER,'0' + (midi_bank/10),'0' + (midi_bank%10));

						  MIOS32_MIDI_SendSysEx(AXEFX_PORT,axefx_request_patch_name_sysex,axefx_request_patch_name_length);
					  }

					  FBV_tempo_tuner_info.status = FBV_BUTTON_RELEASED;
					  FBV_tempo_tuner_info.btn_count = 0;
  				}
  				break;
  			  }
  		  }
  	  }

  	  else if(msg.cmd == 0x82) { //PEDAL
		  fbv_footctrl_t *foot = 0;
		  if(msg.data[0] == 0x00) { // WAH
			  foot = &FBV_ctrls_cont[FBV_ID_FOOT_CTRL_W_VAL_i];
		  } else if (msg.data[0] == 0x01) { // VOL
			  foot = &FBV_ctrls_cont[FBV_ID_FOOT_CTRL_V_VAL_i];
		  }
		  if(foot!=0 ) {
			  if(foot->status == FBV_ID_OFF ) {
				  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, foot->cc_value1, msg.data[1]);
				  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, foot->cc_value1, msg.data[1]);
			  } else {
				  MIOS32_MIDI_SendCC(USB1, RACK_MIDI_CHN, foot->cc_value2, msg.data[1]);
				  MIOS32_MIDI_SendCC(UART1, RACK_MIDI_CHN, foot->cc_value2, msg.data[1]);
			  }

		  }

	  }

    }

  }
}
