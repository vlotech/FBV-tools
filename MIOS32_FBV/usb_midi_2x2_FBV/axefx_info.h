/*
 * axefx_info.h
 *
 *  Created on: 20-mrt-2010
 *      Author: vincent
 */

#ifndef _AXEFX_INFO_H_
#define _AXEFX_INFO_H_

enum
{
    ID_COMP1 = 100,
    ID_COMP2,
    ID_GRAPHEQ1,
    ID_GRAPHEQ2,
    ID_PARAEQ1,
    ID_PARAEQ2,
    ID_AMP1,
    ID_AMP2,
    ID_CAB1,
    ID_CAB2,
    ID_REVERB1,
    ID_REVERB2,
    ID_DELAY1,
    ID_DELAY2,
    ID_MULTITAP1,
    ID_MULTITAP2,
    ID_CHORUS1,
    ID_CHORUS2,
    ID_FLANGER1,
    ID_FLANGER2,
    ID_ROTARY1,
    ID_ROTARY2,
    ID_PHASER1,
    ID_PHASER2,
    ID_WAH1,
    ID_WAH2,
    ID_FORMANT1,
    ID_VOLUME1,
    ID_TREMOLO1,
    ID_TREMOLO2,
    ID_PITCH1,
    ID_FILTER1,
    ID_FILTER2,
    ID_DRIVE1,
    ID_DRIVE2,
    ID_ENHANCER1,
    ID_LOOP1,
    ID_MIXER1,
    ID_MIXER2,
    ID_NOISEGATE1,
    ID_OUT1,
    ID_CONTROL,
    ID_FBSEND,
    ID_FBRETURN,
    ID_SYNTH1,
    ID_SYNTH2,
    ID_VOCODER1,
    ID_MEGATAP1,
    ID_CROSSOVER1,
    ID_CROSSOVER2,
    ID_GATE1,
    ID_GATE2,
    ID_RINGMOD1,
    ID_PITCH2,
    ID_MULTICOMP1,
    ID_MULTICOMP2,
    ID_QUADCHORUS1,
    ID_QUADCHORUS2,
    ID_RESONATOR1,
    ID_RESONATOR2,
    ID_GRAPHEQ3,
    ID_GRAPHEQ4,
    ID_PARAEQ3,
    ID_PARAEQ4,
    ID_FILTER3,
    ID_FILTER4,
    ID_VOLUME2,
    ID_VOLUME3,
    ID_VOLUME4,
};

extern const u8  axefx_request_blocks_sysex[];
extern const u32 axefx_request_blocks_length;

extern const u8  axefx_request_version_sysex[];
extern const u32 axefx_request_version_length;

extern const u8  axefx_request_patch_name_sysex[];
extern const u32 axefx_request_patch_name_length;

typedef struct {
	u16 id;
	u16 cc;
	u8 status;
} axefx_block_status_struct;


#endif /* _AXEFX_INFO_H_ */
