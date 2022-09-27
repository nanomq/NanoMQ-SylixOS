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
** 文   件   名: devtreeProperty.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 07 月 30 日
**
** 描        述: 设备树属性读取接口
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "driver/fdt/libfdt_env.h"
#include "driver/fdt/libfdt.h"
#include "driver/fdt/fdt.h"
/*********************************************************************************************************
** 函数名称: __deviceTreePropertyValueOfSize
** 功能描述: 在设备节点中搜索属性，获取属性的有效大小，并读取属性的值
** 输　入  : pdtnDev        用于查找的设备树节点
**           pcPropname     属性名称
**           uiMin          属性值长度最小有效值
**           uiMax          属性值长度最大有效值
**           pstLen         属性值实际有效的长度
** 输　出  : ERROR_CODE 或 属性值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __deviceTreePropertyValueOfSize (const PLW_DEVTREE_NODE  pdtnDev,
                                               CPCHAR                  pcPropname,
                                               UINT32                  uiMin,
                                               UINT32                  uiMax,
                                               size_t                 *pstLen)
{
    PLW_DEVTREE_PROPERTY  pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcPropname, LW_NULL);

    if (!pdtprop) {
        return  (PVOID)(-EINVAL);
    }

    if (!pdtprop->DTP_pvValue) {
        return  (PVOID)(-ENODATA);
    }

    if (pdtprop->DTP_iLength < uiMin) {
        return  (PVOID)(-EOVERFLOW);
    }

    if (uiMax && pdtprop->DTP_iLength > uiMax) {
        return  (PVOID)(-EOVERFLOW);
    }

    if (pstLen) {
        *pstLen = pdtprop->DTP_iLength;
    }

    return  (pdtprop->DTP_pvValue);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeNodeIsOkay
** 功能描述: 查看某个设备树节点是否为 okay 状态
** 输　入  : pdtnDev          设备树节点
** 输　出  : LW_TRUE 表示为 okay，LW_FALSE 表示不为 okay
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
BOOL  API_DeviceTreeNodeIsOkay (PLW_DEVTREE_NODE  pdtnDev)
{
    CPCHAR  pcStatus;
    INT     iStatLen;

    if (!pdtnDev) {
        return  (LW_FALSE);
    }

    pcStatus = API_DeviceTreePropertyGet(pdtnDev, "status", &iStatLen);
    if (!pcStatus) {                                                    /*  没有 status 属性的都为 okay */
        return  (LW_TRUE);
    }

    if (iStatLen > 0) {
        if (!strcmp(pcStatus, "ok") || !strcmp(pcStatus, "okay")) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeNodeIsOkayByOffset
** 功能描述: 通过 offset 指定查看某个设备树节点是否为 okay 状态
** 输　入  : pvDevTree       设备树基地址
**           iOffset         节点偏移
** 输　出  : LW_TRUE 表示为 okay，LW_FALSE 表示不为 okay
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
BOOL  API_DeviceTreeNodeIsOkayByOffset (PVOID  pvDevTree, INT  iOffset)
{
    CPCHAR  pcStatus;

    if (!pvDevTree) {
        _ErrorHandle(EINVAL);
        return  (LW_FALSE);
    }

    pcStatus = fdt_getprop(pvDevTree, iOffset, "status", LW_NULL);
    if (!pcStatus) {                                                    /*  没有 status 属性的都为 okay */
        return  (LW_TRUE);
    }

    if (!strcmp(pcStatus, "ok") || !strcmp(pcStatus, "okay")) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyFind
** 功能描述: 查找属性节点
** 输　入  : pdtnDev        设备树节点
**           pcPropname     属性名称
**           piLen          获取的属性值长度
** 输　出  : 属性节点 或 LW_NULL
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_DEVTREE_PROPERTY  API_DeviceTreePropertyFind (PLW_DEVTREE_NODE  pdtnDev,
                                                  CPCHAR            pcPropname,
                                                  INT              *piLen)
{
    PLW_DEVTREE_PROPERTY  pdtprop;

    if (!pdtnDev) {
        return  (LW_NULL);
    }

    for (pdtprop = pdtnDev->DTN_pdtpproperties;
         pdtprop;
         pdtprop = pdtprop->DTP_pdtpNext) {                             /*  遍历节点的属性结构          */

        if (lib_strcmp(pdtprop->DTP_pcName, pcPropname) == 0) {
            if (piLen) {
                *piLen = pdtprop->DTP_iLength;
            }
            break;
        }
    }

    return  (pdtprop);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyU32VaraiableArrayRead
** 功能描述: 读取 U32 Array 类型的属性值
** 输　入  : pdtnDev        设备树节点
**           pcPropname     属性名称
**           puiOutValue    读出的数据
**           stMin          属性值长度最小有效值
**           stMax          属性值长度最大有效值
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyU32VaraiableArrayRead (const PLW_DEVTREE_NODE  pdtnDev,
                                                        CPCHAR            pcPropname,
                                                        UINT32           *puiOutValue,
                                                        size_t            stMin,
                                                        size_t            stMax)
{
    size_t         stSize;
    size_t         stCount;
    const UINT32  *puiVal = __deviceTreePropertyValueOfSize(pdtnDev,
                                                            pcPropname,
                                                            (stMin * sizeof(UINT32)),
                                                            (stMax * sizeof(UINT32)),
                                                            &stSize);

    if ((ULONG)puiVal >= (ULONG)-ERRMAX) {
        return  (LONG)(puiVal);
    }

    if (!stMax) {
        stSize = stMin;
    } else {
        stSize /= sizeof(UINT32);
    }

    stCount = stSize;
    while (stCount--) {
        *puiOutValue++ = BE32_TO_CPU(puiVal++);
    }

    return  (stSize);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyU32ArrayRead
** 功能描述: 读取 U32 Array 类型的属性值
** 输　入  : pdtnDev        设备树节点
**           pcPropname     属性名称
**           puiOutValue    读出的数据
**           stSize         Array 的 size
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyU32ArrayRead (PLW_DEVTREE_NODE  pdtnDev,
                                         CPCHAR            pcPropname,
                                         UINT32           *puiOutValue,
                                         size_t            stSize)
{
    INT  iRet = API_DeviceTreePropertyU32VaraiableArrayRead(pdtnDev,
                                                            pcPropname,
                                                            puiOutValue,
                                                            stSize,
                                                            0);
    if (iRet >= 0) {
        return  (ERROR_NONE);
    } else {
        return  (iRet);
    }
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyU32Read
** 功能描述: 读取 U32 类型的属性值
** 输　入  : pdtnDev        设备树节点
**           pcPropname     属性名称
**           puiOutValue    读出的数据
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyU32Read (PLW_DEVTREE_NODE  pdtnDev,
                                    CPCHAR            pcPropname,
                                    UINT32           *puiOutValue)
{
    return  (API_DeviceTreePropertyU32ArrayRead(pdtnDev, pcPropname, puiOutValue, 1));
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyU32IndexRead
** 功能描述: 按序号读取 U32 类型的属性值
** 输　入  : pdtnDev        设备树节点
**           pcPropname     属性名称
**           uiIndex        指定的序号
**           puiOutValue    读出的数据
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyU32IndexRead (PLW_DEVTREE_NODE  pdtnDev,
                                         CPCHAR            pcPropname,
                                         UINT32            uiIndex,
                                         UINT32           *puiOutValue)
{
    const UINT32  *puiVal = __deviceTreePropertyValueOfSize(pdtnDev,
                                                            pcPropname,
                                                            ((uiIndex + 1) * sizeof(UINT32)),
                                                            0,
                                                            LW_NULL);

    if ((ULONG)puiVal >= (ULONG)-ERRMAX) {
        return  (INT)(ULONG)(puiVal);
    }

    *puiOutValue = BE32_TO_CPU(((UINT32 *)puiVal) + uiIndex);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyU32Next
** 功能描述: 按顺序读取 U32 类型的下一个属性值
** 输　入  : pdtprop        当前设备树属性节点
**           puiCur         当前属性地址
**           puiOut         读出的数据
** 输　出  : 更新后的属性地址
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
UINT32*  API_DeviceTreePropertyU32Next (PLW_DEVTREE_PROPERTY  pdtprop,
                                        UINT32               *puiCur,
                                        UINT32               *puiOut)
{
    PVOID  pvCur = puiCur;

    if (!pdtprop) {
        return  (LW_NULL);
    }

    if (!puiCur) {
        puiCur = pdtprop->DTP_pvValue;
        goto  __out_val;
    }

    pvCur += sizeof(PVOID);
    if (pvCur >= (pdtprop->DTP_pvValue + pdtprop->DTP_iLength)) {
        return  (LW_NULL);
    }

__out_val:
    *puiOut = BE32_TO_CPU(puiCur);

    return  (pvCur);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyStringHelperRead
** 功能描述: 解析 string 类型的属性值
** 输　入  : pdtnDev        用于获取属性的设备树节点
**           pcPropname     查找的属性名称
**           ppcOutStrs     输出的字符串指针数组
**           stSize         读取的数组元素数量
**           iSkip          开头跳过的字符串数量
**           piCount        获取 string 类型属性长度
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块: 不要直接调用该接口，而是通过 API_DeviceTreePropertyStringRead* 这样的接口来间接调用该接口
**
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyStringHelperRead (PLW_DEVTREE_NODE  pdtnDev,
                                             CPCHAR            pcPropName,
                                             CPCHAR           *ppcOutStrs,
                                             size_t            stSize,
                                             INT               iSkip,
                                             INT              *piCount)
{
    PLW_DEVTREE_PROPERTY  pdtprop;
    INT                   l;
    INT                   i;
    CPCHAR                pcTmp;
    CPCHAR                pcEnd;

    pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcPropName, LW_NULL);
    if (!pdtprop) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pdtprop->DTP_pvValue) {
        _ErrorHandle(ENODATA);
        return  (PX_ERROR);
    }

    pcTmp = pdtprop->DTP_pvValue;
    pcEnd = pcTmp + pdtprop->DTP_iLength;

    for (i = 0;
         (pcTmp < pcEnd) && (!ppcOutStrs || (i < iSkip + stSize));
         i++, pcTmp += l) {

        l = lib_strnlen(pcTmp, pcEnd - pcTmp) + 1;
        if ((pcTmp + l) > pcEnd) {
            _ErrorHandle(EILSEQ);
            return  (PX_ERROR);
        }

        if (ppcOutStrs && (i >= iSkip)) {
            *ppcOutStrs++ = pcTmp;
        }
    }

    i -= iSkip;

    if (i <= 0) {
        _ErrorHandle(ENODATA);
        return  (PX_ERROR);
    }

    if (piCount) {
        *piCount = i;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyStringIndexRead
** 功能描述: 读取指定序号的 String 类型的属性值
** 输　入  : pdtnDev        设备树节点
**           pcPropName     属性名称
**           iIndex         指定的序号
**           ppcOutPut      读取出的属性字符串
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyStringIndexRead (const PLW_DEVTREE_NODE  pdtnDev,
                                            CPCHAR                  pcPropName,
                                            INT                     iIndex,
                                            CPCHAR                 *ppcOutPut)
{
    return  (API_DeviceTreePropertyStringHelperRead(pdtnDev, pcPropName, ppcOutPut, 1, iIndex, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyStringCount
** 功能描述: 从一个由多条 String 类型组成的属性值中获取 String 的个数
** 输　入  : pdtnDev         设备树节点
**           pcPropName      属性名称
** 输　出  : 获取的 String 个数
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyStringCount (const PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcPropName)
{
    INT  iCount;
    INT  iRet;

    iRet = API_DeviceTreePropertyStringHelperRead(pdtnDev, pcPropName, LW_NULL, 0, 0, &iCount);
    if (iRet) {
        return  (0);
    } else {
        return  (iCount);
    }
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyStringNext
** 功能描述: 读取下一个类型为  String  的属性值
** 输　入  : pdtprop        属性节点
**           pcCur          当前 String 类型属性指针，为 NULL 时获得当前 String 属性值
** 输　出  : 读取出的 String 属性
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
CPCHAR  API_DeviceTreePropertyStringNext (PLW_DEVTREE_PROPERTY  pdtprop, CPCHAR  pcCur)
{
    CPVOID  pvCur = pcCur;

    if (!pdtprop) {
        return  (LW_NULL);
    }

    if (!pcCur) {                                                       /*  为空时，返回当前属性值      */
        return  (pdtprop->DTP_pvValue);
    }

    pvCur += lib_strlen(pcCur) + 1;
    if (pvCur >= (pdtprop->DTP_pvValue + pdtprop->DTP_iLength)) {
        return  (LW_NULL);
    }

    return  (pvCur);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyStringRead
** 功能描述: 读取 String 类型的属性值
** 输　入  : pdtnDev        设备树节点
**           pcPropname     属性名称
**           ppcOutString   读取出的 String 属性
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyStringRead (PLW_DEVTREE_NODE  pdtnDev,
                                       CPCHAR            pcPropName,
                                       CPCHAR           *ppcOutString)
{
    const PLW_DEVTREE_PROPERTY  pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcPropName, LW_NULL);

    if (!pdtprop) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pdtprop->DTP_pvValue) {
        _ErrorHandle(ENODATA);
        return  (PX_ERROR);
    }

    if (lib_strnlen(pdtprop->DTP_pvValue, pdtprop->DTP_iLength) >= pdtprop->DTP_iLength) {
        _ErrorHandle(EILSEQ);
        return  (PX_ERROR);
    }

    *ppcOutString = pdtprop->DTP_pvValue;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyStringMatch
** 功能描述: 属性值与指定字符串比较函数
** 输　入  : pdtnDev        设备树节点
**           pcPropname     属性名称
**           pcString       指定字符串
** 输　出  : PX_ERROR 或 属性序号
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyStringMatch (PLW_DEVTREE_NODE  pdtnDev,
                                        CPCHAR            pcPropName,
                                        CPCHAR            pcString)
{
    PLW_DEVTREE_PROPERTY  pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcPropName, LW_NULL);
    PCHAR                 pcPropTmp;
    PCHAR                 pcPropEnd;
    size_t                stLen;
    INT                   i;

    if (!pdtprop) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pdtprop->DTP_pvValue) {
        _ErrorHandle(ENODATA);
        return  (PX_ERROR);
    }

    pcPropTmp = pdtprop->DTP_pvValue;
    pcPropEnd = pcPropTmp + pdtprop->DTP_iLength;

    for (i = 0; pcPropTmp < pcPropEnd; i++, pcPropTmp += stLen) {
        stLen = lib_strnlen(pcPropTmp, pcPropEnd - pcPropTmp) + 1;
        if ((pcPropTmp + stLen) > pcPropEnd) {
            _ErrorHandle(EILSEQ);
            return  (PX_ERROR);
        }

        if (lib_strcmp(pcString, pcPropTmp) == 0) {
            return  (i);
        }
    }

    _ErrorHandle(ENODATA);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyBoolRead
** 功能描述: 读取 BOOL 类型的属性值
** 输　入  : pdtnDev        设备树节点
**           pcPropName     属性名称
** 输　出  : LW_TRUE 或 LW_FALSE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
BOOL  API_DeviceTreePropertyBoolRead (PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcPropName)
{
    PLW_DEVTREE_PROPERTY  pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcPropName, LW_NULL);

    if (pdtprop) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePropertyGet
** 功能描述: 通过指定节点的指定属性名，获取一个属性值
** 输　入  : pdtnDev        用于获取属性的设备树节点
**           pcName         查找的属性名称
**           piLen          属性的长度
** 输　出  : 属性值 或 LW_NULL
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PVOID  API_DeviceTreePropertyGet (PLW_DEVTREE_NODE  pdtnDev,
                                  CPCHAR            pcName,
                                  INT              *piLen)
{
    PLW_DEVTREE_PROPERTY  pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcName, piLen);

    if (pdtprop) {
        return  (pdtprop->DTP_pvValue);
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeModaliasGet
** 功能描述: 为设备节点找到一个合适的名称
** 输　入  : pdtnDev        用于获取属性的设备树节点
**           pcName         存放属性名的内存指针
**           iLen           存放属性名的内存大小
** 输　出  : 找到返回 ERROR_NONE, 未找到返回其他
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeModaliasGet (PLW_DEVTREE_NODE  pdtnDev,
                                PCHAR             pcName,
                                INT               iLen)
{
    CPCHAR  pcCompatible;
    CPCHAR  pcStart;
    INT     iCpLen;
    
    pcCompatible = API_DeviceTreePropertyGet(pdtnDev, "compatible", &iCpLen);
    if (!pcCompatible || (lib_strlen(pcCompatible) > iCpLen)) {
        return  (ENODEV);
    }

    pcStart = lib_strchr(pcCompatible, ',');
    lib_strlcpy(pcName, pcStart ? (pcStart + 1) : pcCompatible, iLen);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
