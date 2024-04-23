/**
 * @file comm_thrd.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，普通（先来先服务线程）策略线程
 * @version 2.0
 * @date 2024-03-22
 * @copyright Copyright (c) 2024
 */
#ifndef ACORAL_COMM_THRD_H
#define ACORAL_COMM_THRD_H

#include "thread.h"

/**
 * @brief 普通线程调度相关的数据
 * 
 */

/**
 * @brief 注册普通机制
 * @note 调用时机为系统初始化阶段
 */
void comm_policy_init();

#endif
