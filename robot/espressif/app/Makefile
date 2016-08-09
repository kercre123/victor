#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of object file images to be generated ()
#   GEN_BINS - list of binaries to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
TARGET = eagle
FLAVOR = release
#FLAVOR = debug

#EXTRA_CCFLAGS += -u

ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS= eagle.app.v6.bin
SPECIAL_MKTARGETS=$(APP_MKTARGETS)
SUBDIRS=    \
	hal    \
	driver  \
  transport \
  clad \
  application \
	factory \
  supervisor \
  crypto
endif # } PDIR

APPDIR = .
LDDIR = ../ld

TARGET_LDFLAGS =		\
	-nostdlib		\
	-Wl,-EL \
	--longcalls \
	--text-section-literals

ifeq ($(FLAVOR),debug)
    TARGET_LDFLAGS += -g -O2
endif

ifeq ($(FLAVOR),release)
    TARGET_LDFLAGS += -g -Os
endif

COMPONENTS_eagle.app.v6 = \
	hal/libuserhal.a  \
	driver/libdriver.a \
	transport/libtransport.a \
	clad/libclad.a \
	application/libapplication.a \
	factory/libfactory.a \
	supervisor/libsupervisor.a \
	crypto/libusercrypto.a

LINKFLAGS_eagle.app.v6 = \
	-L../lib        \
	-nostdlib	\
	-T$(LD_FILE) \
	-Wl,--no-check-sections	\
	-u call_user_start -u Cache_Read_Enable_New	\
	-Wl,-static						\
	-Wl,--start-group					\
	-lc \
    -lgcc					\
	-lhal					\
	$(DEP_LIBS_eagle.app.v6) \
	-lphy	\
	-lpp	\
	-lnet80211	\
	-lcrypto \
	-llwip	\
	-lwpa	\
	-lmain_w	\
	-ljson	\
	-lssl	\
	-lpwm	\
	-lsmartconfig \
	-Wl,--end-group \
  -Xlinker --Map=esp.map

DEPENDS_eagle.app.v6 = \
                $(LD_FILE) \
                $(LDDIR)/eagle.rom.addr.v6.ld

#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#

#UNIVERSAL_TARGET_DEFINES =		\

# Other potential configuration flags include:
#	-DTXRX_TXBUF_DEBUG
#	-DTXRX_RXBUF_DEBUG
#	-DWLAN_CONFIG_CCX
CONFIGURATION_DEFINES =	-DICACHE_FLASH

DEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES) \
  -DTARGET_ESPRESSIF

DDEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)

#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include -I $(ROBOT_INC_DIR) $(INCLUDE)
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile

.PHONY: FORCE
FORCE:
