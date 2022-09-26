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
# 文   件   名: openssl.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2022 年 06 月 30 日
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
LOCAL_TARGET_NAME := openssl

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
openssl/apps/asn1pars.c \
openssl/apps/ca.c \
openssl/apps/ciphers.c \
openssl/apps/cms.c \
openssl/apps/crl.c \
openssl/apps/crl2p7.c \
openssl/apps/dgst.c \
openssl/apps/dhparam.c \
openssl/apps/dsa.c \
openssl/apps/dsaparam.c \
openssl/apps/ec.c \
openssl/apps/ecparam.c \
openssl/apps/enc.c \
openssl/apps/engine.c \
openssl/apps/errstr.c \
openssl/apps/gendsa.c \
openssl/apps/genpkey.c \
openssl/apps/genrsa.c \
openssl/apps/nseq.c \
openssl/apps/ocsp.c \
openssl/apps/openssl.c \
openssl/apps/passwd.c \
openssl/apps/pkcs12.c \
openssl/apps/pkcs7.c \
openssl/apps/pkcs8.c \
openssl/apps/pkey.c \
openssl/apps/pkeyparam.c \
openssl/apps/pkeyutl.c \
openssl/apps/prime.c \
openssl/apps/rand.c \
openssl/apps/rehash.c \
openssl/apps/req.c \
openssl/apps/rsa.c \
openssl/apps/rsautl.c \
openssl/apps/s_client.c \
openssl/apps/s_server.c \
openssl/apps/s_time.c \
openssl/apps/sess_id.c \
openssl/apps/smime.c \
openssl/apps/speed.c \
openssl/apps/spkac.c \
openssl/apps/srp.c \
openssl/apps/storeutl.c \
openssl/apps/ts.c \
openssl/apps/verify.c \
openssl/apps/version.c \
openssl/apps/x509.c

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./openssl/include" \
-I"./openssl"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := -DNDEBUG

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
-lapps \
-lssl \
-lcrypto
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
LOCAL_POST_LINK_CMD  := cp openssl/apps/openssl.cnf openssl/apps/ct_log_list.cnf $(OUTDIR)
LOCAL_PRE_STRIP_CMD  := 
LOCAL_POST_STRIP_CMD := cp openssl/apps/openssl.cnf openssl/apps/ct_log_list.cnf $(OUTDIR)/strip

#*********************************************************************************************************
# Depend target
#*********************************************************************************************************
LOCAL_DEPEND_TARGET := ./$(OUTDIR)/libcrypto.so ./$(OUTDIR)/libssl.so ./$(OUTDIR)/libapps.a

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
