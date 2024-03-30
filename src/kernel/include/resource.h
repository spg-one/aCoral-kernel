/**
 * @file resource.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief 资源管理
 * @version 1.0
 * @date 2024-03-28
 * @copyright Copyright (c) 2024
 */
#ifndef ACORAL_RESOURCE_H
#define ACORAL_RESOURCE_H

#include "autocfg.h"
#include "list.h"

#define ACORAL_POOL_INDEX_BIT 0
#define ACORAL_POOL_INDEX_MASK (0x3FF << ACORAL_POOL_INDEX_BIT)


#define ACORAL_RES_TYPE_BIT 10
#define ACORAL_RES_INDEX_BIT 16 ///<资源在资源池被创建后，初始的res->id的高16位表示该资源在资源池中的编号

#define ACORAL_RES_INDEX_MASK  (0xFF << ACORAL_RES_INDEX_BIT)
#define ACORAL_RES_TYPE_MASK   (0xF << ACORAL_RES_TYPE_BIT)

/**
 * @brief aCoral包含的资源类型
 * 
 */
typedef enum{
   ACORAL_RES_THREAD,

#if CFG_EVT_MUTEX || CFG_EVT_SEM
   ACORAL_RES_EVENT,
#endif

#if CFG_DRIVER
   ACORAL_RES_DRIVER,
#endif

#if CFG_MSG
   ACORAL_RES_MSG,
   ACORAL_RES_MST,
#endif

    ACORAL_RES_UNKNOWN
}acoralResourceTypeEnum;

/**
 * @brief aCoral资源管理系统函数返回值
 * 
 */
typedef enum{
   ACORAL_RES_NO_RES,
   ACORAL_RES_NO_POOL,
   ACORAL_RES_NO_MEM,
   ACORAL_RES_MAX_POOL
}acoralResourceReturnValEnum;

/**
 * @brief  资源池
*/
typedef struct {
   void *base_adr; ///<这个有两个作用，在为空闲的时候,它在acoral_free_res_pool这个数组中指向下一个pool，否则为它管理的资源的基地址
   void *res_free; ///<指向当前资源池中第一个空闲的资源
   int id;
   unsigned int size;
   unsigned int num;
   unsigned int position;
   unsigned int free_num;
   acoralResourceTypeEnum ctrl_type;
   acoral_list_t ctrl_list;
   acoral_list_t free_list;
}acoral_pool_t;

/**
 * @brief  资源池控制块
 * 
 */
typedef struct {
  unsigned int type;
  unsigned int size;            ///<size of one single resource eg.size of TCB
  unsigned int num_per_pool;    ///<the amount of resource in one pool eg.there are 20 TCBs in one TCB pool
  unsigned int num;             ///<the amount of pools which contain a certain type of resource(maybe TCB) in system at present will be added once one pool created; restrict by max_pools below;
  unsigned int max_pools;       ///<upbound of the amount of pools for this type. eg. the number of TCB pool limited to 2 because that there are at most 40 thread in system at one time and every TCB pool contains 20.
  acoral_list_t *free_pools;
  acoral_list_t *pools,list[2];
  void (*init);
}acoral_res_pool_ctrl_t;

/**
 * @brief aCoral资源管理系统顶层数据结构
 * 
 */
typedef struct {
    acoral_pool_t* system_res_pools;
    acoral_res_pool_ctrl_t* system_res_ctrl_container;
}acoral_res_system_t;

extern acoral_res_pool_ctrl_t acoral_res_pool_ctrl_container[ACORAL_RES_UNKNOWN];
extern acoral_pool_t acoral_res_pools[CFG_MAX_RES_POOLS];

#endif