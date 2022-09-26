#*********************************************************************************************************
#
#                                    中国软件开源组织
#
#                                   嵌入式实时操作系统
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------文件信息--------------------------------------------------------------------------------
#
# 文   件   名: header.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2016 年 08 月 24 日
#
# 描        述: makefile 模板首部
#*********************************************************************************************************

#*********************************************************************************************************
# Check configure
#*********************************************************************************************************
check_defined = \
    $(foreach 1,$1,$(__check_defined))
__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $(value 2), ($(strip $2)))))

$(call check_defined, CONFIG_MK_EXIST, Please configure this project in RealEvo-IDE or create a config.mk file!)
$(call check_defined, SYLIXOS_BASE_PATH, SylixOS base project path)
$(call check_defined, TOOLCHAIN_PREFIX, the prefix name of toolchain)
$(call check_defined, DEBUG_LEVEL, debug level(debug or release))

#*********************************************************************************************************
# All *.mk files
#*********************************************************************************************************
APPLICATION_MK    = $(MKTEMP)/application.mk
LIBRARY_MK        = $(MKTEMP)/library.mk
STATIC_LIBRARY_MK = $(MKTEMP)/static-library.mk
KERNEL_MODULE_MK  = $(MKTEMP)/kernel-module.mk
KERNEL_LIBRARY_MK = $(MKTEMP)/kernel-library.mk
UNIT_TEST_MK      = $(MKTEMP)/unit-test.mk
GTEST_MK          = $(MKTEMP)/gtest.mk
LIBSYLIXOS_MK     = $(MKTEMP)/libsylixos.mk
DUMMY_MK          = $(MKTEMP)/dummy.mk
BSP_MK            = $(MKTEMP)/bsp.mk
BARE_METAL_MK     = $(MKTEMP)/bare-metal.mk
EXTENSION_MK      = $(MKTEMP)/extension.mk
LITE_BSP_MK       = $(MKTEMP)/lite-bsp.mk
END_MK            = $(MKTEMP)/end.mk
CLEAR_VARS_MK     = $(MKTEMP)/clear-vars.mk

#*********************************************************************************************************
# Build paths
#*********************************************************************************************************
ifeq ($(DEBUG_LEVEL), debug)
OUTDIR = Debug
else
OUTDIR = Release
endif

ifeq ($(CUSTOM_OUT_BASE),)
TARGET_WORD_POS = 3
OUTPATH = ./$(OUTDIR)
OBJPATH = $(OUTPATH)/obj
DEPPATH = $(OUTPATH)/dep
else
TARGET_WORD_POS = 4
OUTPATH = $(OUTDIR)
OBJPATH = $(CUSTOM_OUT_BASE)$(OUTPATH)/obj
DEPPATH = $(CUSTOM_OUT_BASE)$(OUTPATH)/dep
endif

#*********************************************************************************************************
# Define some useful variables
#*********************************************************************************************************
BIAS  = /
EMPTY =
SPACE = $(EMPTY) $(EMPTY)

SYLIXOS_BASE_PATH := $(subst \,/,$(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH)))

__TARGET     = $(word $(TARGET_WORD_POS),$(subst $(BIAS),$(SPACE),$(@)))
__DEP        = $(addprefix $(DEPPATH)/$(__TARGET)/, $(addsuffix .d, $(basename $(<))))
ifneq (,$(findstring cl6x,$(TOOLCHAIN_PREFIX)))
__PP         = $(addprefix $(DEPPATH)/$(__TARGET)/, $(addsuffix .pp, $(basename $(<))))
endif
__LIBRARIES  = $($(@F)_DEPEND_LIB_PATH) $($(@F)_DEPEND_LIB)
__OBJS       = $($(@F)_OBJS_FLAGS)
__AR_SO_OBJS = $($(addsuffix .so, $(basename $(@F)))_OBJS_FLAGS)
__CPUFLAGS   = $($(@F)_CPUFLAGS)
__DSYMBOL    = $($(@F)_DSYMBOL)
__LINKFLAGS  = $($(@F)_LINKFLAGS)
__LD         = $($(@F)_LD)

__PRE_LINK_CMD   = $($(@F)_PRE_LINK_CMD)
__POST_LINK_CMD  = $($(@F)_POST_LINK_CMD)

__PRE_STRIP_CMD  = $($(@F)_PRE_STRIP_CMD)
__POST_STRIP_CMD = $($(@F)_POST_STRIP_CMD)

#*********************************************************************************************************
# HOST_OS
#*********************************************************************************************************
UNAME = $(shell uname -sm)

