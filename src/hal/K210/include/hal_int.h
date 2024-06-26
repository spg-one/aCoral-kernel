/**
 * @file hal_int.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief hal层，中断相关头文件
 * @version 1.1
 * @date 2023-04-20
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-17 <td>Standardized 
 *   <tr><td> 1.1 <td>王彬浩 <td> 2023-04-20 <td>riscv
 *  </table>
 */
#ifndef HAL_INT_H
#define HAL_INT_H

#include "encoding.h"

#define HAL_INTR_ENABLE()     hal_intr_enable()
#define HAL_INTR_DISABLE()    hal_intr_disable()

extern int acoral_intr_nesting;

void hal_intr_init();

/**
 * @brief 开启全局中断
 * 
 */
void hal_intr_enable();

/**
 * @brief 关闭全局中断
 * 
 */
void hal_intr_disable();

/**
 * @brief 使能中断。通过向中断屏蔽（INTMSK）寄存器某位写入0来打开相应中断，对中断复用进行了合并处理
 *
 * @param vector 中断向量号（中断复用展开后）
 */
void hal_intr_unmask(int vector);

/**
 * @brief 除能中断。通过向中断屏蔽（INTMSK）寄存器某位写入1来屏蔽相应中断，对中断复用进行了合并处理
 *
 * @param vector 中断向量号（中断复用展开后）
 */
void hal_intr_mask(int vector);

void hal_intr_ack(unsigned int vector);

/**
 * @brief 获取系统当前中断嵌套数
 *
 * @return unsigned int 中断嵌套数
 */
unsigned int hal_get_intr_nesting_comm();

/**
 * @brief 减少系统当前中断嵌套数
 *
 */
void hal_intr_nesting_dec_comm();

/**
 * @brief 增加系统当前中断嵌套数
 *
 */
void hal_intr_nesting_inc_comm();

/**
 * @brief 保证调度的原子性
 *
 */
void hal_sched_bridge_comm();

/**
 * @brief 保证调度（中断引起）的原子性
 *
 */
unsigned long hal_intr_exit_bridge_comm(unsigned long old_sp);

/****************************                                                                                                                 
* the comm interrupt interface of hal     
*  hal层中断部分通用接口
*****************************/

#define HAL_INTR_NESTING_DEC()    hal_intr_nesting_dec_comm()
#define HAL_INTR_NESTING_INC()    hal_intr_nesting_inc_comm()

#define HAL_INTR_ATTACH(vecotr,isr) //TODO 该写什么？
#define HAL_SCHED_BRIDGE() hal_sched_bridge_comm() //SPGcommon指的是老版本的acoral中，有stm32的版本，但是stm32的调度被放在pendsv中，比较特殊，所有这里封装了一层接口，除了stm32其他的实现称为common
#define HAL_INTR_EXIT_BRIDGE(sp) hal_intr_exit_bridge_comm(sp)

#define HAL_ENTER_CRITICAL() hal_enter_critical()
#define HAL_EXIT_CRITICAL() hal_exit_critical()


#endif