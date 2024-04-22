/**
 * @file thread.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，线程优先级、控制块定义，线程管理函数声明
 * @version 1.1
 * @date 2023-04-19
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>pegasus <td>2010-07-19 <td>增加timeout链表，用来处理超时，挂g_timeout_queue
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-06-24 <td>Standardized 
 * 	 <tr><td> 1.1 <td>王彬浩 <td> 2023-04-19 <td>use enum 
 *  </table>
 */

#ifndef ACORAL_THREAD_H
#define ACORAL_THREAD_H
#include "autocfg.h"
#include "list.h"
#include "mem.h"
#include "event.h"
#include "policy.h"
#include "soft_timer.h"
#include "lsched.h"

#include <stdbool.h>

#define ACORAL_MINI_PRIO CFG_MAX_THREAD ///<aCoral最低优先级40

extern unsigned char system_need_sched; 
extern unsigned char system_sched_locked;
extern acoral_thread_t *acoral_cur_thread;

///就绪队列中的优先级位图的大小，目前等于2，算法就是优先级数目除以32向上取整
#define ACORAL_MAX_PRIO_NUM ((CFG_MAX_THREAD + 1) & 0xff) ///<41。总共有40个线程，就有0~40共41个优先级
#define PRIO_BITMAP_SIZE ((ACORAL_MAX_PRIO_NUM+31)/32) 

typedef enum{
	ACORAL_INIT_PRIO,	///<init线程独有的0优先级
	ACORAL_MAX_PRIO,	///<aCoral系统中允许的最高优先级
	ACORAL_HARD_RT_PRIO_MAX,	///<硬实时任务最高优先级
	ACORAL_HARD_RT_PRIO_MIN = ACORAL_HARD_RT_PRIO_MAX+CFG_HARD_RT_PRIO_NUM,	///<硬实时任务最低优先级
	ACORAL_NONHARD_RT_PRIO_MAX,	///<非硬实时任务最高优先级

	ACORAL_DAEMON_PRIO = ACORAL_MINI_PRIO-2,	///<daemon回收线程专用优先级
	ACORAL_NONHARD_RT_PRIO_MIN,	///<非硬实时任务最低优先级
	ACORAL_IDLE_PRIO	///<idle线程专用优先级，也是系统最低优先级ACORAL_MINI_PRIO
}acoralPrioEnum;

typedef enum{
	ACORAL_NONHARD_PRIO, ///<非硬实时任务的优先级，会将tcb中的prio加上ACORAL_NONHARD_RT_PRIO_MAX
	ACORAL_HARD_PRIO	 ///<硬实时任务优先级，会将tcb中的prio加上ACORAL_HARD_RT_PRIO_MAX
}acoralPrioTypeEnum;

typedef enum{
	ACORAL_THREAD_STATE_READY = 1,			///表示线程已经被挂载到acoral_ready_queues，就绪状态
	ACORAL_THREAD_STATE_SUSPEND = 1<<1,		///表示线程已经被从acoral_ready_queues取下，挂起状态
	ACORAL_THREAD_STATE_RUNNING = 1<<2, 	///表示线程正在运行
	ACORAL_THREAD_STATE_EXIT = 1<<3,		///表示线程已经运行完毕，但还不能被daem线程释放，等待被切换之后才能彻底送走
	ACORAL_THREAD_STATE_RELEASE = 1<<4,		///表示线程已经运行完毕并已经被切换，等待daem回收TCB和堆栈资源
	ACORAL_THREAD_STATE_DELAY = 1<<5,		///表示线程将在一段时间之后被重新唤醒并挂载到acoral_ready_queues上，对于普通线程来说，就是调用了delay接口，对于周期线程来说，就是周期
}acoralThreadStateEnum;

typedef enum{
    ACORAL_ERR_THREAD,
    ACORAL_ERR_THREAD_DELAY,
    ACORAL_ERR_THREAD_NO_STACK  ///<线程栈指针为空
}acoralThreadErrorEnum;

/**
 * 
 *  @struct acoral_thread_t
 *  @brief 线程控制块TCB
 * 
 * 
 */
