/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: af_sctp.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2022 �� 04 �� 09 ��
**
** ��        ��: AF_SCTP ֧��
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_SCTP_EN > 0
#include "limits.h"
#include "socket.h"
#include "af_sctp.h"
/*********************************************************************************************************
  SCTP ����
*********************************************************************************************************/
static PLW_SCTP_DRV  _G_psctpDrv = LW_NULL;
/*********************************************************************************************************
** ��������: API_SctpDrvInstall
** ��������: ��װ SCTP Э������
** �䡡��  : psctpDrv          SCTP Э������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_socket
** ��������: sctp socket
** �䡡��  : iDomain        ��, ������ AF_SCTP
**           iType          SOCK_STREAM / SOCK_DGRAM / SOCK_SEQPACKET
**           iProtocol      Э��
** �䡡��  : sctp socket
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_bind
** ��������: bind
** �䡡��  : pafsctp   sctp file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_listen
** ��������: listen
** �䡡��  : pafsctp   sctp file
**           backlog   back log num
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_accept
** ��������: accept
** �䡡��  : pafsctp   sctp file
**           addr      address
**           addrlen   address len
** �䡡��  : sctp socket
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_connect
** ��������: connect
** �䡡��  : pafsctp   sctp file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_recvmsg
** ��������: recvmsg
** �䡡��  : pafsctp   sctp file
**           msg       ��Ϣ
**           flags     flag
** �䡡��  : NUM ���ݳ���
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_recvfrom
** ��������: recvfrom
** �䡡��  : pafsctp   sctp file
**           mem       buffer
**           len       buffer len
**           flags     flag
**           from      packet from
**           fromlen   name len
** �䡡��  : NUM
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_recv
** ��������: recv
** �䡡��  : pafsctp   sctp file
**           mem       buffer
**           len       buffer len
**           flags     flag
** �䡡��  : NUM
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_sendmsg
** ��������: sendmsg
** �䡡��  : pafsctp   sctp file
**           msg       ��Ϣ
**           flags     flag
** �䡡��  : NUM ���ݳ���
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_sendto
** ��������: sendto
** �䡡��  : pafsctp   sctp file
**           data      send buffer
**           size      send len
**           flags     flag
**           to        packet to
**           tolen     name len
** �䡡��  : NUM
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_send
** ��������: send
** �䡡��  : pafsctp   sctp file
**           data      send buffer
**           size      send len
**           flags     flag
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_close
** ��������: close
** �䡡��  : pafsctp   sctp file
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_shutdown
** ��������: shutdown
** �䡡��  : pafsctp   sctp file
**           how       SHUT_RD  SHUT_WR  SHUT_RDWR
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_getsockname
** ��������: getsockname
** �䡡��  : pafsctp   sctp file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_getpeername
** ��������: getpeername
** �䡡��  : pafsctp   sctp file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_setsockopt
** ��������: setsockopt
** �䡡��  : pafsctp   sctp file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_getsockopt
** ��������: getsockopt
** �䡡��  : pafsctp   sctp file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: kmsctp_ioctl
** ��������: ioctl
** �䡡��  : pafsctp   sctp file
**           iCmd      ����
**           pvArg     ����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __kmsctp_have_event
** ��������: ����Ӧ�Ŀ��ƿ��Ƿ���ָ�����¼�
** �䡡��  : pafsctp   sctp file
**           type      �¼�����
**           piSoErr   ����ȴ����¼���Ч����� SO_ERROR
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
** ˵  ��  :

SELREAD:
    1>.�׽ӿ������ݿɶ�
    2>.�����ӵĶ���һ��ر� (Ҳ���ǽ�����FIN��TCP����). ���������׽ӿڽ��ж�������������������0
    3>.���׽ӿ���һ�������׽ӿ�������ɵ���������Ϊ0.
    4>.������һ���׽ӿڴ�����������������׽ӿڵĶ�������������������-1, ������errno.
       ����ͨ������ SO_ERROR ѡ����� getsockopt �������.

SELWRITE:
    1>.�׽ӿ��п�����д�Ŀռ�.
    2>.�����ӵ�д��һ��رգ����������׽ӿڽ���д����������SIGPIPE�ź�.
    3>.���׽ӿ�ʹ�÷������ķ�ʽconnect�������ӣ����������Ѿ��첽����������connect�Ѿ���ʧ�ܸ���.
    4>.������һ���׽ӿڴ��������.

SELEXCEPT
    1>.�������ӵ��׽ӿ�û������.
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
** ��������: __kmsctp_set_sockfile
** ��������: ���ö�Ӧ�� socket �ļ�
** �䡡��  : pafsctp   sctp file
**           file      �ļ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
