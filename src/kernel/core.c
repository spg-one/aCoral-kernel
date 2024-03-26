/**
 * @file core.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief aCoral内核初始化
 * @version 2.0
 * @date 2024-03-26
 * @copyright Copyright (c) 2024
 */
#include "kernel.h"

#include "hal.h"

#include <stdlib.h>

acoral_list_t acoral_res_release_queue; ///< 将被daem线程回收的线程队列
volatile unsigned int acoral_start_sched = false; ///<aCoral启动后，经过init线程，这个变量就永远变为true
int daemon_id, idle_id, init_id;
extern void user_main();

char* logo = "\n\
              \n\
             $$$$$$\\                                $$\\ \n\
            $$  __$$\\                               $$ |\n\
   $$$$$$\\  $$ /  \\__| $$$$$$\\   $$$$$$\\   $$$$$$\\  $$ |\n\
   \\____$$\\ $$ |      $$  __$$\\ $$  __$$\\  \\____$$\\ $$ |\n\
   $$$$$$$ |$$ |      $$ /  $$ |$$ |  \\__| $$$$$$$ |$$ |\n\
  $$  __$$ |$$ |  $$\\ $$ |  $$ |$$ |      $$  __$$ |$$ |\n\
  \\$$$$$$$ |\\$$$$$$  |\\$$$$$$  |$$ |      \\$$$$$$$ |$$ |\n\
   \\_______| \\______/  \\______/ \\__|       \\_______|\\__|\n\
              \n\
              \n\
";

/**
 * @brief aCoral空闲线程idle函数
 *
 */
static void idle()
{
	for(;;){}
}

/**
 * @brief aCoral初始化线程init函数
 *
 */
static void init()
{
	acoral_intr_disable();
	ACORAL_LOG_TRACE("Init Thread Start");

	acoral_init_list(&time_delay_queue);
	acoral_init_list(&timeout_queue);
	acoral_init_list(&acoral_res_release_queue);

	if(acoral_ticks_init()!=0){
		ACORAL_LOG_ERROR("Ticks Init Failed");
		exit(1);
	}
	ACORAL_LOG_TRACE("Ticks Init Done");
	acoral_intr_enable();
	acoral_start_sched = true;

	/*应用级相关服务初始化,应用级不要使用延时函数，没有效果的*/
#ifdef CFG_SHELL
	acoral_shell_init();
#endif
	user_main();
	ACORAL_LOG_TRACE("Init Thread Done");
	
}

/**
 * @brief aCoral资源回收线程daem函数
 *
 */
static void daem()
{
	acoral_thread_t *thread;
	acoral_list_t *head, *tmp, *tmp1;
	head = &acoral_res_release_queue;
	while (1)
	{
		for (tmp = head->next; tmp != head;)
		{
			tmp1 = tmp->next;
			acoral_enter_critical();
			thread = list_entry(tmp, acoral_thread_t, waiting);

			/* 如果线程资源已经不再使用，即release状态，则释放 */
			acoral_list_del(tmp);
			acoral_exit_critical();
			tmp = tmp1;
			if (thread->state == ACORAL_THREAD_STATE_RELEASE)
			{
				ACORAL_LOG_INFO("Daem is Cleaning Thread: %s",thread->name);
				acoral_release_thread((acoral_res_t *)thread);
			}
			else
			{
				acoral_enter_critical();
				tmp1 = head->prev;
				acoral_list_add2_tail(&thread->waiting, head); /**/
				acoral_exit_critical();
			}
		}
		acoral_suspend_self();
	}
}

/**
 * @brief aCoral内核各模块初始化
 * 
 */
static void module_init()
{
	acoral_intr_sys_init();
	acoral_mem_sys_init();
	acoral_thread_sys_init();
	acoral_evt_sys_init();
}

/**
 * @brief 主CPU创建idle、init、daem线程
 * 
 */
static void acoral_main_cpu_start()
{
	acoral_comm_policy_data_t data;

	/* 创建idle线程 */
	acoral_start_sched = false;
	data.prio = ACORAL_IDLE_PRIO;
	data.prio_type = ACORAL_HARD_PRIO;
	idle_id = acoral_create_thread(idle, IDLE_STACK_SIZE, NULL, "idle", NULL, ACORAL_SCHED_POLICY_COMM, &data);
	if (idle_id == -1)
	{
		ACORAL_LOG_ERROR("Create Idle Thread Failed");
		exit(2);
	}

	/* 创建init线程 */
	data.prio = ACORAL_INIT_PRIO;
	init_id = acoral_create_thread(init, INIT_STACK_SIZE, "in init", "init", NULL, ACORAL_SCHED_POLICY_COMM, &data);
	if (init_id == -1)
	{
		ACORAL_LOG_ERROR("Create Init Thread Failed");
		exit(2);
	}

	/* 创建daem线程 */
	data.prio = ACORAL_DAEMON_PRIO;
	data.prio_type = ACORAL_HARD_PRIO;
	daemon_id = acoral_create_thread(daem, DAEM_STACK_SIZE, NULL, "daemon", NULL, ACORAL_SCHED_POLICY_COMM, &data);
	if (daemon_id == -1){
		ACORAL_LOG_ERROR("Create Daem Thread Failed");
		exit(2);
	}	
	printf("%s",logo);

	acoral_sched_locked = false;
	acoral_need_sched = false; 
	acoral_set_running_thread(acoral_select_thread());
	HAL_SWITCH_TO(&acoral_cur_thread->stack);
}

void system_start()
{
	/* 关中断，在init线程中打开 */
	acoral_intr_disable();

	ACORAL_LOG_DEBUG("Build at %s %s",__DATE__,__TIME__);

	ACORAL_LOG_TRACE("Kernel Module Init Start");
	module_init();
	ACORAL_LOG_TRACE("Kernel Module Init Done");

	acoral_main_cpu_start();
}