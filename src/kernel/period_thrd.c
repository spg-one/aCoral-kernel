/**
 * @file period_thrd.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，周期线程
 * @version 1.0
 * @date 2023-05-08
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td> 2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2023-05-08 <td>Standardized
 *  </table>
 */


#include "thread.h"
#include "hal.h"
#include "policy.h"
#include "mem.h"
#include "soft_timer.h"
#include "period_thrd.h"
#include "int.h"
#include "dag.h"

#include <stdio.h>

#if CFG_THRD_PERIOD

acoral_list_t period_wait_queue; ///<周期线程专用等待队列，差分队列。只要是周期线程，就会被挂载到这个队列上，延时时间就是周期，每次周期过后重新挂载

/**
 * @brief 初始化周期线程的一些数据
 * 
 * @param thread 线程指针
 * @param data 周期线程数据，acoral_period_policy_data_t
 * @return int 
 */
static int period_policy_thread_init(acoral_thread_t *thread,void *data){
	acoral_period_policy_data_t *policy_data;

    policy_data=(acoral_period_policy_data_t *)acoral_malloc(sizeof(acoral_period_policy_data_t));
    if(policy_data==NULL){
        printf("No level2 mem space for policy_data:%s\n",thread->name);
        acoral_enter_critical();
        acoral_release_res((acoral_res_t *)thread);
        acoral_exit_critical();
        return -1;
    }
    policy_data->period_time_mm=((acoral_period_policy_data_t*)data)->period_time_mm;
    thread->policy_data=policy_data;

    /* 分配TCB中的period_timer */
    acoral_timer_t* period_timer = (acoral_timer_t *)acoral_get_res(ACORAL_RES_TIMER);
    if(NULL==period_timer){
        printf("Alloc thread timer fail\n");
        printf("No Mem Space or Beyond the max thread\n");
        return -1;
    }
    acoral_init_list(&period_timer->delay_queue_hook);
    thread->thread_period_timer = period_timer;
    thread->thread_period_timer->owner = thread->res;

	if(system_thread_init(thread,period_thread_exit)!=0){
		printf("No thread stack:%s\n",thread->name);
		acoral_enter_critical();
		acoral_release_res((acoral_res_t *)thread);
		acoral_exit_critical();
		return -1;
	}
        /*将线程就绪，并重新调度*/
	acoral_resume_thread(thread);
	acoral_enter_critical();
	period_thread_delay(thread,((acoral_period_policy_data_t*)data)->period_time_mm);
	acoral_exit_critical();
	return thread->res.id;
}

void period_policy_thread_release(acoral_thread_t *thread){
	acoral_free2(thread->policy_data);	
}

void acoral_periodqueue_add(acoral_thread_t *new){
	acoral_list_t   *tmp,*head;
	acoral_thread_t *thread;
	int  delay2;
	int  new_thread_period= new->thread_period_timer->delay_time;
	head=&period_wait_queue;
	new->state|=ACORAL_THREAD_STATE_DELAY;

    /* 找到period_wait_queue中新线程应该插入的位置并插入 */
	for (tmp=head->next; delay2 = new_thread_period, tmp != head; tmp = tmp->next){
		thread = list_entry (tmp, acoral_thread_t, period_wait_hook);
		new_thread_period  = new_thread_period - thread->thread_period_timer->delay_time;
		if (new_thread_period < 0)
			break;
	}
	new->thread_period_timer->delay_time = delay2;
	acoral_list_add(&new->period_wait_hook,tmp->prev);

	/* 插入新线程后，后续线程的时间处理*/
	if(tmp != head){
		thread = list_entry(tmp, acoral_thread_t, period_wait_hook);
		thread->thread_period_timer->delay_time-=delay2;
	}
}

void period_thread_delay(acoral_thread_t* thread,unsigned int time){
	thread->thread_period_timer->delay_time=time_to_ticks(time);
	acoral_periodqueue_add(thread);
}

void period_delay_deal(){
	// int need_re_sched= 0;
	acoral_list_t *tmp,*tmp1,*head;
	acoral_thread_t * thread;
	head=&period_wait_queue;
	if(acoral_list_empty(head))
	    	return;
	thread=list_entry(head->next,acoral_thread_t,period_wait_hook);
	thread->thread_period_timer->delay_time--;
	for(tmp=head->next;tmp!=head;){
		thread=list_entry(tmp,acoral_thread_t,period_wait_hook);
		if(thread->thread_period_timer->delay_time>0)
		    break;
		/*防止add判断delay时取下thread*/
		tmp1=tmp->next;
		acoral_list_del(&thread->period_wait_hook);
		tmp=tmp1;
		if(thread->state&ACORAL_THREAD_STATE_SUSPEND){
			thread->stack=(unsigned int *)((char *)thread->stack_buttom+thread->stack_size-4);
			thread->stack = HAL_STACK_INIT(thread->stack,thread->route,period_thread_exit,thread->args);
			acoral_rdy_thread(thread);
			// need_re_sched = 1;
		}
		period_thread_delay(thread,((acoral_period_policy_data_t*)thread->policy_data)->period_time_mm);
		// if(need_re_sched)
		// {
		// 	acoral_sched();//SPG我是小丑，时钟中断里不能调度的，应该在把中断退出函数加回来
		// }
	}
}

void period_thread_exit(){
	acoral_suspend_self();
}


void period_policy_init(void){
	acoral_init_list(&period_wait_queue);

    acoral_sched_policy_t* period_policy = acoral_get_res(ACORAL_RES_POLICY);

	period_policy->type=ACORAL_SCHED_POLICY_PERIOD;
	period_policy->policy_thread_init=period_policy_thread_init;
	period_policy->policy_thread_release=period_policy_thread_release;
	period_policy->delay_deal=period_delay_deal;
	acoral_register_sched_policy(period_policy);
}

#endif