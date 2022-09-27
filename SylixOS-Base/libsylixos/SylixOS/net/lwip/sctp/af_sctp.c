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
** 文   件   名: af_sctp.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2022 年 04 月 09 日
**
** 描        述: AF_SCTP 支持
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_SCTP_EN > 0
#include "limits.h"
#include "socket.h"
#include "af_sctp.h"
/*********************************************************************************************************
  SCTP 驱动
*********************************************************************************************************/
static PLW_SCTP_DRV  _G_psctpDrv = LW_NULL;
/*********************************************************************************************************
** 函数名称: API_SctpDrvInstall
** 功能描述: 安装 SCTP 协议驱动
** 输　入  : psctpDrv          SCTP 协议驱动
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_SctpDrvInstall (PLW_SCTP_DRV   psctpDrv)
{
    if (psctpDrv) {
        _G_psctpDrv = psctpDrv;
        return  (ERROR_NONE);
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: kmsctp_socket
** 功能描述: sctp socket
** 输　入  : iDomain        域, 必须是 AF_SCTP
**           iType          SOCK_STREAM / SOCK_DGRAM / SOCK_SEQPACKET
**           iProtocol      协议
** 输　出  : sctp socket
** 全局变量:
** 调用模块:
*********************************************************************************************************/
AF_SCTP_T  *kmsctp_socket (INT  iDomain, INT  iType, INT  iProtocol)
{
    AF_SCTP_T  *pafsctp;

    if (_G_psctpDrv && _G_psctpDrv->DRV_socket) {
        pafsctp = _G_psctpDrv->DRV_socket(iDomain, iType, iProtocol);
    } else {
        _ErrorHandle(ENOSYS);
        pafsctp = LW_NULL;
    }

    return  (pafsctp);
}
/*********************************************************************************************************
** 函数名称: kmsctp_bind
** 功能描述: bind
** 输　入  : pafsctp   sctp file
**           name      address
**           namelen   address len
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  kmsctp_bind (AF_SCTP_T  *pafsctp, const struct sockaddr *name, socklen_t namelen)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_bind) {
        iRet = _G_psctpDrv->DRV_bind(pafsctp, name, namelen);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_listen
** 功能描述: listen
** 输　入  : pafsctp   sctp file
**           backlog   back log num
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  kmsctp_listen (AF_SCTP_T  *pafsctp, INT  backlog)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_listen) {
        iRet = _G_psctpDrv->DRV_listen(pafsctp, backlog);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_accept
** 功能描述: accept
** 输　入  : pafsctp   sctp file
**           addr      address
**           addrlen   address len
** 输　出  : sctp socket
** 全局变量:
** 调用模块:
*********************************************************************************************************/
AF_SCTP_T  *kmsctp_accept (AF_SCTP_T  *pafsctp, struct sockaddr *addr, socklen_t *addrlen)
{
    AF_SCTP_T  *pafsctpNew;

    if (_G_psctpDrv && _G_psctpDrv->DRV_accept) {
        pafsctpNew = _G_psctpDrv->DRV_accept(pafsctp, addr, addrlen);
    } else {
        _ErrorHandle(ENOSYS);
        pafsctpNew = LW_NULL;
    }

    return  (pafsctpNew);
}
/*********************************************************************************************************
** 函数名称: kmsctp_connect
** 功能描述: connect
** 输　入  : pafsctp   sctp file
**           name      address
**           namelen   address len
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  kmsctp_connect (AF_SCTP_T  *pafsctp, const struct sockaddr *name, socklen_t namelen)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_connect) {
        iRet = _G_psctpDrv->DRV_connect(pafsctp, name, namelen);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_recvmsg
** 功能描述: recvmsg
** 输　入  : pafsctp   sctp file
**           msg       消息
**           flags     flag
** 输　出  : NUM 数据长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  kmsctp_recvmsg (AF_SCTP_T  *pafsctp, struct msghdr *msg, int flags)
{
    ssize_t     ssRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_recvmsg) {
        ssRet = _G_psctpDrv->DRV_recvmsg(pafsctp, msg, flags);
    } else {
        _ErrorHandle(ENOSYS);
        ssRet = PX_ERROR;
    }

    return  (ssRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_recvfrom
** 功能描述: recvfrom
** 输　入  : pafsctp   sctp file
**           mem       buffer
**           len       buffer len
**           flags     flag
**           from      packet from
**           fromlen   name len
** 输　出  : NUM
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  kmsctp_recvfrom (AF_SCTP_T  *pafsctp, void *mem, size_t len, int flags,
                          struct sockaddr *from, socklen_t *fromlen)
{
    ssize_t     ssRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_recvfrom) {
        ssRet = _G_psctpDrv->DRV_recvfrom(pafsctp, mem, len, flags, from, fromlen);
    } else {
        _ErrorHandle(ENOSYS);
        ssRet = PX_ERROR;
    }

    return  (ssRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_recv
** 功能描述: recv
** 输　入  : pafsctp   sctp file
**           mem       buffer
**           len       buffer len
**           flags     flag
** 输　出  : NUM
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  kmsctp_recv (AF_SCTP_T  *pafsctp, void *mem, size_t len, int flags)
{
    ssize_t     ssRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_recv) {
        ssRet = _G_psctpDrv->DRV_recv(pafsctp, mem, len, flags);
    } else {
        _ErrorHandle(ENOSYS);
        ssRet = PX_ERROR;
    }

    return  (ssRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_sendmsg
** 功能描述: sendmsg
** 输　入  : pafsctp   sctp file
**           msg       消息
**           flags     flag
** 输　出  : NUM 数据长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  kmsctp_sendmsg (AF_SCTP_T  *pafsctp, const struct msghdr *msg, int flags)
{
    ssize_t     ssRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_sendmsg) {
        ssRet = _G_psctpDrv->DRV_sendmsg(pafsctp, msg, flags);
    } else {
        _ErrorHandle(ENOSYS);
        ssRet = PX_ERROR;
    }

    return  (ssRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_sendto
** 功能描述: sendto
** 输　入  : pafsctp   sctp file
**           data      send buffer
**           size      send len
**           flags     flag
**           to        packet to
**           tolen     name len
** 输　出  : NUM
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  kmsctp_sendto (AF_SCTP_T  *pafsctp, const void *data, size_t size, int flags,
                        const struct sockaddr *to, socklen_t tolen)
{
    ssize_t     ssRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_sendto) {
        ssRet = _G_psctpDrv->DRV_sendto(pafsctp, data, size, flags, to, tolen);
    } else {
        _ErrorHandle(ENOSYS);
        ssRet = PX_ERROR;
    }

    return  (ssRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_send
** 功能描述: send
** 输　入  : pafsctp   sctp file
**           data      send buffer
**           size      send len
**           flags     flag
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t  kmsctp_send (AF_SCTP_T  *pafsctp, const void *data, size_t size, int flags)
{
    ssize_t     ssRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_send) {
        ssRet = _G_psctpDrv->DRV_send(pafsctp, data, size, flags);
    } else {
        _ErrorHandle(ENOSYS);
        ssRet = PX_ERROR;
    }

    return  (ssRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_close
** 功能描述: close
** 输　入  : pafsctp   sctp file
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  kmsctp_close (AF_SCTP_T  *pafsctp)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_close) {
        iRet = _G_psctpDrv->DRV_close(pafsctp);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_shutdown
** 功能描述: shutdown
** 输　入  : pafsctp   sctp file
**           how       SHUT_RD  SHUT_WR  SHUT_RDWR
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  kmsctp_shutdown (AF_SCTP_T  *pafsctp, int how)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_shutdown) {
        iRet = _G_psctpDrv->DRV_shutdown(pafsctp, how);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_getsockname
** 功能描述: getsockname
** 输　入  : pafsctp   sctp file
**           name      address
**           namelen   address len
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  kmsctp_getsockname (AF_SCTP_T  *pafsctp, struct sockaddr *name, socklen_t *namelen)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_getsockname) {
        iRet = _G_psctpDrv->DRV_getsockname(pafsctp, name, namelen);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_getpeername
** 功能描述: getpeername
** 输　入  : pafsctp   sctp file
**           name      address
**           namelen   address len
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  kmsctp_getpeername (AF_SCTP_T  *pafsctp, struct sockaddr *name, socklen_t *namelen)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_getpeername) {
        iRet = _G_psctpDrv->DRV_getpeername(pafsctp, name, namelen);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_setsockopt
** 功能描述: setsockopt
** 输　入  : pafsctp   sctp file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  kmsctp_setsockopt (AF_SCTP_T  *pafsctp, int level, int optname, const void *optval, socklen_t optlen)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_setsockopt) {
        iRet = _G_psctpDrv->DRV_setsockopt(pafsctp, level, optname, optval, optlen);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_getsockopt
** 功能描述: getsockopt
** 输　入  : pafsctp   sctp file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  kmsctp_getsockopt (AF_SCTP_T  *pafsctp, int level, int optname, void *optval, socklen_t *optlen)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_getsockopt) {
        iRet = _G_psctpDrv->DRV_getsockopt(pafsctp, level, optname, optval, optlen);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: kmsctp_ioctl
** 功能描述: ioctl
** 输　入  : pafsctp   sctp file
**           iCmd      命令
**           pvArg     参数
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  kmsctp_ioctl (AF_SCTP_T  *pafsctp, INT  iCmd, PVOID  pvArg)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_ioctl) {
        iRet = _G_psctpDrv->DRV_ioctl(pafsctp, iCmd, pvArg);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __kmsctp_have_event
** 功能描述: 检测对应的控制块是否有指定的事件
** 输　入  : pafsctp   sctp file
**           type      事件类型
**           piSoErr   如果等待的事件有效则更新 SO_ERROR
** 输　出  : ERROR
** 全局变量:
** 调用模块:
** 说  明  :

SELREAD:
    1>.套接口有数据可读
    2>.该连接的读这一半关闭 (也就是接收了FIN的TCP连接). 对这样的套接口进行读操作将不阻塞并返回0
    3>.该套接口是一个侦听套接口且已完成的连接数不为0.
    4>.其上有一个套接口错误待处理，对这样的套接口的读操作将不阻塞并返回-1, 并设置errno.
       可以通过设置 SO_ERROR 选项调用 getsockopt 函数获得.

SELWRITE:
    1>.套接口有可用于写的空间.
    2>.该连接的写这一半关闭，对这样的套接口进行写操作将产生SIGPIPE信号.
    3>.该套接口使用非阻塞的方式connect建立连接，并且连接已经异步建立，或则connect已经以失败告终.
    4>.其上有一个套接口错误待处理.

SELEXCEPT
    1>.面向连接的套接口没有连接.
*********************************************************************************************************/
int  __kmsctp_have_event (AF_SCTP_T *pafsctp, int type, int  *piSoErr)
{
    INT     iRet;

    if (_G_psctpDrv && _G_psctpDrv->DRV_have_event) {
        iRet = _G_psctpDrv->DRV_have_event(pafsctp, type, piSoErr);
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __kmsctp_set_sockfile
** 功能描述: 设置对应的 socket 文件
** 输　入  : pafsctp   sctp file
**           file      文件
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
void  __kmsctp_set_sockfile (AF_SCTP_T *pafsctp, void *file)
{
    if (_G_psctpDrv && _G_psctpDrv->DRV_set_sockfile) {
        _G_psctpDrv->DRV_set_sockfile(pafsctp, file);
    }
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_SCTP_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
