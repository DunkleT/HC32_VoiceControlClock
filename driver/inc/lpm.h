/******************************************************************************
* Copyright (C) 2017, Xiaohua Semiconductor Co.,Ltd All rights reserved.
*
* This software is owned and published by:
* Xiaohua Semiconductor Co.,Ltd ("XHSC").
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software contains source code for use with XHSC
* components. This software is licensed by XHSC to be adapted only
* for use in systems utilizing XHSC components. XHSC shall not be
* responsible for misuse or illegal use of this software for devices not
* supported herein. XHSC is providing this software "AS IS" and will
* not be responsible for issues arising from incorrect user implementation
* of the software.
*
* Disclaimer:
* XHSC MAKES NO WARRANTY, EXPRESS OR IMPLIED, ARISING BY LAW OR OTHERWISE,
* REGARDING THE SOFTWARE (INCLUDING ANY ACOOMPANYING WRITTEN MATERIALS),
* ITS PERFORMANCE OR SUITABILITY FOR YOUR INTENDED USE, INCLUDING,
* WITHOUT LIMITATION, THE IMPLIED WARRANTY OF MERCHANTABILITY, THE IMPLIED
* WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE OR USE, AND THE IMPLIED
* WARRANTY OF NONINFRINGEMENT.
* XHSC SHALL HAVE NO LIABILITY (WHETHER IN CONTRACT, WARRANTY, TORT,
* NEGLIGENCE OR OTHERWISE) FOR ANY DAMAGES WHATSOEVER (INCLUDING, WITHOUT
* LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION,
* LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS) ARISING FROM USE OR
* INABILITY TO USE THE SOFTWARE, INCLUDING, WITHOUT LIMITATION, ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOSS OF DATA,
* SAVINGS OR PROFITS,
* EVEN IF Disclaimer HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* YOU ASSUME ALL RESPONSIBILITIES FOR SELECTION OF THE SOFTWARE TO ACHIEVE YOUR
* INTENDED RESULTS, AND FOR THE INSTALLATION OF, USE OF, AND RESULTS OBTAINED
* FROM, THE SOFTWARE.
*
* This software may be replicated in part or whole for the licensed use,
* with the restriction that this Disclaimer and Copyright notice must be
* included with each copy of this software, whether used in part or whole,
* at all times.
*/
/*****************************************************************************/
/** \file lpm.h
 **
 ** Lpm 数据结构及API声明
 **  
 **
 ** History:
 **   - 2017-06-06   Lux     V1.0
 **
 *****************************************************************************/

#ifndef __LPM_H__
#define __LPM_H__

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 *******************************************************************************
 ** \defgroup LpmGroup Low Power Management (LPM)
 **
 **
 ******************************************************************************/
//@{
    
/******************************************************************************
 ** Global pre-processor symbols/macros ('#define')                            
 ******************************************************************************/

/******************************************************************************
 * Global type definitions
 ******************************************************************************/

/******************************************************************************
 * Global variable declarations ('extern', definition in C source)
 *****************************************************************************/

/******************************************************************************
 * Global function prototypes (definition in C source)
 *****************************************************************************/
///<功能配置及操作函数
///<进入普通休眠模式
void Lpm_GotoSleep(boolean_t bOnExit);
///<进入深度休眠模式
void Lpm_GotoDeepSleep(boolean_t bOnExit);

//@} // LpmGroup

#ifdef __cplusplus
#endif


#endif /* __LPM_H__ */
/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/