ifneq (,$(findstring Linux, $(UNAME)))
HOST_OS = linux
endif
ifneq (,$(findstring Darwin, $(UNAME)))
HOST_OS = darwin
endif
ifneq (,$(findstring Macintosh, $(UNAME)))
HOST_OS = darwin
endif
ifneq (,$(findstring CYGWIN, $(UNAME)))
HOST_OS = windows
endif
ifneq (,$(findstring windows, $(UNAME)))
HOST_OS = windows
endif
ifneq (,$(findstring MINGW, $(UNAME)))
HOST_OS = linux
endif
ifneq (,$(findstring MSYS_NT, $(UNAME)))
HOST_OS = linux
endif

ifeq ($(HOST_OS),)
$(error Unable to determine HOST_OS from uname -sm: $(UNAME)!)
endif

hide = @

#*********************************************************************************************************
# Define HOST_ECHO_N to perform the equivalent of 'echo -n' on all platforms.
#*********************************************************************************************************
ifeq ($(HOST_OS),windows)
HOST_ECHO_N := echo -n
else
# On Posix, just use bare printf.
HOST_ECHO_N := printf %s
endif

#*********************************************************************************************************
# Function : generate-list-file
# Arguments: 1: list of strings (possibly very long)
#            2: file name
# Returns  : write the content of a possibly very long string list to a file.
#            this shall be used in commands and will work around limitations
#            of host command-line lengths.
# Usage    : $(call host-echo-to-file,<string-list>,<file>)
# Rationale: When there is a very large number of objects and/or libraries at
#            link time, the size of the command becomes too large for the
#            host system's maximum. Various tools however support the
#            @<listfile> syntax, where <listfile> is the path of a file
#            which content will be parsed as if they were options.
#
#            This function is used to generate such a list file from a long
#            list of strings in input.
#
#*********************************************************************************************************

#*********************************************************************************************************
# Helper functions because the GNU Make $(word ...) function does
# not accept a 0 index, so we need to bump any of these to 1 when
# we find them.
#*********************************************************************************************************
index-is-zero = $(filter 0 00 000 0000 00000 000000 0000000,$1)
bump-0-to-1 = $(if $(call index-is-zero,$1),1,$1)

#*********************************************************************************************************
# Same as $(wordlist ...) except the start index, if 0, is bumped to 1
#*********************************************************************************************************
index-word-list = $(wordlist $(call bump-0-to-1,$1),$2,$3)

#*********************************************************************************************************
# NOTE: With GNU Make $1 and $(1) are equivalent, which means
#       that $10 is equivalent to $(1)0, and *not* $(10).
#
# Used to generate a slice of up to 10 items starting from index $1,
# If $1 is 0, it will be bumped to 1 (and only 9 items will be printed)
# $1: start (tenth) index. Can be 0
# $2: word list
#*********************************************************************************************************
define list-file-start-gen-10
	$$(hide) $$(HOST_ECHO_N) "$(call index-word-list,$10,$19,$2) " >> $$@
endef

#*********************************************************************************************************
# Used to generate a slice of always 10 items starting from index $1
# $1: start (tenth) index. CANNOT BE 0
# $2: word list
#*********************************************************************************************************
define list-file-always-gen-10
	$$(hide) $$(HOST_ECHO_N) "$(wordlist $10,$19,$2) " >> $$@
endef

#*********************************************************************************************************
# Same as list-file-always-gen-10, except that the word list might be
# empty at position $10 (i.e. $(1)0)
#*********************************************************************************************************
define list-file-maybe-gen-10
ifneq ($(word $10,$2),)
	$$(hide) $$(HOST_ECHO_N) "$(wordlist $10,$19,$2) " >> $$@
endif
endef

define list-file-start-gen-100
$(call list-file-start-gen-10,$10,$2)
$(call list-file-always-gen-10,$11,$2)
$(call list-file-always-gen-10,$12,$2)
$(call list-file-always-gen-10,$13,$2)
$(call list-file-always-gen-10,$14,$2)
$(call list-file-always-gen-10,$15,$2)
$(call list-file-always-gen-10,$16,$2)
$(call list-file-always-gen-10,$17,$2)
$(call list-file-always-gen-10,$18,$2)
$(call list-file-always-gen-10,$19,$2)
endef

define list-file-always-gen-100
$(call list-file-always-gen-10,$10,$2)
$(call list-file-always-gen-10,$11,$2)
$(call list-file-always-gen-10,$12,$2)
$(call list-file-always-gen-10,$13,$2)
$(call list-file-always-gen-10,$14,$2)
$(call list-file-always-gen-10,$15,$2)
$(call list-file-always-gen-10,$16,$2)
$(call list-file-always-gen-10,$17,$2)
$(call list-file-always-gen-10,$18,$2)
$(call list-file-always-gen-10,$19,$2)
endef

