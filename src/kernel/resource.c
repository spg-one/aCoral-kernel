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



acoral_res_pool_ctrl_t acoral_res_pool_ctrl_container[ACORAL_RES_UNKNOWN] = {
    {
        .type = ACORAL_RES_THREAD,
        .size = sizeof(acoral_thread_t),                     // TCB的大小
        .num_per_pool = (CFG_MAX_THREAD>20?20:CFG_MAX_THREAD), // 每个TCB池中的TCB数量
        .num = 0,                                            // 初始时没有创建 TCB 池
        .max_pools = CFG_MAX_THREAD/(CFG_MAX_THREAD>20?20:CFG_MAX_THREAD), // 最多允许创建TCB池的数量
        .free_pools = NULL,                                  // 初始化为空
        .pools = NULL,                                       // 初始化为空
        .list = {NULL , NULL},                               // 初始化为空
        .init = acoral_pool_ctrl_init
    },
#if CFG_EVT_MUTEX || CFG_EVT_SEM
    {
        .type = ACORAL_RES_EVENT,
        .size = sizeof(acoral_evt_t),                  // event控制块ECB的大小
        .num_per_pool = 8,                             // 每个ECB池中的ECB数量
        .num = 0,                                      // 初始时没有创建ECB池
        .max_pools = 4,                                // 最多允许创建ECB池的数量
        .free_pools = NULL,                            // 初始化为空
        .pools = NULL,                                 // 初始化为空
        .list = {NULL , NULL},                         // 初始化为空
        .init = acoral_pool_ctrl_init
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
        .free_pools = NULL,                         // 初始化为空
        .pools = NULL,                              // 初始化为空
        .list = {NULL , NULL},                      // 初始化为空
        .init = acoral_pool_ctrl_init
    },

    {
        .type = ACORAL_RES_MSG,
        .size = sizeof(acoral_msg_t),               // 消息容器控制块的大小
        .num_per_pool = 10,                         // 每个消息容器控制块池中的消息容器控制块数量
        .num = 0,                                   // 初始时没有创建消息容器控制块池
        .max_pools = 4,                             // 最多允许创建消息容器控制块池的数量
        .free_pools = NULL,                         // 初始化为空
        .pools = NULL,                              // 初始化为空
        .list = {NULL , NULL},                      // 初始化为空
        .init = acoral_pool_ctrl_init
    }
#endif

};

/// aCoral资源池数组，总共有ACORAL_MAX_POOLS=40个
acoral_pool_t acoral_res_pools[CFG_MAX_RES_POOLS];

/// @brief aCoral资源池管理系统最顶层数据结构，描述了系统中所有的资源池以及资源池控制块
acoral_res_system_t acoral_res_system = {
    .system_res_pools = acoral_res_pools,
    .system_res_ctrl_container = acoral_res_pool_ctrl_container
};