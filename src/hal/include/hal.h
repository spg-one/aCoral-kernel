/**
 * @file hal.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief hal层，顶层头文件
 * @version 1.0
 * @date 2024-03-21
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-06-24 <td>Standardized 
 *   <tr><td> 2.0 <td>王彬浩 <td> 2024-03-21 <td>加入CFG_K210
 *  </table>
 */

#ifndef HAL_H_
#define HAL_H_
#include "autocfg.h"

/* 使用K210 SOC */
#if CFG_SOC == SOC_K210
#include "../K210/include/hal_int.h"
#include "../K210/include/hal_thread.h"
#include "../K210/include/hal_timer.h"

///RISC-V要求sp寄存器16字节对齐
#define hal_sp_align 16 

#elif CFG_SOC == SOC_S3C2440 
#endif



#endif /* HAL_H_ */
