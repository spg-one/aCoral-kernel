/**
 * @file core.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，core.c对应的头文件
 * @version 1.1
 * @date 2023-04-19
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-23 <td>Standardized
 *   <tr><td> 1.1 <td>王彬浩 <td> 2023-04-19 <td>use enum
 *  </table>
 */
#ifndef ACORAL_CORE_H
#define ACORAL_CORE_H

volatile extern unsigned int acoral_start_sched;

#define DAEM_STACK_SIZE 256
#define IDLE_STACK_SIZE 128
#define INIT_STACK_SIZE 512

/**
 * @brief aCoral入口
 * 
 */
void acoral_start(void);

#endif
