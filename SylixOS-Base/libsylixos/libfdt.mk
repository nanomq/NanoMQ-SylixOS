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
# 文   件   名: libfdt.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2018 年 04 月 23 日
#
# 描        述: 本文件由 RealEvo-IDE 生成，用于配置 Makefile 功能，请勿手动修改
#*********************************************************************************************************

#*********************************************************************************************************
# Clear setting
#*********************************************************************************************************
include $(CLEAR_VARS_MK)

#*********************************************************************************************************
# Target
#*********************************************************************************************************
LOCAL_TARGET_NAME := libfdt.a

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
SylixOS/driver/fdt/fdt_addresses.c \
SylixOS/driver/fdt/fdt_empty_tree.c \
SylixOS/driver/fdt/fdt_overlay.c \
SylixOS/driver/fdt/fdt_ro.c \
SylixOS/driver/fdt/fdt_rw.c \
SylixOS/driver/fdt/fdt_strerror.c \
SylixOS/driver/fdt/fdt_sw.c \
SylixOS/driver/fdt/fdt_wip.c \
SylixOS/driver/fdt/fdt.c 

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your hearder files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := -I"SylixOS/driver/fdt"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := 

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB      := 
LOCAL_DEPEND_LIB_PATH := 

#*********************************************************************************************************
# C++ config
#*********************************************************************************************************
LOCAL_USE_CXX        := no
LOCAL_USE_CXX_EXCEPT := no

#*********************************************************************************************************
# Code coverage config
#*********************************************************************************************************
LOCAL_USE_GCOV := no

include $(KERNEL_LIBRARY_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