define list-file-maybe-gen-100
ifneq ($(word $(call bump-0-to-1,$100),$2),)
ifneq ($(word $199,$2),)
$(call list-file-start-gen-10,$10,$2)
$(call list-file-always-gen-10,$11,$2)
$(call list-file-always-gen-10,$12,$2)
$(call list-file-always-gen-10,$13,$2)
$(call list-file-always-gen-10,$14,$2)
$(call list-file-always-gen-10,$15,$2)
$(call list-file-always-gen-10,$16,$2)
$(call list-file-always-gen-10,$17,$2)
$(call list-file-always-gen-10,$18,$2)
$(call list-file-always-gen-10,$19,$2)
else
ifneq ($(word $150,$2),)
$(call list-file-start-gen-10,$10,$2)
$(call list-file-always-gen-10,$11,$2)
$(call list-file-always-gen-10,$12,$2)
$(call list-file-always-gen-10,$13,$2)
$(call list-file-always-gen-10,$14,$2)
$(call list-file-maybe-gen-10,$15,$2)
$(call list-file-maybe-gen-10,$16,$2)
$(call list-file-maybe-gen-10,$17,$2)
$(call list-file-maybe-gen-10,$18,$2)
$(call list-file-maybe-gen-10,$19,$2)
else
$(call list-file-start-gen-10,$10,$2)
$(call list-file-maybe-gen-10,$11,$2)
$(call list-file-maybe-gen-10,$12,$2)
$(call list-file-maybe-gen-10,$13,$2)
$(call list-file-maybe-gen-10,$14,$2)
endif
endif
endif
endef

define list-file-maybe-gen-1000
ifneq ($(word $(call bump-0-to-1,$1000),$2),)
ifneq ($(word $1999,$2),)
$(call list-file-start-gen-100,$10,$2)
$(call list-file-always-gen-100,$11,$2)
$(call list-file-always-gen-100,$12,$2)
$(call list-file-always-gen-100,$13,$2)
$(call list-file-always-gen-100,$14,$2)
$(call list-file-always-gen-100,$15,$2)
$(call list-file-always-gen-100,$16,$2)
$(call list-file-always-gen-100,$17,$2)
$(call list-file-always-gen-100,$18,$2)
$(call list-file-always-gen-100,$19,$2)
else
ifneq ($(word $1500,$2),)
$(call list-file-start-gen-100,$10,$2)
$(call list-file-always-gen-100,$11,$2)
$(call list-file-always-gen-100,$12,$2)
$(call list-file-always-gen-100,$13,$2)
$(call list-file-always-gen-100,$14,$2)
$(call list-file-maybe-gen-100,$15,$2)
$(call list-file-maybe-gen-100,$16,$2)
$(call list-file-maybe-gen-100,$17,$2)
$(call list-file-maybe-gen-100,$18,$2)
$(call list-file-maybe-gen-100,$19,$2)
else
$(call list-file-start-gen-100,$10,$2)
$(call list-file-maybe-gen-100,$11,$2)
$(call list-file-maybe-gen-100,$12,$2)
$(call list-file-maybe-gen-100,$13,$2)
$(call list-file-maybe-gen-100,$14,$2)
endif
endif
endif
endef

define generate-list-file-ev
__list_file := $2

$$(__list_file): $1
	@if [ ! -d "$(dir $2)" ]; then \
		mkdir -p "$(dir $2)"; fi
	$$(hide) $$(HOST_ECHO_N) "" > $$@
$(call list-file-maybe-gen-1000,0,$1)
$(call list-file-maybe-gen-1000,1,$1)
$(call list-file-maybe-gen-1000,2,$1)
$(call list-file-maybe-gen-1000,3,$1)
$(call list-file-maybe-gen-1000,4,$1)
$(call list-file-maybe-gen-1000,5,$1)
$(call list-file-maybe-gen-1000,6,$1)
$(call list-file-maybe-gen-1000,7,$1)
$(call list-file-maybe-gen-1000,8,$1)
$(call list-file-maybe-gen-1000,9,$1)
endef

generate-list-file = $(eval $(call generate-list-file-ev,$1,$2))

#*********************************************************************************************************
# Do not export the following environment variables 
#*********************************************************************************************************
unexport CPATH
unexport C_INCLUDE_PATH
unexport CPLUS_INCLUDE_PATH
unexport OBJC_INCLUDE_PATH
unexport LIBRARY_PATH
unexport LD_LIBRARY_PATH

#*********************************************************************************************************
# Include toolchain mk
#*********************************************************************************************************
ifneq (,$(findstring cl6x,$(TOOLCHAIN_PREFIX)))
include $(MKTEMP)/cl6x.mk
else
include $(MKTEMP)/gcc.mk
endif

#*********************************************************************************************************
# Include arch.mk
#*********************************************************************************************************
include $(MKTEMP)/arch.mk

#*********************************************************************************************************
# End
#*********************************************************************************************************
