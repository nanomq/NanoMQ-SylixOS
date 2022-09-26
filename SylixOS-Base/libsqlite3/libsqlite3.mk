#*********************************************************************************************************
#
#                                    �й�������Դ��֯
#
#                                   Ƕ��ʽʵʱ����ϵͳ
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------�ļ���Ϣ--------------------------------------------------------------------------------
#
# ��   ��   ��: libsqlite3.mk
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
LOCAL_TARGET_NAME := libsqlite3.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
sqlite3/sqlite3.c

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your hearder files search path")
#*********************************************************************************************************
LOCAL_INC_PATH := 

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := \
-DSQLITE_THREADSAFE=1 \
-DSQLITE_ENABLE_FTS3=1 \
-DSQLITE_ENABLE_FTS3_PARENTHESIS=1 \
-DSQLITE_ENABLE_FTS4=1 \
-DSQLITE_ENABLE_FTS5=1 \
-DSQLITE_ENABLE_JSON1=1 \
-DSQLITE_ENABLE_RTREE=1 \
-DSQLITE_ENABLE_UPDATE_DELETE_LIMIT=1 \
-D__NEED_LIKELY__

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