typedef struct acoral_thread_tcb{
  	acoral_res_t res;	            ///<资源id，线程创建后作为线程id

    /* 基本信息 */
    char *name;
	acoralThreadStateEnum state;    ///<线程状态
	unsigned char prio;             ///<原始优先级
	acoralSchedPolicyEnum policy;   ///<调度策略
    unsigned int *stack;            ///<栈顶指针，高地址
	unsigned int *stack_buttom;	    ///<栈底指针，低地址
	unsigned int stack_size;        ///<栈大小
    
    /* 钩子 */
    acoral_list_t ready_hook;	        ///<用于挂载到全局就绪队列
	acoral_list_t timeout_hook;         ///<
    acoral_list_t daem_hook;            ///<用于挂载到daem线程回收队列
    acoral_list_t ipc_waiting_hook;     ///<用于挂载到ipc（互斥量、信号量、消息）等待队列
#if	CFG_THRD_PERIOD
	acoral_list_t period_wait_hook; ///<周期等待队列

    /* timer */
    acoral_timer_t* thread_period_timer; ///<用于周期线程等待下一个周期到来，因为线程在等待这个周期的过程中是处于运行状态的，因此不能和thread_timer共用
#endif
    acoral_timer_t* thread_timer; ///<用于等待互斥量、信号量等的超时时间timeout、线程延时acoral_delay_self的时间，这些等待过程的共同点在于线程都是在suspend状态下等待的，不存在又等互斥量又等线程延时时间的情况，因此可以共用一个timer
	
    /* 获取的资源 */
    acoral_evt_t* evt; //SPG 只能获取一个信号量或者互斥量？

	void*	private_data;
	void*	data;
}acoral_thread_t;

/**
 * @brief aCoral就绪队列
 * 
 */
typedef struct{
	unsigned int num;							///<就绪的线程数
	unsigned int bitmap[PRIO_BITMAP_SIZE];		///<优先级位图，每一位对应一个优先级，为1表示这个优先级有就绪线程
	acoral_list_t queue[ACORAL_MAX_PRIO_NUM];	///<每一个优先级都有独立的队列
}acoral_rdy_queue_t;

typedef struct{
    acoral_list_t daem_thread_res_release_queue;
    acoral_rdy_queue_t ready_queue;
}thread_res_private_data;


void acoral_suspend_thread(acoral_thread_t *thread);
void acoral_resume_thread(acoral_thread_t *thread);
void acoral_kill_thread(acoral_thread_t *thread);
int system_thread_init(acoral_thread_t *thread,void (*route)(void *args),void (*exit)(void),void *args);

void system_thread_module_init(void);
void acoral_unrdy_thread(acoral_thread_t *thread);
void acoral_rdy_thread(acoral_thread_t *thread);
void acoral_thread_change_prio(acoral_thread_t* thread, unsigned int prio);

/***************线程控制API****************/

/**
 * @brief 创建一个线程
 * 
 * @param route 线程函数
 * @param stack_size 线程栈大小
 * @param args 线程函数参数
 * @param name 线程名字
 * @param stack 线程栈指针
 * @param sched_policy 线程调度策略
 * @param data 线程策略数据
 * @return int 成功返回线程id，失败返回-1
 */
int acoral_create_thread(void (*route)(void *args),unsigned int stack_size,void *args,char *name,void *stack,acoralSchedPolicyEnum sched_policy,void *data);

/**
 * @brief 挂起当前线程
 * 
 */
void acoral_suspend_self(void);

/**
 * @brief 挂起某个线程
 * 
 * @param thread_id 要挂起的线程id
 */
void acoral_suspend_thread_by_id(unsigned int thread_id);

/**
 * @brief 唤醒某个线程
 * 
 * @param thread_id 要唤醒的线程id
 */
void acoral_resume_thread_by_id(unsigned int thread_id);

/**
 * @brief 将当前线程延时
 * 
 * @param time 延时时间（毫秒）
 */
void acoral_delay_self(unsigned int time);

/**
 * @brief 干掉某个线程
 * 
 * @param id 要干掉的线程id
 */
void acoral_kill_thread_by_id(int id);

/**
 * @brief 结束当前线程
 * 
 */
void comm_thread_exit(void);

/**
 * @brief 改变当前线程优先级
 * 
 * @param prio 目标优先级
 */
void acoral_change_prio_self(unsigned int prio);

/**
 * @brief 改变某个线程优先级
 * 
 * @param thread_id 线程id
 * @param prio 目标优先级
 */
void acoral_thread_change_prio_by_id(unsigned int thread_id, unsigned int prio);


/* *******************lsched.h**************************** */




void system_set_running_thread(acoral_thread_t *thread);
void acoral_thread_runqueue_init(void);

/**
 * @brief 将某个线程挂载到就绪队列上
 * 
 * @param new 将被挂载的线程
 */
void acoral_rdyqueue_add(acoral_thread_t *new);

/**
 * @brief 从就绪队列上删除一个线程
 * 
 * @param old 要删除的线程
 */
void acoral_rdyqueue_del(acoral_thread_t *old);


/**
 * @brief 从就绪队列中选出优先级最高的线程
 * 
 * @return acoral_thread_t* 优先级最高的线程
 */
acoral_thread_t* acoral_select_thread();

void acoral_sched(void);
void acoral_real_sched();
unsigned long acoral_real_intr_sched(unsigned long old_sp);


#endif

