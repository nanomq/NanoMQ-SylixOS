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
** ��   ��   ��: devtree_inline.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 06 �� 24 ��
**
** ��        ��: �豸���ӿ�ϵͳ�����ӿ�ʵ��
*********************************************************************************************************/

#ifndef __DEVTREE_INLINE_H
#define __DEVTREE_INLINE_H

#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "devtree.h"
#include "linux/bitops.h"

/*********************************************************************************************************
  ����Ӧ��ַ�ڵ�����ֵת��Ϊ������
*********************************************************************************************************/

#define BE32_TO_CPU(puiVal)                 (be32toh(*(UINT32 *)(puiVal)))

/*********************************************************************************************************
  ��ȡ���ڵ��ÿһ���ӽڵ�
*********************************************************************************************************/

#define _LIST_EACH_CHILD_OF_NODE(pdtnParent, pdtnChild)                     \
    for (pdtnChild  = API_DeviceTreeNextChildGet(pdtnParent, LW_NULL);      \
         pdtnChild != LW_NULL;                                              \
         pdtnChild  = API_DeviceTreeNextChildGet(pdtnParent, pdtnChild))

#define _LIST_EACH_OF_PROPERTY(pdtnDev, pcPropName, pdtproperty, pcStr)          \
    for (pdtproperty = API_DeviceTreePropertyFind(pdtnDev, pcPropName, LW_NULL), \
         pcStr  = API_DeviceTreePropertyStringNext(pdtproperty, LW_NULL);        \
         pcStr != LW_NULL;                                                       \
         pcStr  = API_DeviceTreePropertyStringNext(pdtproperty, pcStr))

#define _LIST_EACH_OF_UINT32_PROPERTY(pdtnDev, pcPropName, pdtproperty, puiCur, uiOut)  \
    for (pdtproperty = API_DeviceTreePropertyFind(pdtnDev, pcPropName, NULL),           \
         puiCur  = API_DeviceTreePropertyU32Next(pdtproperty, LW_NULL, &uiOut);         \
         puiCur != LW_NULL;                                                             \
         puiCur  = API_DeviceTreePropertyU32Next(pdtproperty, puiCur, &uiOut))

/*********************************************************************************************************
  ����ÿһ���ڵ�
*********************************************************************************************************/

#define _LIST_EACH_OF_ALLNODES_FROM(pdtnFrom, pdtnDev)                      \
    for (pdtnDev  = API_DeviceTreeFindAllNodes(pdtnFrom);                   \
         pdtnDev != LW_NULL;                                                \
         pdtnDev  = API_DeviceTreeFindAllNodes(pdtnDev))

#define _LIST_EACH_OF_ALLNODES(dn)  _LIST_EACH_OF_ALLNODES_FROM(LW_NULL, dn)

/*********************************************************************************************************
  ����ÿһ�� PHANDLE
*********************************************************************************************************/

#define _LIST_EACH_PHANDLE(it, err, np, ln, cn, cc)                          \
    for (API_DeviceTreePhandleIteratorInit((it), (np), (ln), (cn), (cc)),    \
         err  = API_DeviceTreePhandleIteratorNext(it);                       \
         err == 0;                                                           \
         err  = API_DeviceTreePhandleIteratorNext(it))

/*********************************************************************************************************
** ��������: __deviceTreeNodeFlagSet
** ��������: �����豸���ڵ�ı�־
** �䡡��  : pdtnDev     �豸���ڵ�
**           ulFlag      ���õ��豸���ڵ��־
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE  VOID  __deviceTreeNodeFlagSet (PLW_DEVTREE_NODE  pdtnDev, ULONG  ulFlag)
{
    __set_bit(ulFlag, &(pdtnDev->DTN_ulFlags));
}
/*********************************************************************************************************
** ��������: __deviceTreeNodeFlagClear
** ��������: ����豸���ڵ�ı�־
** �䡡��  : pdtnDev     �豸���ڵ�
**           ulFlag      ������豸���ڵ��־
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE  VOID  __deviceTreeNodeFlagClear (PLW_DEVTREE_NODE  pdtnDev, ULONG  ulFlag)
{
    __clear_bit(ulFlag, &(pdtnDev->DTN_ulFlags));
}
/*********************************************************************************************************
** ��������: __deviceTreeNodeFlagCheck
** ��������: ����豸���ڵ��ĳ����־�Ƿ�����
** �䡡��  : pdtnDev    �豸���ڵ�
**           ulFlag     �����豸���ڵ��־
** �䡡��  : ����ֵ��ʾ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE  INT  __deviceTreeNodeFlagCheck (PLW_DEVTREE_NODE  pdtnDev, ULONG  ulFlag)
{
    return  (__test_bit(ulFlag, &(pdtnDev->DTN_ulFlags)));
}
/*********************************************************************************************************
** ��������: __deviceTreeNodeFullName
** ��������: ��ȡ�豸���ڵ��ȫ·��
** �䡡��  : pdtnDev     �豸���ڵ�
** �䡡��  : �豸���ڵ��ȫ·��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE  CPCHAR  __deviceTreeNodeFullName (const PLW_DEVTREE_NODE  pdtnDev)
{
    return  (pdtnDev ? pdtnDev->DTN_pcFullName : "<no-node>");
}
/*********************************************************************************************************
** ��������: __deviceTreeFindNodeByPath
** ��������: ͨ��·�������豸���ڵ�
** �䡡��  : pcPath   �豸���ڵ�·��
** �䡡��  : �豸���ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE  PLW_DEVTREE_NODE  __deviceTreeFindNodeByPath (CPCHAR  pcPath)
{
    return  (API_DeviceTreeFindNodeOptsByPath(pcPath, LW_NULL));
}
/*********************************************************************************************************
** ��������: __deviceTreeNumberRead
** ��������: ��ȡ cells �е�ֵ
** �䡡��  : puiCell   cells ���ԵĻ���ַ
**           iSize     cells �� size ��С
** �䡡��  : cells �е�ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE  UINT64  __deviceTreeNumberRead (const UINT32  *puiCell, INT  iSize)
{
    UINT64  ullRet = 0;

    while (iSize--) {
        ullRet = (ullRet << 32) | be32toh(*(puiCell++));
    }

    return  (ullRet);
}
/*********************************************************************************************************
** ��������: __deviceTreeBaseNameGet
** ��������: ��ȡ�ڵ�����
** �䡡��  : pcPath          �ڵ��ȫ·��
** �䡡��  : �ڵ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE  CPCHAR  __deviceTreeBaseNameGet (CPCHAR  pcPath)
{
    CPCHAR  pcTail = lib_strrchr(pcPath, '/');

    return  (pcTail ? (pcTail + 1) : pcPath);
}
/*********************************************************************************************************
** ��������: __deviceTreeChildCountGet
** ��������: ��ȡ�ڵ�����
** �䡡��  : pdtnDev          �豸���ڵ�
** �䡡��  : �ӽڵ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE  INT  __deviceTreeChildCountGet (const PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_DEVTREE_NODE    pdtnChild;
    INT                 iCount      = 0;

    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {
        iCount++;
    }

    return  (iCount);
}

#endif                                                                  /*  __DEVTREE_INLINE_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
