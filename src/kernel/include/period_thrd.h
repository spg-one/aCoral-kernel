/**
 * @file period_thrd.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，周期策略头文件
 * @version 1.0
 * @date 2023-04-20
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-08 <td>Standardized 
 *   <tr><td> 1.1 <td>王彬浩 <td> 2023-04-20 <td>optimized 
 *  </table>
 */
#ifndef PERIOD_THRD_H
#define PERIOD_THRD_H

#include "thread.h"

/**
 * @brief 周期策略数据块
 * 
 */
typedef struct{
	unsigned int period_time_mm; 			///<线程周期，单位为毫秒
}acoral_period_policy_data_t;

void period_thread_exit(void);
void period_thread_delay(acoral_thread_t* thread,unsigned int time);
void period_delay_deal(void);

void period_policy_init(void);
#endif
