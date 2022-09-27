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
** 文   件   名: devtree_inline.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 06 月 24 日
**
** 描        述: 设备树接口系统内联接口实现
*********************************************************************************************************/

#ifndef __DEVTREE_INLINE_H
#define __DEVTREE_INLINE_H

#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "devtree.h"
#include "linux/bitops.h"

/*********************************************************************************************************
  将对应地址内的整型值转换为主机序
*********************************************************************************************************/

#define BE32_TO_CPU(puiVal)                 (be32toh(*(UINT32 *)(puiVal)))

/*********************************************************************************************************
  获取父节点的每一个子节点
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
  遍历每一个节点
*********************************************************************************************************/

#define _LIST_EACH_OF_ALLNODES_FROM(pdtnFrom, pdtnDev)                      \
    for (pdtnDev  = API_DeviceTreeFindAllNodes(pdtnFrom);                   \
         pdtnDev != LW_NULL;                                                \
         pdtnDev  = API_DeviceTreeFindAllNodes(pdtnDev))

#define _LIST_EACH_OF_ALLNODES(dn)  _LIST_EACH_OF_ALLNODES_FROM(LW_NULL, dn)

/*********************************************************************************************************
  遍历每一个 PHANDLE
*********************************************************************************************************/

#define _LIST_EACH_PHANDLE(it, err, np, ln, cn, cc)                          \
    for (API_DeviceTreePhandleIteratorInit((it), (np), (ln), (cn), (cc)),    \
         err  = API_DeviceTreePhandleIteratorNext(it);                       \
         err == 0;                                                           \
         err  = API_DeviceTreePhandleIteratorNext(it))

/*********************************************************************************************************
** 函数名称: __deviceTreeNodeFlagSet
** 功能描述: 设置设备树节点的标志
** 输　入  : pdtnDev     设备树节点
**           ulFlag      设置的设备树节点标志
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE  VOID  __deviceTreeNodeFlagSet (PLW_DEVTREE_NODE  pdtnDev, ULONG  ulFlag)
{
    __set_bit(ulFlag, &(pdtnDev->DTN_ulFlags));
}
/*********************************************************************************************************
** 函数名称: __deviceTreeNodeFlagClear
** 功能描述: 清除设备树节点的标志
** 输　入  : pdtnDev     设备树节点
**           ulFlag      清除的设备树节点标志
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE  VOID  __deviceTreeNodeFlagClear (PLW_DEVTREE_NODE  pdtnDev, ULONG  ulFlag)
{
    __clear_bit(ulFlag, &(pdtnDev->DTN_ulFlags));
}
/*********************************************************************************************************
** 函数名称: __deviceTreeNodeFlagCheck
** 功能描述: 检查设备树节点的某个标志是否设置
** 输　入  : pdtnDev    设备树节点
**           ulFlag     检查的设备树节点标志
** 输　出  : 非零值表示有设置
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE  INT  __deviceTreeNodeFlagCheck (PLW_DEVTREE_NODE  pdtnDev, ULONG  ulFlag)
{
    return  (__test_bit(ulFlag, &(pdtnDev->DTN_ulFlags)));
}
/*********************************************************************************************************
** 函数名称: __deviceTreeNodeFullName
** 功能描述: 获取设备树节点的全路径
** 输　入  : pdtnDev     设备树节点
** 输　出  : 设备树节点的全路径
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE  CPCHAR  __deviceTreeNodeFullName (const PLW_DEVTREE_NODE  pdtnDev)
{
    return  (pdtnDev ? pdtnDev->DTN_pcFullName : "<no-node>");
}
/*********************************************************************************************************
** 函数名称: __deviceTreeFindNodeByPath
** 功能描述: 通过路径查找设备树节点
** 输　入  : pcPath   设备树节点路径
** 输　出  : 设备树节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE  PLW_DEVTREE_NODE  __deviceTreeFindNodeByPath (CPCHAR  pcPath)
{
    return  (API_DeviceTreeFindNodeOptsByPath(pcPath, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: __deviceTreeNumberRead
** 功能描述: 读取 cells 中的值
** 输　入  : puiCell   cells 属性的基地址
**           iSize     cells 的 size 大小
** 输　出  : cells 中的值
** 全局变量:
** 调用模块:
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
** 函数名称: __deviceTreeBaseNameGet
** 功能描述: 获取节点名称
** 输　入  : pcPath          节点的全路径
** 输　出  : 节点名称
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE  CPCHAR  __deviceTreeBaseNameGet (CPCHAR  pcPath)
{
    CPCHAR  pcTail = lib_strrchr(pcPath, '/');

    return  (pcTail ? (pcTail + 1) : pcPath);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeChildCountGet
** 功能描述: 获取节点名称
** 输　入  : pdtnDev          设备树节点
** 输　出  : 子节点数
** 全局变量:
** 调用模块:
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
