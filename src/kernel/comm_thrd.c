/**
 * @file comm_thrd.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，普通（先来先服务线程）策略线程
 * @version 2.0
 * @date 2024-03-22
 * @copyright Copyright (c) 2024
 */
#include "comm_thrd.h"
#include "thread.h"
#include "lsched.h"
#include "policy.h"
#include "int.h"
#include "log.h"

#include "hal.h"


/**
 * @brief 初始化普通线程的一些数据
 * 
 * @param thread TCB指针
 * @param route 线程函数
 * @param args 线程函数参数
 * @param data 线程私有数据
 * @return int 线程id
 */
static int comm_policy_thread_init(acoral_thread_t *thread, void (*route)(void *args), void *args, void *data)
{
	unsigned int prio;
	acoral_comm_policy_data_t *policy_data;
	policy_data = (acoral_comm_policy_data_t *)data;
	prio = policy_data->prio;
	if (policy_data->prio_type == ACORAL_NONHARD_PRIO)
	{
		prio += ACORAL_NONHARD_RT_PRIO_MAX;
		if (prio >= ACORAL_NONHARD_RT_PRIO_MIN)
			prio = ACORAL_NONHARD_RT_PRIO_MIN;
	}
	// SPG加上硬实时判断
	//  else{
	//  	prio += ACORAL_HARD_RT_PRIO_MAX;
	//  	if(prio > ACORAL_HARD_RT_PRIO_MIN){
	//  		prio = ACORAL_HARD_RT_PRIO_MIN;
	//  	}
	//  }
	thread->prio = prio;

	if (system_thread_init(thread, route, comm_thread_exit, args) != 0)
	{
		ACORAL_LOG_ERROR("No thread stack:%s", thread->name);
		acoral_enter_critical();
		acoral_release_res((acoral_res_t *)thread);
		acoral_exit_critical();
		return -1;
	}
	/*将线程就绪，并重新调度*/
	acoral_resume_thread(thread);
	return thread->res.id;
}


void comm_policy_init()
{
    acoral_sched_policy_t* comm_policy = (acoral_sched_policy_t*)acoral_get_res(ACORAL_RES_POLICY);
	comm_policy->type = ACORAL_SCHED_POLICY_COMM;
	comm_policy->policy_thread_init = comm_policy_thread_init;
	comm_policy->policy_thread_release = NULL;
	comm_policy->delay_deal = NULL;
	acoral_register_sched_policy(comm_policy);
}
