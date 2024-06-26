/**
 * @file timer.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，定时器
 * @version 1.0
 * @date 2023-04-21
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2023-04-21 <td>Standardized
 *  </table>
 */

/*---------------*/
/*  增加timeout队列 g_timeout_queue*/
/*  pegasus   0719*/
/*---------------*/
/*  增加acoral_timeout_queue_add 函数*/
/*  pegasus   0719*/
/*---------------*/

#include "hal.h"
#include "policy.h"
#include "comm_thrd.h"
#include "soft_timer.h"
#include "int.h"
#include "thread.h"
#include "log.h"
#include "list.h"
#include <stdbool.h>

/*----------------*/
/*  延时处理队列timeout*/
/*  pegasus   0719*/
/*----------------*/
static unsigned int ticks;

int time_to_ticks(unsigned int mtime){
	return (mtime)*CFG_TICKS_PER_SEC/1000; ///<计算time对应的ticks数量
}

unsigned int acoral_get_ticks(){
	return ticks;
}

void acoral_set_ticks(unsigned int time){
  	ticks=time;
}

void acoral_ticks_entry(){
	ticks++;
	time_delay_deal();
	acoral_policy_delay_deal();
	/*--------------------*/
	/* 超时链表处理函数*/
	/* pegasus  0719*/
	/*--------------------*/
	timeout_delay_deal();
}

int system_ticks_init(){
	ticks=0;    	/*初始化滴答时钟计数器*/
	return hal_timer_init(CFG_TICKS_PER_SEC, acoral_ticks_entry, NULL);
}


void acoral_delayqueue_add(acoral_list_t *queue, acoral_thread_t *new){
	acoral_list_t   *tmp, *head;
	acoral_thread_t *thread;
	int  delay2;
	int  delay= new->thread_timer->delay_time;
	head=queue;
	acoral_enter_critical();
	/*这里采用关ticks中断，不用关中断，是为了减少最大关中断时间，下面是个链表，时间不确定。*/
	/*这里可以看出，延时函数会有些误差，因为ticks中断可能被延迟*/

	new->state|=ACORAL_THREAD_STATE_DELAY;
	for (tmp=head->next;delay2=delay,tmp!=head; tmp=tmp->next){
		thread = (acoral_thread_t *)acoral_get_res_by_id(((acoral_timer_t*)list_entry(tmp, acoral_timer_t, delay_queue_hook))->owner.id);
		delay  = delay-thread->thread_timer->delay_time;
		if (delay < 0)
			break;
	}
	new->thread_timer->delay_time = delay2;
	acoral_list_add(&new->thread_timer->delay_queue_hook,tmp->prev);
	/* 插入等待任务后，后继等待任务时间处理*/
	if(tmp != head){
		thread = (acoral_thread_t *)acoral_get_res_by_id(((acoral_timer_t*)list_entry(tmp, acoral_timer_t, delay_queue_hook))->owner.id);
		thread->thread_timer->delay_time-=delay2;
	}
	unrdy_thread(new);

	acoral_exit_critical();
	acoral_sched();
	return;
}

void time_delay_deal(){
	acoral_list_t   *tmp,*tmp1,*head;
	acoral_thread_t *thread;
	head = &(((timer_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_TIMER].type_private_data))->global_time_delay_queue);
	if(acoral_list_empty(head))
	  	return;
	thread=(acoral_thread_t *)acoral_get_res_by_id(((acoral_timer_t*)list_entry(head->next, acoral_timer_t, delay_queue_hook))->owner.id);
	thread->thread_timer->delay_time--;
	for(tmp=head->next;tmp!=head;){
		thread=(acoral_thread_t *)acoral_get_res_by_id(((acoral_timer_t*)list_entry(tmp, acoral_timer_t, delay_queue_hook))->owner.id);
		if(thread->thread_timer->delay_time>0)
		    break;
		/*防止add判断delay时取下thread*/

		tmp1=tmp->next;
		acoral_list_del(&thread->thread_timer->delay_queue_hook);

		tmp=tmp1;
		thread->state&=~ACORAL_THREAD_STATE_DELAY;
		ready_thread(thread);
	}
}

void timeout_queue_add(acoral_thread_t *new)
{
	acoral_list_t   *tmp ,*head;
	acoral_thread_t *thread;
	int  delay2;
	int  delay= new->thread_timer->delay_time; //SPG用tcb的delay，delay线程也用delay成员，冲突？
	head = &(((timer_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_TIMER].type_private_data))->global_timeout_queue);
	acoral_enter_critical();

	for (tmp=head->next;delay2=delay,tmp!=head; tmp=tmp->next){
		thread = list_entry (tmp, acoral_thread_t, timeout_hook);
		delay  = delay-thread->thread_timer->delay_time;
		if (delay < 0)
			break;
	}
	new->thread_timer->delay_time = delay2;
	acoral_list_add(&new->timeout_hook,tmp->prev);
	/* 插入等待任务后，后继等待任务时间处理*/
	if(tmp != head){
		thread = list_entry(tmp, acoral_thread_t, timeout_hook);
		thread->thread_timer->delay_time-=delay2;
	}

	acoral_exit_critical();
	return;
}

void timeout_queue_del(acoral_thread_t *new)
{
	if (acoral_list_empty(&new->timeout_hook))
		return;
	acoral_list_del(&new->timeout_hook);
	return;
}

void timeout_delay_deal()
{
	acoral_list_t *tmp, *tmp1, *head;
	acoral_thread_t  *thread;

	head = &(((timer_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_TIMER].type_private_data))->global_timeout_queue);
	if(acoral_list_empty(head))
	{
	  	return;
	}

	thread=list_entry(head->next,acoral_thread_t,timeout_hook);
	if (thread->thread_timer->delay_time>0)
		thread->thread_timer->delay_time--;
	for(tmp=head->next;tmp!=head;)
	{
		thread=list_entry(tmp,acoral_thread_t,timeout_hook);
		if(thread->thread_timer->delay_time>0)
		    break;

		tmp1=tmp->next;
		acoral_list_del(&thread->timeout_hook);

		tmp=tmp1;
		/*thread->state*/
		ready_thread(thread);
	}
}
