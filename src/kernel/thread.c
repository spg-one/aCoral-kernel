/**
 * @file thread.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，线程机制相关函数
 * @version 2.0
 * @date 2024-04-22
 * @copyright Copyright (c) 2024
 */
#include "thread.h"
#include "int.h"
#include "log.h"

#include "hal.h"

#include <stdio.h>

extern void acoral_evt_queue_del(acoral_thread_t *thread);
extern int daemon_id;

/// aCoral是否需要调度标志，仅当aCoral就绪队列ready_queue有线程加入或被取下时，该标志被置为true；
/// 当发生调度之后，该标志位被置为false，直到又有新的线程被就绪或者挂起
unsigned char system_need_sched = false; 

/// aCoral初始化完成之前，调度都是被上锁的 
unsigned char system_sched_locked = true;

/// acoral当前运行的线程
acoral_thread_t *acoral_cur_thread = NULL;		

int acoral_create_thread(char *name, void (*route)(void *args),void *args,unsigned int stack_size,void *stack,acoralSchedPolicyEnum sched_policy,unsigned char prio,acoralPrioTypeEnum prio_type,void *data){
	acoral_thread_t* thread;
    acoral_timer_t* thread_timer;

    /* 分配TCB资源*/
	thread = (acoral_thread_t *)acoral_get_res(ACORAL_RES_THREAD);
	if(NULL == thread){
		ACORAL_LOG_ERROR("Alloc thread:%s fail, No Mem Space or Beyond the max thread\n",name);
		return -1;
	}

    /* TCB 基本信息初始化 */
	thread->name = name;
    thread->state = ACORAL_THREAD_STATE_SUSPEND;
    thread->route = route;
    thread->args = args;
    thread->policy = sched_policy;
    thread->prio = prio;
    thread->prio_type = prio_type;
	thread->stack_size = stack_size&(~(hal_sp_align-1)); //确保堆栈是hal_sp_align字节对齐的
    thread->stack_buttom = (stack == NULL) ? NULL : (unsigned int *)stack;

    /* 根据优先级类型调整prio */
    if (thread->prio_type == ACORAL_NONHARD_PRIO)
	{
		thread->prio += ACORAL_NONHARD_RT_PRIO_MAX;
		if (thread->prio >= ACORAL_NONHARD_RT_PRIO_MIN)
			thread->prio = ACORAL_NONHARD_RT_PRIO_MIN;
	}
	// SPG加上硬实时判断
	//  else{
	//  	thread->prio += ACORAL_HARD_RT_PRIO_MAX;
	//  	if(thread->prio > ACORAL_HARD_RT_PRIO_MIN){
	//  		thread->prio = ACORAL_HARD_RT_PRIO_MIN;
	//  	}
	//  }

    /* 钩子初始化 */
    acoral_init_list(&thread->timeout_hook);
    acoral_init_list(&thread->ready_hook);
    acoral_init_list(&thread->daem_hook);
    acoral_init_list(&thread->ipc_waiting_hook);

    /* 初始化 thread_timer */
    thread_timer = (acoral_timer_t *)acoral_get_res(ACORAL_RES_TIMER);
    if(NULL == thread_timer){
		ACORAL_LOG_ERROR("Alloc thread timer fail\n");
		return -1;
	}
    acoral_init_list(&thread_timer->delay_queue_hook);
    thread->thread_timer = thread_timer;
    thread->thread_timer->owner = thread->res;

    /* 根据策略进行特异初始化 */
	return acoral_policy_thread_init(sched_policy,thread,data);
}

static void suspend_thread(acoral_thread_t *thread){
	/* 挂起一个线程等价于把线程从acoral_ready_queues上取下，这就意味着这个线程必须之前在acoral_ready_queues上，也就等价于必须是就绪状态的线程才能被挂起*/
    if(!(ACORAL_THREAD_STATE_READY&thread->state)) 
	{
        return;
    }
	acoral_enter_critical();
	acoral_rdyqueue_del(thread);
	acoral_exit_critical();

	acoral_sched();
}

void acoral_suspend_self(){
	suspend_thread(acoral_cur_thread);
}

void acoral_suspend_thread_by_id(int thread_id){
	acoral_thread_t *thread=(acoral_thread_t *)acoral_get_res_by_id(thread_id);
	suspend_thread(thread);
}

void acoral_resume_thread_by_id(int thread_id){
	acoral_thread_t *thread=(acoral_thread_t *)acoral_get_res_by_id(thread_id);
	acoral_resume_thread(thread);
}

void acoral_resume_thread(acoral_thread_t *thread){
	if(!(thread->state&ACORAL_THREAD_STATE_SUSPEND))
		return;

	acoral_enter_critical();
	acoral_rdyqueue_add(thread);
	acoral_exit_critical();
	
	acoral_sched();
}

