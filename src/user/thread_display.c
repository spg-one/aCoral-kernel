#include "thread.h"
#include "policy.h"
#include "hal.h"
#include "shell.h"
#include <stdio.h>

void display_thread_new(int argc,char **argv){	
	acoral_list_t *head,*tmp;
	acoral_thread_t * thread;
    acoral_list_t *list;
    acoral_res_t *res;
    acoral_pool_t* pool;

	head = acoral_res_system.system_res_ctrl_container[ACORAL_RES_THREAD].pools;
	if (acoral_list_empty(head))
	{
        return;
    }
    
    printf("\t\tSystem Thread Information\r\n");
	printf("------------------------------------------------------\r\n");
	printf("Name\t\tid\t\tType\t\tState\t\tPrio\r\n");
	HAL_ENTER_CRITICAL();

	for (list = head->next; list != head; list = list->next)
	{
        pool = list_entry(list,acoral_pool_t,ctrl_list);
        for(int i =0 ; i<pool->num ; i++){
            res = (acoral_res_t*)(pool->base_adr + pool->size * i);
            if(ACORAL_RES_TYPE(res->id) == ACORAL_RES_THREAD){
                thread=list_entry(res,acoral_thread_t,res);
                printf("%s\t\t",thread->name);
		        printf("%d\t\t",thread->res.id);
		        switch(thread->policy){
			        case ACORAL_SCHED_POLICY_COMM:
				        printf("Common\t\t");
				        break;
			        case ACORAL_SCHED_POLICY_PERIOD:
				        printf("Period\t\t");
				        break;
			        default:
				        break;
		        }
		        if(thread->state&ACORAL_THREAD_STATE_RUNNING)
				    printf("Running\t\t");
		        else if(thread->state&ACORAL_THREAD_STATE_READY)
				    printf("Ready\t\t");
		        else if(thread->state&ACORAL_THREAD_STATE_DELAY)
				    printf("Delay\t\t");
		        else if(thread->state&ACORAL_THREAD_STATE_SUSPEND)
				    printf("Sleep\t\t");
		        else if(thread->state==ACORAL_THREAD_STATE_EXIT)
				    printf("Freeze\t\t");
		        else
				    printf("Error\t\t");
		
		        printf("%d\t\t",thread->prio);
		        printf("\r\n");
            }
        }
    }
	
	printf("------------------------------------------------------\r\n");

	HAL_EXIT_CRITICAL();
}

acoral_shell_cmd_t dt_cmd={
	"dt",
	(void*)display_thread_new,
	"View all thread info",
	NULL
};
