/**
 * @file resource.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief 资源管理
 * @version 1.0
 * @date 2024-03-28
 * @copyright Copyright (c) 2024
 */

#include "resource.h"
#include "thread.h"
#include "message.h"
#include "int.h"
#include "log.h"


acoral_res_pool_ctrl_t acoral_res_pool_ctrl_container[ACORAL_RES_UNKNOWN] = {
    {
        .type = ACORAL_RES_THREAD,
        .size = sizeof(acoral_thread_t),                     // TCB的大小
        .num_per_pool = (CFG_MAX_THREAD>20?20:CFG_MAX_THREAD), // 每个TCB池中的TCB数量
        .num = 0,                                            // 初始时没有创建 TCB 池
        .max_pools = CFG_MAX_THREAD/(CFG_MAX_THREAD>20?20:CFG_MAX_THREAD), // 最多允许创建TCB池的数量
        .free_pools = NULL,                                  
        .pools = NULL,                                      
        .list = {NULL , NULL},                              
    },
#if CFG_EVT_MUTEX || CFG_EVT_SEM
    {
        .type = ACORAL_RES_EVENT,
        .size = sizeof(acoral_evt_t),                  // event控制块ECB的大小
        .num_per_pool = 8,                             // 每个ECB池中的ECB数量
        .num = 0,                                      // 初始时没有创建ECB池
        .max_pools = 4,                                // 最多允许创建ECB池的数量
        .free_pools = NULL,                           
        .pools = NULL,                                 
        .list = {NULL , NULL},                         
    },
#endif

#if CFG_DRIVER
    acoral_driver_pool_ctrl,
#endif

#if CFG_MSG
    {
        .type = ACORAL_RES_MST,
        .size = sizeof(acoral_msgctr_t),            // 消息容器控制块的大小
        .num_per_pool = 10,                         // 每个消息容器控制块池中的消息容器控制块数量
        .num = 0,                                   // 初始时没有创建消息容器控制块池
        .max_pools = 4,                             // 最多允许创建消息容器控制块池的数量
        .free_pools = NULL,                         
        .pools = NULL,                              
        .list = {NULL , NULL},                      
    },

    {
        .type = ACORAL_RES_MSG,
        .size = sizeof(acoral_msg_t),               // 消息容器控制块的大小
        .num_per_pool = 10,                         // 每个消息容器控制块池中的消息容器控制块数量
        .num = 0,                                   // 初始时没有创建消息容器控制块池
        .max_pools = 4,                             // 最多允许创建消息容器控制块池的数量
        .free_pools = NULL,                         
        .pools = NULL,                              
        .list = {NULL , NULL},                      
    }
#endif

};

/// aCoral资源池数组，总共有ACORAL_MAX_POOLS=40个
acoral_pool_t acoral_res_pools[CFG_MAX_RES_POOLS];


///aCoral资源池管理系统最顶层数据结构，描述了系统中所有的资源池以及资源池控制块
acoral_res_system_t acoral_res_system = {
    .system_res_pools = acoral_res_pools,
    .system_free_res_pool = NULL,
    .system_res_ctrl_container = acoral_res_pool_ctrl_container,
};

/**
 * @brief 从acoral_res_pools中为某一资源池控制块分配一块资源池
 * @note 调用的时机包括系统刚初始化时，以及系统中空闲资源池不够时
 *
 * @param pool_ctrl 资源池控制块
 * @return int 0成功
 */
static int allocate_res_pool(acoral_res_pool_ctrl_t *pool_ctrl)
{
	acoral_pool_t *pool;
	if (pool_ctrl->num >= pool_ctrl->max_pools)
    {
        return ACORAL_RES_MAX_POOL;
    }

    acoral_enter_critical();
    pool = acoral_res_system.system_free_res_pool;
	if (pool != NULL)
	{
		acoral_res_system.system_free_res_pool = *(void **)pool->base_adr;
	}else
    {
        return ACORAL_RES_NO_POOL;
    }
	acoral_exit_critical();	

    /* 定义pool的类型 */
	pool->id = pool_ctrl->type << ACORAL_RES_TYPE_BIT | pool->id;
    
	pool->size = pool_ctrl->size;
	pool->num = pool_ctrl->num_per_pool;

    /* 从伙伴系统中拿到一个池子所有资源所需的内存 */
	pool->base_adr = (void *)acoral_malloc(pool->size * pool->num);
	if (pool->base_adr == NULL)
    {
        return ACORAL_RES_NO_MEM;
    }
		
	pool->res_free = pool->base_adr;
	pool->free_num = pool->num;
	acoral_pool_res_init(pool);
	acoral_list_add2_tail(&pool->ctrl_list, pool_ctrl->pools);
	acoral_list_add2_tail(&pool->free_list, pool_ctrl->free_pools);

	pool_ctrl->num++;
	return 0;
}

/**
 * @brief 某一资源池控制块归还其所有资源池的使用权
 *
 * @param pool_ctrl 资源池控制块
 */
static void release_res_pool(acoral_res_pool_ctrl_t *pool_ctrl)
{
	acoral_pool_t *pool;
	acoral_list_t *list, *head;
	head = pool_ctrl->pools;
	if (acoral_list_empty(head))
		return;
	for (list = head->next; list != head; list = list->next)
	{
		pool = list_entry(list, acoral_pool_t, free_list);
		acoral_list_del(&pool->ctrl_list);
		acoral_list_del(&pool->free_list);
		acoral_free(pool->base_adr);
		pool->base_adr = (void *)acoral_res_system.system_free_res_pool;

		/* 清除清除31到10位的内容，即该资源池的类型acoralResourceTypeEnum,只保留低9位的内容，即该资源池的在acoral_pools的编号 */
		pool->id = pool->id & ACORAL_POOL_INDEX_MASK;
		acoral_res_system.system_free_res_pool = pool;
	}
}

