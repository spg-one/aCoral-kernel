/**
 * @file core.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief aCoral内核初始化
 * @version 2.0
 * @date 2024-03-26
 * @copyright Copyright (c) 2024
 */
#ifndef ACORAL_CORE_H
#define ACORAL_CORE_H

volatile extern unsigned int acoral_start_sched;

#define DAEM_STACK_SIZE 256
#define IDLE_STACK_SIZE 128
#define INIT_STACK_SIZE 512

/**
 * @brief aCoral入口
 * 
 */
void system_start(void);

#endif
