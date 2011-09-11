# defines additional rules for adding Line6 FBV support

# enhance include path
C_INCLUDE += -I $(MIOS32_FBV_PATH)/modules/fbv


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_FBV_PATH)/modules/fbv/fbv_uart.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_FBV_PATH)/modules/fbv
