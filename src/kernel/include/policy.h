/**
 * @file policy.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，线程调度策略头文件
 * @version 1.1
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
#ifndef POLICY_H
#define POLICY_H

#include "list.h"
#include "resource.h"
// #include "thread.h"
#include <stdbool.h>

typedef struct acoral_thread_tcb acoral_thread_t; 

typedef enum{
	ACORAL_SCHED_POLICY_COMM,
	ACORAL_SCHED_POLICY_PERIOD
}acoralSchedPolicyEnum;

/**
 * @brief 调度策略控制块
 * 
 */
typedef struct{
    acoral_res_t res;
	acoral_list_t list; 	///<用于把各个调度策略串到一个链表上，创建线程找策略的时候就去这个链表上，根据策略名找
	unsigned char type; 		///<策略名
	int (*policy_thread_init)(acoral_thread_t *,void *); ///<某种策略的初始化函数，用于线程创建时调用
	void (*policy_thread_release)(acoral_thread_t *); 	///<某种策略的释放函数，用于消灭线程时调用
	void (*delay_deal)(void); 							///<线程延时函数，用于例如周期、时间片等和时间相关的调度策略
}acoral_sched_policy_t;


typedef struct{
#if CFG_THRD_PERIOD
    acoral_list_t global_period_wait_queue; ///<周期线程专用等待队列，差分队列。只要是周期线程，就会被挂载到这个队列上，延时时间就是周期，每次周期过后重新挂载
#endif
}policy_res_private_data;


void acoral_policy_delay_deal(void);
acoral_sched_policy_t *acoral_get_policy_ctrl(unsigned char type);

/**
 * @brief 根据线程调度策略进行特异初始化的入口
 * 
 * @param policy 调度策略名
 * @param thread 线程指针
 * @param data 线程调度策略专用数据
 * @return int 线程id
 */
int acoral_policy_thread_init(acoralSchedPolicyEnum policy,acoral_thread_t *thread,void *data);
void acoral_sched_policy_init(void);
void system_policy_thread_release(acoral_thread_t *thread);

/***************调度策略相关API****************/

/**
 * @brief 向aCoral中注册新的调度策略
 * 
 * @param policy 调度策略控制块指针
 */
void acoral_register_sched_policy(acoral_sched_policy_t *policy);

#endif
