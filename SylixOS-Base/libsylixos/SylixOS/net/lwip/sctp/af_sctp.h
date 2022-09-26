/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: af_sctp.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2022 年 04 月 09 日
**
** 描        述: AF_SCTP 支持
*********************************************************************************************************/

#ifndef __AF_SCTP_H
#define __AF_SCTP_H

/*********************************************************************************************************
  AF_SCTP 控制块
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_SCTP_EN > 0

typedef VOID    AF_SCTP_T;

/*********************************************************************************************************
  AF_SCTP 驱动
*********************************************************************************************************/

typedef struct lw_sctp_drv {
    AF_SCTP_T  *(*DRV_socket)(INT  iDomain, INT  iType, INT  iProtocol);
    INT         (*DRV_bind)(AF_SCTP_T  *pafsctp, const struct sockaddr *name, socklen_t namelen);
    INT         (*DRV_listen)(AF_SCTP_T  *pafsctp, INT  backlog);
    AF_SCTP_T  *(*DRV_accept)(AF_SCTP_T  *pafsctp, struct sockaddr *addr, socklen_t *addrlen);
    INT         (*DRV_connect)(AF_SCTP_T  *pafsctp, const struct sockaddr *name, socklen_t namelen);
    ssize_t     (*DRV_recvfrom)(AF_SCTP_T  *pafsctp, void *mem, size_t len, int flags,
                                 struct sockaddr *from, socklen_t *fromlen);
    ssize_t     (*DRV_recv)(AF_SCTP_T  *pafsctp, void *mem, size_t len, int flags);
    ssize_t     (*DRV_recvmsg)(AF_SCTP_T  *pafsctp, struct msghdr *msg, int flags);
    ssize_t     (*DRV_sendto)(AF_SCTP_T  *pafsctp, const void *data, size_t size, int flags,
                               const struct sockaddr *to, socklen_t tolen);
    ssize_t     (*DRV_send)(AF_SCTP_T  *pafsctp, const void *data, size_t size, int flags);
    ssize_t     (*DRV_sendmsg)(AF_SCTP_T  *pafsctp, const struct msghdr *msg, int flags);
    INT         (*DRV_close)(AF_SCTP_T  *pafsctp);
    INT         (*DRV_shutdown)(AF_SCTP_T  *pafsctp, int how);
    INT         (*DRV_getsockname)(AF_SCTP_T  *pafsctp, struct sockaddr *name, socklen_t *namelen);
    INT         (*DRV_getpeername)(AF_SCTP_T  *pafsctp, struct sockaddr *name, socklen_t *namelen);
    INT         (*DRV_setsockopt)(AF_SCTP_T  *pafsctp, int level, int optname,
                                   const void *optval, socklen_t optlen);
    INT         (*DRV_getsockopt)(AF_SCTP_T  *pafsctp, int level, int optname,
                                  void *optval, socklen_t *optlen);
    INT         (*DRV_ioctl)(AF_SCTP_T  *pafsctp, INT  iCmd, PVOID  pvArg);
    INT         (*DRV_have_event)(AF_SCTP_T *pafsctp, int type, int  *piSoErr);
    void        (*DRV_set_sockfile)(AF_SCTP_T *pafsctp, void *file);
} LW_SCTP_DRV, *PLW_SCTP_DRV;

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API INT  API_SctpDrvInstall(PLW_SCTP_DRV   psctpDrv);

/*********************************************************************************************************
  内部函数
*********************************************************************************************************/

AF_SCTP_T  *kmsctp_socket(INT  iDomain, INT  iType, INT  iProtocol);
INT         kmsctp_bind(AF_SCTP_T  *pafsctp, const struct sockaddr *name, socklen_t namelen);
INT         kmsctp_listen(AF_SCTP_T  *pafsctp, INT  backlog);
AF_SCTP_T  *kmsctp_accept(AF_SCTP_T  *pafsctp, struct sockaddr *addr, socklen_t *addrlen);
INT         kmsctp_connect(AF_SCTP_T  *pafsctp, const struct sockaddr *name, socklen_t namelen);
ssize_t     kmsctp_recvfrom(AF_SCTP_T  *pafsctp, void *mem, size_t len, int flags,
                            struct sockaddr *from, socklen_t *fromlen);
ssize_t     kmsctp_recv(AF_SCTP_T  *pafsctp, void *mem, size_t len, int flags);
ssize_t     kmsctp_recvmsg(AF_SCTP_T  *pafsctp, struct msghdr *msg, int flags);
ssize_t     kmsctp_sendto(AF_SCTP_T  *pafsctp, const void *data, size_t size, int flags,
                          const struct sockaddr *to, socklen_t tolen);
ssize_t     kmsctp_send(AF_SCTP_T  *pafsctp, const void *data, size_t size, int flags);
ssize_t     kmsctp_sendmsg(AF_SCTP_T  *pafsctp, const struct msghdr *msg, int flags);
INT         kmsctp_close(AF_SCTP_T  *pafsctp);
INT         kmsctp_shutdown(AF_SCTP_T  *pafsctp, int how);
INT         kmsctp_getsockname(AF_SCTP_T  *pafsctp, struct sockaddr *name, socklen_t *namelen);
INT         kmsctp_getpeername(AF_SCTP_T  *pafsctp, struct sockaddr *name, socklen_t *namelen);
INT         kmsctp_setsockopt(AF_SCTP_T  *pafsctp, int level, int optname,
                              const void *optval, socklen_t optlen);
INT         kmsctp_getsockopt(AF_SCTP_T  *pafsctp, int level, int optname,
                              void *optval, socklen_t *optlen);
INT         kmsctp_ioctl(AF_SCTP_T  *pafsctp, INT  iCmd, PVOID  pvArg);

/*********************************************************************************************************
  AF_SCTP 协议域 (内部使用)
*********************************************************************************************************/

#define AF_SCTP     (AF_MAX + 1)

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_SCTP_EN > 0      */
#endif                                                                  /*  __AF_SCTP_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
