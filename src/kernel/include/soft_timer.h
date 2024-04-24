/**
 * @file timer.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，定时器相关头文件
 * @version 1.0
 * @date 2022-07-20
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-20 <td>Standardized 
 *  </table>
 */

#ifndef ACORAL_TIMER_H
#define ACORAL_TIMER_H

#include "autocfg.h"
#include "core.h"
#include "thread.h"

/**
 * @brief aCoral软定时器，即以ticks中断为基准的软件实现的定时器
 * 
 */
typedef struct 
{
    acoral_res_t res;
    acoral_list_t delay_queue_hook; ///<timer挂载到目标队列的钩子
    int delay_time;                 ///<timer将要等待的时间
    acoral_res_t owner;             ///<timer持有者，一般为线程
}acoral_timer_t;

typedef struct{
    acoral_list_t global_time_delay_queue; ///<aCoral线程延时队列，调用线程delay相关函数的线程都会被加到这个队列上，等待一段具体时间后重新被唤醒
}timer_res_private_data;

int time_to_ticks(unsigned int time); 
extern acoral_list_t timeout_queue;

void acoral_time_init(void);
int system_ticks_init();
void acoral_ticks_entry();
void time_delay_deal(void);

/**
 * @brief 将线程挂到延时队列上
 * 
 */
void acoral_delayqueue_add(acoral_list_t*, acoral_thread_t*);

/**
 * @brief 超时链表处理函数
 * 
 */
void timeout_delay_deal(void);

/**
 * @brief 将线程挂到超时队列上
 * 
 */
void timeout_queue_add(acoral_thread_t*);

/**
 * @brief 将线程从超时队列删除
 * 
 */
void timeout_queue_del(acoral_thread_t*);

/***************ticks相关API****************/

/**
 * @brief 设置aCoral心跳tick的值
 * 
 * @param time tick新值
 */
void acoral_set_ticks(unsigned int time);

/**
 * @brief 得到tick的值
 * 
 * @return tick的值
 */
unsigned int acoral_get_ticks();

#endif

