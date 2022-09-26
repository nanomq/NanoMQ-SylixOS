#*********************************************************************************************************
#
#                                    �й������Դ��֯
#
#                                   Ƕ��ʽʵʱ����ϵͳ
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------�ļ���Ϣ--------------------------------------------------------------------------------
#
# ��   ��   ��: libreadline.mk
#
# ��   ��   ��: RealEvo-IDE
#
# �ļ���������: 2016 �� 10 �� 08 ��
#
# ��        ��: ���ļ��� RealEvo-IDE ���ɣ��������� Makefile ���ܣ������ֶ��޸�
#*********************************************************************************************************

#*********************************************************************************************************
# Clear setting
#*********************************************************************************************************
include $(CLEAR_VARS_MK)

#*********************************************************************************************************
# Target
#*********************************************************************************************************
LOCAL_TARGET_NAME := libreadline.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
readline/bind.c \
readline/callback.c \
readline/colors.c \
readline/compat.c \
readline/complete.c \
readline/display.c \
readline/funmap.c \
readline/histexpand.c \
readline/histfile.c \
readline/history.c \
readline/histsearch.c \
readline/input.c \
readline/isearch.c \
readline/keymaps.c \
readline/kill.c \
readline/macro.c \
readline/mbutil.c \
readline/misc.c \
readline/nls.c \
readline/parens.c \
readline/parse-colors.c \
readline/readline.c \
readline/rltty.c \
readline/savestring.c \
readline/search.c \
readline/shell.c \
readline/signals.c \
readline/terminal.c \
readline/text.c \
readline/tilde.c \
readline/undo.c \
readline/util.c \
readline/vi_mode.c \
readline/xfree.c \
readline/xmalloc.c 

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your hearder files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := \
-I"." \
-I"./readline" \
-I"$(SYLIXOS_BASE_PATH)/libcextern/libcextern/include"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := 
LOCAL_DSYMBOL += -DHAVE_CONFIG_H
LOCAL_DSYMBOL += -D_POSIX_VERSION=2008

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB      := -lcextern
LOCAL_DEPEND_LIB_PATH := -L"$(SYLIXOS_BASE_PATH)/libcextern/$(OUTDIR)"

#*********************************************************************************************************
# C++ config
#*********************************************************************************************************
LOCAL_USE_CXX        := no
LOCAL_USE_CXX_EXCEPT := no

#*********************************************************************************************************
# Code coverage config
#*********************************************************************************************************
LOCAL_USE_GCOV := no

include $(LIBRARY_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
