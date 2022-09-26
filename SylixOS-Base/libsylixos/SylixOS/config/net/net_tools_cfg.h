/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: net_tools_cfg.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 13 ��
**
** ��        ��: ���繤������.

** BUG:
2011.03.10  �ϲ� syslog ����, ʹ���� posix ��׼, �������ò��ٷ�������.
*********************************************************************************************************/

#ifndef __NET_TOOLS_CFG_H
#define __NET_TOOLS_CFG_H

/*********************************************************************************************************
                                            ping (ICMP)
*  �����ϵ: 1: ����
             2: shell
             3: thread ext
             4: raw
*********************************************************************************************************/

#define LW_CFG_NET_PING_EN                          1                   /*  �Ƿ���Ҫ ping ����          */

/*********************************************************************************************************
                                            telnet (TCP : 23)
*  �����ϵ: 1: ����
             2: shell
             3: pty
             4: thread cancel
             5: tcp
*********************************************************************************************************/

#define LW_CFG_NET_TELNET_EN                        1                   /*  �Ƿ�ʹ�� telnet ����        */
#define LW_CFG_NET_TELNET_LOGFD_EN                  0                   /*  �Ƿ����ӵ� logmsg ����ļ���*/
#define LW_CFG_NET_TELNET_LOGIN_EN                  1                   /*  �Ƿ�ʹ���û���½            */
#if LW_CFG_CPU_WORD_LENGHT == 32
#define LW_CFG_NET_TELNET_STK_SIZE                  (6 * LW_CFG_KB_SIZE)/*  telnet ����̶߳�ջ         */
#else
#define LW_CFG_NET_TELNET_STK_SIZE                 (12 * LW_CFG_KB_SIZE)/*  telnet ����̶߳�ջ         */
#endif
#define LW_CFG_NET_TELNET_MAX_LINKS                 10                  /*  telnet ���������, ����Ϊ 5 */
#define LW_CFG_NET_TELNET_RBUFSIZE                  128                 /*  pty read buffer size        */
#define LW_CFG_NET_TELNET_WBUFSIZE                  128                 /*  pty write buffer size       */
                                                                        /*  ������ΪĬ��ֵ, ioctl ���޸�*/

/*********************************************************************************************************
                                            netbios (UDP : 137)
*  �����ϵ: 1: ����
             2: udp
*********************************************************************************************************/

#define LW_CFG_NET_NETBIOS_EN                       1                   /*  �Ƿ�ʹ�ܼ���netbios���ַ��� */

/*********************************************************************************************************
                                            tftp (UDP : 69)
*  �����ϵ: 1: ����
             2: �ļ�ϵͳ
             3: udp
*  ע  ��  : tftp client ��Ҫ��Ϊ sylixos ϵͳ�ı��� bootloader ��������, ���Ի�����ϵͳ����.
             tftp server ��Ҫ��Ϊ�������ϵͳ�ľ�������������.
             tftp Э����Լ�, ֻҪ LW_CFG_NET_TFTP_EN Ϊ 1, ��ʾ tftp ��������ͻ�����ʹ��.
*********************************************************************************************************/

#define LW_CFG_NET_TFTP_EN                          0                   /*  �Ƿ�ʹ�� tftp ����          */
#if LW_CFG_CPU_WORD_LENGHT == 32
#define LW_CFG_NET_TFTP_STK_SIZE                    (8 * LW_CFG_KB_SIZE)/*  tftp �̶߳�ջ               */
#else
#define LW_CFG_NET_TFTP_STK_SIZE                   (16 * LW_CFG_KB_SIZE)/*  tftp �̶߳�ջ               */
#endif

/*********************************************************************************************************
                                            ftp (TCP : 21)
*  �����ϵ: 1: ����
             2: tcp
*  ע  ��  : ��Ϊ ftp ��Ϊ����, ��������Խϴ�, ��������ֿ��ü�.
*********************************************************************************************************/

#define LW_CFG_NET_FTPD_EN                          1                   /*  �Ƿ�ʹ�� ftp ������         */
#define LW_CFG_NET_FTPD_LOG_EN                      0                   /*  �Ƿ�ʹ�� ftp ��������ӡ log */
#define LW_CFG_NET_FTPD_LOGIN_EN                    1                   /*  �Ƿ�ʹ���û���½            */
#if LW_CFG_CPU_WORD_LENGHT == 32
#define LW_CFG_NET_FTPD_STK_SIZE                    (12 * LW_CFG_KB_SIZE)
#else
#define LW_CFG_NET_FTPD_STK_SIZE                    (24 * LW_CFG_KB_SIZE)
#endif
                                                                        /*  ftp �̶߳�ջ                */
#define LW_CFG_NET_FTPD_MAX_LINKS                   10                  /*  ftp ���ͻ���������        */
#define LW_CFG_NET_FTPD_AUTO_SYNC                   0                   /*  ÿ���ļ�д��֤ sync (Ч�ʵ�)*/
#define LW_CFG_NET_FTPC_EN                          1                   /*  �Ƿ�ʹ�� ftp �ͻ���         */
                                                                        /*  �Ƽ�ʹ�� ncftp �����Ŀͻ��� */