static void acoral_delay_thread(acoral_thread_t* thread,unsigned int time){
	unsigned int real_ticks;
	if(!acoral_list_empty(&thread->thread_timer->delay_queue_hook)){
		return;	
	}

	/*timeticks*/
	/*real_ticks=time*CFG_TICKS_PER_SEC/1000;*/
	real_ticks = time_to_ticks(time);
	thread->thread_timer->delay_time = real_ticks;
	/**/
	acoral_delayqueue_add(&time_delay_queue,thread);
}

void acoral_delay_self(unsigned int time){
	acoral_delay_thread(acoral_cur_thread,time);
}

void acoral_kill_thread(acoral_thread_t *thread){
	acoral_evt_t *evt;
	acoral_enter_critical();
        /*	*/
        /*	*/
	if(thread->state&ACORAL_THREAD_STATE_SUSPEND){
		evt=thread->evt;
		/**/
		if(thread->state&ACORAL_THREAD_STATE_DELAY){
			acoral_list_del(&thread->thread_timer->delay_queue_hook);
		}else
		{
			/**/
			if(evt!=NULL){
				acoral_evt_queue_del(thread);
			}
		}
	}
	acoral_unrdy_thread(thread);
	
    /* 让线程进入ACORAL_THREAD_STATE_EXIT状态，但此时TCB和堆栈在上下文切换和函数调用的时候还有用，直到切换到新线程的上下文之后，才会变成ACORAL_THREAD_STATE_RELEASE状态，这个状态下的线程才会被daem释放。详见绿书P98.*/
    acoral_list_t* daem_res_release_queue = &(((thread_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_THREAD].type_private_data))->daem_thread_res_release_queue); ///< 将被daem线程回收的线程队列

    acoral_list_t *head = daem_res_release_queue;
	acoral_thread_t *daem = (acoral_thread_t *)acoral_get_res_by_id(daemon_id);
	thread->state=ACORAL_THREAD_STATE_EXIT;
	acoral_list_add2_tail(&thread->daem_hook,head);
	acoral_rdy_thread(daem);

    acoral_exit_critical();
	acoral_sched();
}

void acoral_kill_thread_by_id(int id){
	acoral_thread_t *thread;
	thread=(acoral_thread_t *)acoral_get_res_by_id(id);
	acoral_kill_thread(thread);
}

void comm_thread_exit(){
        acoral_kill_thread(acoral_cur_thread);
}

void acoral_thread_change_prio(acoral_thread_t* thread, unsigned int prio){
	acoral_enter_critical();
	if(thread->state&ACORAL_THREAD_STATE_READY){
		acoral_rdyqueue_del(thread);
		thread->prio = prio;
		acoral_rdyqueue_add(thread);
	}else
		thread->prio = prio;
	acoral_exit_critical();
}

void acoral_change_prio_self(unsigned int prio){
	acoral_thread_change_prio(acoral_cur_thread, prio);
}

void acoral_thread_change_prio_by_id(unsigned int thread_id, unsigned int prio){
	acoral_thread_t *thread=(acoral_thread_t *)acoral_get_res_by_id(thread_id);
	acoral_thread_change_prio(thread, prio);
}

void acoral_rdy_thread(acoral_thread_t *thread){
	if(!(ACORAL_THREAD_STATE_SUSPEND&thread->state)) //SPG线程必须要不在就绪队列上（有suspend状态）才能被就绪（加入就绪队列）
		return;

	acoral_rdyqueue_add(thread);
}

void acoral_unrdy_thread(acoral_thread_t *thread){
	if(!(ACORAL_THREAD_STATE_READY&thread->state))
		return;

	acoral_rdyqueue_del(thread);
}

int system_thread_init(acoral_thread_t *thread,void (*exit)(void)){
	unsigned int stack_size=thread->stack_size;
	if(thread->stack_buttom==NULL){
		if(stack_size<CFG_MIN_STACK_SIZE)
			stack_size=CFG_MIN_STACK_SIZE;
		thread->stack_buttom=(unsigned int *)acoral_malloc(stack_size);
		if(thread->stack_buttom==NULL)
			return ACORAL_ERR_THREAD_NO_STACK;
		thread->stack_size=stack_size;
	}
	thread->stack=(unsigned int *)((char *)thread->stack_buttom+stack_size-4);
	thread->stack = HAL_STACK_INIT(thread->stack,thread->route,exit,thread->args);

	return 0;
}

void acoral_sched_mechanism_init(){
	acoral_thread_runqueue_init();
}

void system_thread_module_init(){
	acoral_sched_mechanism_init();
	acoral_sched_policy_init();
}


/* ******************sched.c********************** */



void acoral_prio_queue_add(acoral_rdy_queue_t *array, unsigned char prio, acoral_list_t *list)
{
	acoral_list_t *queue;
	acoral_list_t *head;
	array->num++;
	queue = array->queue + prio;
	head = queue;
	acoral_list_add2_tail(list, head);
	acoral_set_bit_in_bitmap(prio, array->bitmap);
}

void acoral_prio_queue_del(acoral_rdy_queue_t *array, unsigned char prio, acoral_list_t *list)
{
	acoral_list_t *queue;
	acoral_list_t *head;
	queue = array->queue + prio;
	head = queue;
	array->num--;
	acoral_list_del(list);
	if (acoral_list_empty(head))
		acoral_clear_bit_in_bitmap(prio, array->bitmap);
}

