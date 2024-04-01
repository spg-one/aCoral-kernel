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

///pool->id[9:0]表示该资源池在acoral_res_pools中的编号
#define ACORAL_POOL_INDEX_BIT 0
#define ACORAL_POOL_INDEX_MASK (0x3FF << ACORAL_POOL_INDEX_BIT)

///res->id[13:10]表示该资源的类型（acoralResourceTypeEnum）
#define ACORAL_RES_TYPE_BIT 10
#define ACORAL_RES_TYPE_MASK   (0xF << ACORAL_RES_TYPE_BIT)

///res->id[23:16]表示该资源在本资源池中的编号
#define ACORAL_RES_INDEX_BIT 16 ///<资源在资源池被创建后，初始的res->id的高16位表示该资源在资源池中的编号
#define ACORAL_RES_INDEX_MASK  (0xFF << ACORAL_RES_INDEX_BIT)

///根据资源id获取资源类型
#define ACORAL_RES_TYPE(id) ((id&ACORAL_RES_TYPE_MASK)>>ACORAL_RES_TYPE_BIT)

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
   ACORAL_RES_MSG, ///<消息
   ACORAL_RES_MST,
#endif
    ACORAL_RES_UNKNOWN ///<未分配的资源池
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
 * @brief id虽然是32位的，但实际上只有高16位，因为低16位属于next_id，之所以用union的写法：
 *        1、是为了给id的低16位取个名字叫next_id。提高代码的可读性
 *        2、是为了将id直接作为资源的id，可以整体访问，如果用struct的方式，分段定义id的各个字段，那返回整个struct才是资源的id
 */
typedef union {
   int id;     
   unsigned short next_id; 
}acoral_res_t;

/**
 * @brief  资源池
*/
typedef struct {
   void *base_adr; ///< 在资源池未被未分配的时,在acoral_res_system.system_free_res_pool数组中指向下一个未被分配的资源池；分配后为该资源池管理的资源的基地址
   void *res_free; ///< 指向当前资源池中第一个空闲的资源
   int id; ///< bit[13:10]:资源类型；bit[9:0]:在acoral_res_pools中的编号
   unsigned int size; ///< 该资源池中每个资源的大小
   unsigned int num; ///< 资源池中资源的总数
   unsigned int free_num; ///< 资源池中未分配的资源个数
   acoralResourceTypeEnum type; ///< 该资源池的类型（acoralResourceTypeEnum）
   acoral_list_t ctrl_list; ///< 资源池创建后挂载到资源池控制块pools链表上的钩子
   acoral_list_t free_list; ///< 资源池中尚有未分配的资源时，挂载到资源池控制块free_pools链表的钩子
}acoral_pool_t;

/**
 * @brief  资源池控制块
 * 
 */
typedef struct {
  unsigned int type;            ///< 该资源池控制块所管理的资源类型（acoralResourceTypeEnum）
  unsigned int size;            ///< 该资源池控制块所管理的单个资源的大小
  unsigned int num_per_pool;    ///< 该资源池控制块管理的资源池中，每个资源池包含的资源数量
  unsigned int num;             ///< 该资源池控制块当前所管理的资源池数量
  unsigned int max_pools;       ///< 该资源池控制块最多可管理的资源池数量
  acoral_list_t *free_pools;    ///< 该资源池控制块当前管理的资源池中未满的资源池链表表头
  acoral_list_t *pools;         ///< 该资源池控制块管理的所有资源池的链表
  acoral_list_t list[2]; 
}acoral_res_pool_ctrl_t;

/**
 * @brief aCoral资源管理系统顶层数据结构
 * 
 */
typedef struct {
    acoral_pool_t* system_res_pools; ///<系统中所有的资源池
    int system_res_pools_bitmap[(CFG_MAX_RES_POOLS+31)/32]; ///<每一位0代表未分配，1代表已分配
    acoral_res_pool_ctrl_t* system_res_ctrl_container; ///<各类资源池控制块的容器
}acoral_res_system_t;

extern acoral_res_pool_ctrl_t acoral_res_pool_ctrl_container[ACORAL_RES_UNKNOWN];
extern acoral_pool_t acoral_res_pools[CFG_MAX_RES_POOLS];

/**
 * @brief 根据资源ID获取某一资源对应的资源池
 *
 * @param res_id 资源ID
 * @return acoral_pool_t* 获取到的资源池指针
 */
acoral_pool_t *acoral_get_pool_by_id(int id);


/**
 * @brief 从某个资源池中获取一个资源（tcb、event等）
 *
 * @param pool_ctrl 资源池控制块
 * @return acoral_res_t* 资源指针
 */
acoral_res_t *acoral_get_res(acoralResourceTypeEnum res_type);

/**
 * @brief 释放某一资源
 *
 * @param res 要释放的资源
 */
void acoral_release_res(acoral_res_t *res);

/**
 * @brief 根据id获取某一资源
 *
 * @param id 资源id
 * @return acoral_res_t* 获取到的资源
 */
acoral_res_t * acoral_get_res_by_id(int id);

/**
 * @brief 资源池中的资源id和next_id初始化
 *
 * @param pool 指定的资源池
 */
void acoral_pool_res_init(acoral_pool_t * pool);

/**
 * @brief 资源系统初始化
 * @note link all pools by making every pool's base_adr point to next pool
 *
 */
void acoral_res_sys_init(void);

/**
 * @brief 利用资源池控制块对某种类型的资源池进行初始化
 * @note 调用时机为系统启动后，每个子系统（驱动、事件、内存、线程）初始化的时候
 * @param pool_ctrl 每种资源池控制块
 */
void acoral_pool_ctrl_init(acoral_res_pool_ctrl_t *pool_ctrl);

#endif