# $Id: Makefile 662 2009-07-25 22:40:36Z tk $

################################################################################
# following setup taken from environment variables
################################################################################


MIOS32_PATH=~/Workspace/MIOS32_base
MIOS32_BIN_PATH=$MIOS32_PATH/bin

MIOS32_FBV_PATH=~/Workspace/MIOS32_FBV


MIOS32_GCC_PREFIX=arm-none-eabi
MIOS32_FAMILY=STM32F10x
MIOS32_PROCESSOR=STM32F103RE
MIOS32_BOARD=MBHP_CORE_STM32
MIOS32_LCD=clcd

PROCESSOR =	    $(MIOS32_PROCESSOR)
FAMILY    = 	$(MIOS32_FAMILY)
BOARD	  = 	$(MIOS32_BOARD)
LCD       =     $(MIOS32_LCD)


################################################################################
# Source Files, include paths and libraries
################################################################################

THUMB_SOURCE    = app.c \
		  midimon.c \
		  axefx_info.c 

# (following source stubs not relevant for Cortex M3 derivatives)
THUMB_AS_SOURCE =
ARM_SOURCE      =
ARM_AS_SOURCE   =

C_INCLUDE = 	-I .
A_INCLUDE = 	-I .

LIBS = 		


################################################################################
# Remaining variables
################################################################################

LD_FILE   = 	$(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
PROJECT   = 	project

DEBUG     =	-g
OPTIMIZE  =	-Os

CFLAGS =	$(DEBUG) $(OPTIMIZE)


################################################################################
# Include source modules via additional makefiles
################################################################################

# sources of programming model
include $(MIOS32_PATH)/programming_models/traditional/programming_model.mk

# FBX 
include $(MIOS32_FBV_PATH)/modules/fbv/fbv.mk

# common make rules
include $(MIOS32_PATH)/include/makefile/common.mk
