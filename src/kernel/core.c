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
int daemon_id, idle_id, init_id;

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

extern void user_main();

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

    /* 初始化全局延时队列 */
	acoral_init_list(&(((timer_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_TIMER].type_private_data))->global_time_delay_queue));
    
    /* 初始化全局超时队列 */
	acoral_init_list(&(((timer_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_TIMER].type_private_data))->global_timeout_queue));
    
    /* 初始化daem线程回收的线程队列 */
	acoral_init_list(&(((thread_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_THREAD].type_private_data))->global_daem_release_queue));

	if(system_ticks_init()!=0){
		ACORAL_LOG_ERROR("Ticks Init Failed");
		exit(1);
	}
	ACORAL_LOG_TRACE("Ticks Init Done");

	/* 开中断，此时系统才真正能调度 */
	acoral_intr_enable();

	/*应用级相关服务初始化,应用级不要使用延时函数，没有效果的*/
#ifdef CFG_SHELL
	// system_shell_init();
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
    acoral_list_t* daem_res_release_queue = &(((thread_res_private_data*)(acoral_res_system.system_res_ctrl_container[ACORAL_RES_THREAD].type_private_data))->global_daem_release_queue); ///< 将被daem线程回收的线程队列

	head = daem_res_release_queue;
	while (1)
	{
		for (tmp = head->next; tmp != head;)
		{
			tmp1 = tmp->next;
			acoral_enter_critical();
			thread = list_entry(tmp, acoral_thread_t, daem_hook);

			/* 如果线程资源已经不再使用，即release状态，则释放 */
			acoral_list_del(tmp);
			acoral_exit_critical();
			tmp = tmp1;
			if (thread->state == ACORAL_THREAD_STATE_RELEASE)
			{
				ACORAL_LOG_INFO("Daem is Cleaning Thread: %s",thread->name);
				system_policy_thread_release(thread);
  				acoral_free((void *)thread->stack_buttom);
				acoral_release_res((acoral_res_t *)thread);
			}
			else
			{
				acoral_enter_critical();
				tmp1 = head->prev;
				acoral_list_add2_tail(&thread->daem_hook, head); /**/
				acoral_exit_critical();
			}
		}
		acoral_suspend_self();
	}
}

/**
 * @brief 主CPU创建idle、init、daem线程
 * 
 */
static void main_cpu_start()
{

	/* 创建idle线程 */
	idle_id = acoral_create_thread("idle",idle, NULL, IDLE_STACK_SIZE, ACORAL_SCHED_POLICY_COMM, ACORAL_IDLE_PRIO,ACORAL_HARD_PRIO,NULL);
	if (idle_id == -1) //SPG 不一定是-1才有问题吧
	{
		ACORAL_LOG_ERROR("Create Idle Thread Failed");
		exit(2);//SPG 新建一个lib文件夹，用宏定义来封装不同的SDK
	}

	/* 创建init线程 */
	init_id = acoral_create_thread("init",init, "in init", INIT_STACK_SIZE, ACORAL_SCHED_POLICY_COMM, ACORAL_INIT_PRIO,ACORAL_HARD_PRIO,NULL);
	if (init_id == -1)
	{
		ACORAL_LOG_ERROR("Create Init Thread Failed");
		exit(2);
	}

	/* 创建daem线程 */
	daemon_id = acoral_create_thread("daemon",daem, NULL, DAEM_STACK_SIZE, ACORAL_SCHED_POLICY_COMM, ACORAL_DAEMON_PRIO,ACORAL_HARD_PRIO,NULL);
	if (daemon_id == -1){
		ACORAL_LOG_ERROR("Create Daem Thread Failed");
		exit(2);
	}	
	printf("%s",logo);

	system_sched_locked = false;
	system_need_sched = false; 
	system_set_running_thread(acoral_select_thread());
	HAL_SWITCH_TO(&acoral_cur_thread->stack);
}

void system_start()
{
	/* 关中断，在init线程中打开 */
	acoral_intr_disable();

	ACORAL_LOG_DEBUG("Build at %s %s",__DATE__,__TIME__);

    /* aCoral内核各模块初始化 */
	ACORAL_LOG_TRACE("Kernel Module Init Start");
    system_intr_module_init();
	system_mem_module_init();
	system_thread_module_init();
	ACORAL_LOG_TRACE("Kernel Module Init Done");

	main_cpu_start();
}