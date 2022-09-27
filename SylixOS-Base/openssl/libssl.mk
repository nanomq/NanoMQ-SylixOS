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
# ��   ��   ��: libssl.mk
#
# ��   ��   ��: RealEvo-IDE
#
# �ļ���������: 2022 �� 06 �� 30 ��
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
LOCAL_TARGET_NAME := libssl.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
openssl/ssl/bio_ssl.c \
openssl/ssl/d1_lib.c \
openssl/ssl/d1_msg.c \
openssl/ssl/d1_srtp.c \
openssl/ssl/methods.c \
openssl/ssl/packet.c \
openssl/ssl/pqueue.c \
openssl/ssl/record/dtls1_bitmap.c \
openssl/ssl/record/rec_layer_d1.c \
openssl/ssl/record/rec_layer_s3.c \
openssl/ssl/record/ssl3_buffer.c \
openssl/ssl/record/ssl3_record.c \
openssl/ssl/record/ssl3_record_tls13.c \
openssl/ssl/s3_cbc.c \
openssl/ssl/s3_enc.c \
openssl/ssl/s3_lib.c \
openssl/ssl/s3_msg.c \
openssl/ssl/ssl_asn1.c \
openssl/ssl/ssl_cert.c \
openssl/ssl/ssl_ciph.c \
openssl/ssl/ssl_conf.c \
openssl/ssl/ssl_err.c \
openssl/ssl/ssl_init.c \
openssl/ssl/ssl_lib.c \
openssl/ssl/ssl_mcnf.c \
openssl/ssl/ssl_rsa.c \
openssl/ssl/ssl_sess.c \
openssl/ssl/ssl_stat.c \
openssl/ssl/ssl_txt.c \
openssl/ssl/ssl_utst.c \
openssl/ssl/statem/extensions.c \
openssl/ssl/statem/extensions_clnt.c \
openssl/ssl/statem/extensions_cust.c \
openssl/ssl/statem/extensions_srvr.c \
openssl/ssl/statem/statem.c \
openssl/ssl/statem/statem_clnt.c \
openssl/ssl/statem/statem_dtls.c \
openssl/ssl/statem/statem_lib.c \
openssl/ssl/statem/statem_srvr.c \
openssl/ssl/t1_enc.c \
openssl/ssl/t1_lib.c \
openssl/ssl/t1_trce.c \
openssl/ssl/tls13_enc.c \
openssl/ssl/tls_srp.c

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./openssl/include" \
-I"./openssl"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := \
-DOPENSSL_USE_NODELETE \
-DOPENSSL_PIC \
-DNDEBUG

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS   := 
LOCAL_CXXFLAGS := 
LOCAL_LINKFLAGS :=

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB :=  \
-lcrypto -latomic

LOCAL_DEPEND_LIB_PATH :=  \
-L$(OUTDIR)

#*********************************************************************************************************
# C++ config
#*********************************************************************************************************
LOCAL_USE_CXX        := no
LOCAL_USE_CXX_EXCEPT := no

#*********************************************************************************************************
# Code coverage config
#*********************************************************************************************************
LOCAL_USE_GCOV := no

#*********************************************************************************************************
# OpenMP config
#*********************************************************************************************************
LOCAL_USE_OMP := no

#*********************************************************************************************************
# Use short command for link and ar
#*********************************************************************************************************
LOCAL_USE_SHORT_CMD := no

#*********************************************************************************************************
# User link command
#*********************************************************************************************************
LOCAL_PRE_LINK_CMD   := 
LOCAL_POST_LINK_CMD  := 
LOCAL_PRE_STRIP_CMD  := 
LOCAL_POST_STRIP_CMD := 

#*********************************************************************************************************
# Depend target
#*********************************************************************************************************
LOCAL_DEPEND_TARGET := ./$(OUTDIR)/libcrypto.so

include $(LIBRARY_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