unsigned int acoral_get_highprio(acoral_rdy_queue_t *array)
{
	return acoral_find_first_bit_in_array(array->bitmap, PRIO_BITMAP_SIZE,1);
}

void acoral_prio_queue_init(acoral_rdy_queue_t *array)
{
	unsigned char i;
	acoral_list_t *queue;
	acoral_list_t *head;
	array->num = 0;
	for (i = 0; i < PRIO_BITMAP_SIZE; i++)
		array->bitmap[i] = 0;
	for (i = 0; i < ACORAL_MAX_PRIO_NUM; i++)
	{
		queue = array->queue + i;
		head = queue;
		acoral_init_list(head);
	}
}


void system_set_running_thread(acoral_thread_t *thread)
{
	acoral_cur_thread->state &= ~ACORAL_THREAD_STATE_RUNNING;
	thread->state |= ACORAL_THREAD_STATE_RUNNING;
	acoral_cur_thread = thread;
}


void acoral_thread_runqueue_init() //TODO多核队列？
{
	/*初始化每个核上的优先级队列*/
    acoral_rdy_queue_t* rdy_queue = &(((thread_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_THREAD].type_private_data))->ready_queue);
	acoral_prio_queue_init(rdy_queue);
}

void acoral_rdyqueue_add(acoral_thread_t *thread)
{
	 acoral_rdy_queue_t* rdy_queue = &(((thread_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_THREAD].type_private_data))->ready_queue);
	acoral_prio_queue_add(rdy_queue, thread->prio, &thread->ready_hook);
	thread->state &= ~ACORAL_THREAD_STATE_SUSPEND;
	thread->state |= ACORAL_THREAD_STATE_READY;
	system_need_sched = true;
}

void acoral_rdyqueue_del(acoral_thread_t *thread)
{
	 acoral_rdy_queue_t* rdy_queue = &(((thread_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_THREAD].type_private_data))->ready_queue);
	acoral_prio_queue_del(rdy_queue, thread->prio, &thread->ready_hook);
	thread->state &= ~ACORAL_THREAD_STATE_READY;
	thread->state &= ~ACORAL_THREAD_STATE_RUNNING;
	thread->state |= ACORAL_THREAD_STATE_SUSPEND;
	/*设置线程所在的核可调度*/
	system_need_sched = true;
}

void acoral_sched()
{
	/*如果不需要调度，则返回*/
	if (!system_need_sched)
    {
        return;
    }

	if (acoral_intr_nesting)
    {
        return;
    }
	if (system_sched_locked)
    {
        return;
    }
		
		
	/*这个函数进行简单处理后会直接或间接调用acoral_real_sched,或者acoral_real_intr_sched*/
	HAL_SCHED_BRIDGE();
	return;
}
void acoral_real_sched()
{
	acoral_thread_t *prev;
	acoral_thread_t *next;
	system_need_sched = false;
	prev = acoral_cur_thread;
	/*选择最高优先级线程*/
	next = acoral_select_thread();
	if (prev != next)
	{
		system_set_running_thread(next);
		ACORAL_LOG_TRACE("Switch to Thread: %s",acoral_cur_thread->name);
		if (prev->state == ACORAL_THREAD_STATE_EXIT)
		{
			prev->state = ACORAL_THREAD_STATE_RELEASE;
			HAL_SWITCH_TO(&next->stack);
			return;
		}
		/*线程切换*/
		HAL_CONTEXT_SWITCH(&prev->stack, &next->stack);
	}
}

unsigned long acoral_real_intr_sched(unsigned long old_sp)
{
	acoral_thread_t *prev;
	acoral_thread_t *next;
	system_need_sched = false;
	prev = acoral_cur_thread;
	/*选择最高优先级线程*/
	next = acoral_select_thread();
	if (prev != next)
	{
		system_set_running_thread(next);
		// ACORAL_LOG_TRACE("Switch to Thread: %s\n",acoral_cur_thread->name);
		if (prev->state == ACORAL_THREAD_STATE_EXIT)
		{
			prev->state = ACORAL_THREAD_STATE_RELEASE;
			return (unsigned long)next->stack;
		}
		prev->stack = (unsigned int*)old_sp;
		//需要在中断退出时切换线程，就返回新线程的栈指针
		return (unsigned long)next->stack;
	}
	//不需要在中断退出时切换线程，直接返回旧线程指针
	return old_sp;
}

acoral_thread_t* acoral_select_thread()
{
	unsigned int index;
	acoral_list_t *head;
	acoral_thread_t *thread;
	acoral_list_t *queue;
	acoral_rdy_queue_t* rdy_queue = &(((thread_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_THREAD].type_private_data))->ready_queue);
	/*找出就绪队列中优先级最高的线程的优先级*/
	index = acoral_get_highprio(rdy_queue);
	queue = rdy_queue->queue + index;
	head = queue;
	thread = list_entry(head->next, acoral_thread_t, ready_hook);
	return thread;
}