acoral_res_t *acoral_get_res(acoral_res_pool_ctrl_t *pool_ctrl)
{
	acoral_list_t *first;
	acoral_res_t *res;
	acoral_pool_t *pool;
	acoral_enter_critical();
	first = pool_ctrl->free_pools->next;
	if (acoral_list_empty(first))
	{
		if (allocate_res_pool(pool_ctrl))
		{
			acoral_exit_critical();
			return NULL;
		}
		else
		{
			first = pool_ctrl->free_pools->next;
		}
	}
	pool = list_entry(first, acoral_pool_t, free_list);
	res = (acoral_res_t *)pool->res_free;
	pool->res_free = (void *)((unsigned char *)pool->base_adr + res->next_id * pool->size);

	/* 修改被获取的资源的id，原本没被获取时，id第16位表示该资源在池内的编号，现在改到第14位//SPG这是为什么 */
	res->id = res->id & ACORAL_RES_INDEX_MASK | pool->id;
	pool->free_num--;
	if (!pool->free_num)
	{
		acoral_list_del(&pool->free_list);
	}
	acoral_exit_critical();
	return res;
}

void acoral_release_res(acoral_res_t *res)
{
	acoral_pool_t *pool;
	unsigned int index;
	void *tmp;
	acoral_res_pool_ctrl_t *pool_ctrl;
	if (res == NULL || acoral_get_res_by_id(res->id) != res)
	{
		return;
	}
	pool = acoral_get_pool_by_id(res->id);
	if (pool == NULL)
	{
		ACORAL_LOG_ERROR("Resource %d Release Error",res->id);
		return;
	}
	pool_ctrl = &(acoral_res_pool_ctrl_container[pool->type]);
	
	index = (((unsigned int)res - (unsigned int)pool->base_adr) / pool->size);
	if (index >= pool->num)
	{
		ACORAL_LOG_ERROR("Err Res");
		return;
	}
	tmp = pool->res_free;
	pool->res_free = (void *)res;
	res->id = index << ACORAL_RES_INDEX_BIT;
	res->next_id = ((acoral_res_t *)tmp)->id >> ACORAL_RES_INDEX_BIT;
	pool->free_num++;
	if (acoral_list_empty(&pool->free_list))
    {
        acoral_list_add(&pool->free_list, pool_ctrl->free_pools);
    }
		
	return;
}

acoral_pool_t *acoral_get_pool_by_id(int res_id)
{
	unsigned int index;
	index = res_id & ACORAL_POOL_INDEX_MASK;
	if (index < CFG_MAX_RES_POOLS)
	{
		return acoral_res_pools + index;
	}
		
	return NULL;
}

acoral_pool_t *acoral_get_free_pool()
{
	acoral_pool_t *tmp;
	acoral_enter_critical();
	tmp = acoral_res_system.system_free_res_pool;
	if (NULL != tmp)
	{
		acoral_res_system.system_free_res_pool = *(void **)tmp->base_adr;
	}
	acoral_exit_critical();
	return tmp;
}

acoral_res_t *acoral_get_res_by_id(int id)
{
	acoral_pool_t *pool;
	unsigned int index;
	pool = acoral_get_pool_by_id(id);
	if (pool == NULL)
		return NULL;
	index = (id & ACORAL_RES_INDEX_MASK) >> ACORAL_RES_INDEX_BIT;
	return (acoral_res_t *)((unsigned char *)pool->base_adr + index * pool->size);
}

void acoral_pool_res_init(acoral_pool_t *pool)
{
	acoral_res_t *res;
	unsigned int i;
	unsigned char *pblk;
	unsigned int blks;
	blks = pool->num;
	res = (acoral_res_t *)pool->base_adr;
	pblk = (unsigned char *)pool->base_adr + pool->size;
	for (i = 0; i < (blks - 1); i++)
	{
		res->id = i << ACORAL_RES_INDEX_BIT;
		res->next_id = i + 1;
		res = (acoral_res_t *)pblk;
		pblk += pool->size;
	}
	res->id = (blks - 1) << ACORAL_RES_INDEX_BIT; //SPG 括号是我加的，原本是没括号的，但是不对吧
	res->next_id = 0;
}

void acoral_pool_ctrl_init(acoral_res_pool_ctrl_t *pool_ctrl)
{
	unsigned int size;
	pool_ctrl->free_pools = &pool_ctrl->list[0];
	pool_ctrl->pools = &pool_ctrl->list[1];

	acoral_init_list(pool_ctrl->pools);
	acoral_init_list(pool_ctrl->free_pools);
    
	/* 调整资源池中资源的个数，以最大化利用分配的内存，详见绿书p144 */
	size = acoral_malloc_adjust_size(pool_ctrl->size * pool_ctrl->num_per_pool);
	if (size < pool_ctrl->size)
	{
		pool_ctrl->num_per_pool = 0;
	}
	else
	{
		pool_ctrl->num_per_pool = size / pool_ctrl->size;
		allocate_res_pool(pool_ctrl); // 先创建一个资源池，后面如果一个池子不够了，那在不超过这类资源的max_pool的条件下再创建新的池子
	}
}

void acoral_res_sys_init()
{
	acoral_pool_t *pool;
	unsigned int i;
	pool = &acoral_res_pools[0];
	for (i = 0; i < (CFG_MAX_RES_POOLS - 1); i++)
	{
		pool->base_adr = (void *)&acoral_res_pools[i + 1];
		pool->id = i;
		pool++;
	}
	pool->base_adr = (void *)0;
	acoral_res_system.system_free_res_pool = &acoral_res_pools[0];
}