/*********************************************************************************************************
                                            RPC (Remote procedure call)
*  �����ϵ: 1: ����
             2: udp, tcp
*********************************************************************************************************/

#define LW_CFG_NET_RPC_EN                           1                   /*  �Ƿ�ʹ�� RPC ����           */

/*********************************************************************************************************
                                            NAT (�����ַת��, ����:���ò���·����)
*  �����ϵ: 1: ����
             2: ���ڴ����
             3: ��ʱ��
             4: LW_CFG_NET_ROUTER
*********************************************************************************************************/

#define LW_CFG_NET_NAT_EN                           1                   /*  �Ƿ�ʹ�� NAT ����           */
#define LW_CFG_NET_NAT_MAX_AP_IF                    5                   /*  NAT �����ӿ�����            */
#define LW_CFG_NET_NAT_MAX_LOCAL_IF                 5                   /*  NAT �����ӿ�����            */
#define LW_CFG_NET_NAT_MAX_SESSION                  2048                /*  NAT ���Ự���� 256-24576  */
#define LW_CFG_NET_NAT_IDLE_TIMEOUT                 10                  /*  NAT �������ӳ�ʱ, ��λ:���� */

/*********************************************************************************************************
                                            VLAN tools
*  �����ϵ: 1: ����
*********************************************************************************************************/

#define LW_CFG_NET_VLAN_EN                          1                   /*  �Ƿ�ʹ�� VLAN ����          */

/*********************************************************************************************************
                                            VPN (KidVPN ����ר������)
*  �����ϵ: 1: ����
             2: udp
*********************************************************************************************************/

#define LW_CFG_NET_VPN_EN                           1
#define LW_CFG_NET_KIDVPN_EN                        LW_CFG_NET_VPN_EN   /*  �Ƿ�ʹ�� KidVPN ����        */
#define LW_CFG_NET_VNETDEV_EN                       LW_CFG_NET_VPN_EN   /*  ��������                    */

/*********************************************************************************************************
                                            NETDEV MIP (����ӿڶ� IP ����)
*  �����ϵ: 1: ����
*********************************************************************************************************/

#define LW_CFG_NET_NETDEV_MIP_EN                    1                   /*  �Ƿ�ʹ������ӿڶ� IP ����  */

/*********************************************************************************************************
                                            QoS TOOL (���ڹ���� QoS ����)
*  �����ϵ: 1: ����
*********************************************************************************************************/

#define LW_CFG_NET_QOS_EN                           1                   /*  �Ƿ�ʹ�� QoS ���ù���       */

/*********************************************************************************************************
                                            NPF (���ڹ�����������ݱ�������)
*  �����ϵ: 1: ����
             2: udp, tcp
*********************************************************************************************************/

#define LW_CFG_NET_NPF_EN                           1                   /*  �Ƿ�ʹ�� NPF ����           */

/*********************************************************************************************************
                                            SNTP (������ʱ��Э��)
*  �����ϵ: 1: ����
             2: udp
*********************************************************************************************************/

#define LW_CFG_NET_SNTP_EN                          1                   /*  �Ƿ�ʹ�� SNTP ����          */

/*********************************************************************************************************
                                            SNMP (���������Э��)
*  �����ϵ: 1: ����
             2: V3 ���� mbedTLS
             3: udp
*********************************************************************************************************/

#define LW_CFG_NET_SNMP_V1_EN                       0                   /*  �Ƿ�ʹ�� SNMPv1 ����        */
#define LW_CFG_NET_SNMP_V2_EN                       0                   /*  �Ƿ�ʹ�� SNMPv2 ����        */
#define LW_CFG_NET_SNMP_V3_EN                       1                   /*  �Ƿ�ʹ�� SNMPv3 ����        */
                                                                        /*  ������ mbedTLS ����½����� */
                                                                        /*  ���м�����֤���ܵ� SNMPv3   */
/*********************************************************************************************************
                                            LOGINBL (�����¼������)
*  �����ϵ: 1: ����
*********************************************************************************************************/

#define LW_CFG_NET_LOGINBL_EN                       1                   /*  �����¼����������          */
#define LW_CFG_NET_LOGINBL_MAX_NODE                 1000                /*  �������洢������          */

/*********************************************************************************************************
                                            PPP (���Ź���)
*  �����ϵ: 1: ����
*********************************************************************************************************/

#define LW_CFG_NET_PPP_TIMEOUT                      6                   /*  ���γ��Գ�ʱʱ�� (s)        */
#define LW_CFG_NET_PPP_RETRANSMITS                  8                   /*  ������Դ���                */
#define LW_CFG_NET_PPP_ECHOINTERVAL                 10                  /*  Keepalive Echo ���ʱ��     */

#endif                                                                  /*  __NET_TOOLS_CFG_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/