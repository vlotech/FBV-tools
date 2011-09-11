/*
 * axefx_info.c
 *
 *  Created on: 20-mrt-2010
 *      Author: vincent
 */

#include <mios32.h>
#include "axefx_info.h"


const u8 axefx_request_blocks_sysex[] = {0xF0, 0x00, 0x00, 0x7D, 0x00, 0x0E, 0xF7};
const u32 axefx_request_blocks_length = 7;

const u8 axefx_request_version_sysex[] = {0xF0, 0x00, 0x00, 0x7D, 0x00, 0x08, 0x00, 0x00, 0xF7};
const u32 axefx_request_version_length = 9;

const u8 axefx_request_patch_name_sysex[] = {0xF0, 0x00, 0x00, 0x7D, 0x00, 0x0F, 0xF7};
const u32 axefx_request_patch_name_length = 7;
