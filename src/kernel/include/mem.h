/**
 * @file mem.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，内存相关头文件
 * @version 1.1
 * @date 2023-04-20
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-08 <td>Standardized 
 *   <tr><td> 1.1 <td>王彬浩 <td> 2023-04-20 <td>optimized 
 *  </table>
 */
#ifndef ACORAL_MEM_H
#define ACORAL_MEM_H
#include "autocfg.h"
#include "hal.h"
#include "core.h"
#include "list.h"
#include "resource.h"

/**
 * 伙伴系统部分
*/

/**
 * @brief 伙伴系统初始化
 *
 * @param start_adr
 * @param end_adr
 * @return unsigned int
 */
unsigned int buddy_init(unsigned int start, unsigned int end);

/**
 * @brief 伙伴系统分配内存
 *
 * @param size 用户需要的大小
 * @return void* 返回分配的地址
 */
void* buddy_malloc(unsigned int  size);


/**
 * @brief 伙伴系统回收内存
 *
 * @param ptr 要回收的地址
 */
void buddy_free(void *p);

/**
 * @brief 伙伴系统扫描，查看是否有空闲块
 *
 */
void buddy_scan(void);

#define acoral_malloc(size) buddy_malloc(size)
#define acoral_free(ptr) buddy_free(ptr)
#define acoral_malloc_adjust_size(size) buddy_malloc_size(size)
#define acoral_mem_init(start,end) buddy_init(start,end)
#define acoral_mem_scan() buddy_scan()

#ifdef CFG_MEM2 
/**
 * @brief 任意大小内存分配
 * 
 */
   void * v_malloc(int size);

/**
 * @brief 任意大小内存释放
 * 
 */
   void v_free(void * p);

/**
 * @brief 任意大小内存分配系统初始化。从伙伴系统中拿出一部分内存，用作任意大小分配的内存
 * 
 */
   void v_mem_init(void);
   void v_mem_scan(void);
   #define acoral_mem_init2() v_mem_init()
   #define acoral_malloc2(size) v_malloc(size) //SPG有问题
   #define acoral_free2(p) v_free(p)
   #define acoral_mem_scan2() v_mem_scan()
#endif

#define LEVEL 14                          ///<最大层数
#define BLOCK_INDEX(index) ((index)>>1)   ///<bitmap的index换算，因为除去最大内存块的剩余层中64块用一个32位图表示，所以要除以2
#define BLOCK_SHIFT 7                     ///<基本内存块偏移量
#define BASIC_BLOCK_SIZE (1<<BLOCK_SHIFT) ///<基本内存块大小 128B

#define MAGIC 0xcc
#define MAGIC_MASK 0xfe
#define USED 1
#define FREE 0 
#define USETAG_MASK 0x1
#define SIZE_MASK 0xffffff00
#define SIZE_BIT 8
#define BLOCK_CHECK(value) (((value&MAGIC_MASK)==MAGIC))
#define BLOCK_SIZE(value) ((value&SIZE_MASK)>>SIZE_BIT)
#define BLOCK_TAG(value) (value&USETAG_MASK)
#define BLOCK_FREE(value) (BLOCK_TAG(value)==FREE)
#define BLOCK_USED(value) (BLOCK_TAG(value)==USED)
#define BLOCK_SET_USED(ptr,size) (*ptr=(size<<SIZE_BIT)|0x1|MAGIC)
#define BLOCK_SET_FREE(ptr,size) (*ptr=(size<<SIZE_BIT)|MAGIC)
#define BLOCK_CLEAR(ptr) (*ptr=0)

typedef enum{
   MEM_NO_ALLOC,  ///<内存系统状态定义：容量太小不可分配
   MEM_OK         ///<内存系统状态定义：容量足够可以分配
}acoralMemAllocStateEnum;

/**
 * @brief 内存块层数结构体，用于回收内存块时，描述要回收的内存块的大小。
 * 因为知道回收的起始地址，所以就知道要回收的这块内存的第一块基本内存块的编号是多少，那只要知道了这个基本内存块是哪一层分配出去的，就知道实际分配了多少大小的内存。
 * 详见书p131
 * 
 */
typedef struct{
	char level;
}acoral_block_t; //SPG 这个结构有必要？

/**
 * @brief 内存控制块结构体
 * 
 */
typedef struct{
	int *free_list[LEVEL];  ///<各层空闲位图链表
	unsigned int *bitmap[LEVEL];    ///<各层内存状态位图块，两种情况：一. 最大内存块层，为一块内存空闲与否；二.其余层，1 标识两块相邻内存块有一块空闲，0 标识没有空闲
	int free_cur[LEVEL];    ///<各层空闲位图链表头
	unsigned int num[LEVEL];        ///<各层内存块个数
	char level;               ///<层数 
	unsigned char state;              ///<状态
	unsigned int start_adr;         ///<内存起始地址
	unsigned int end_adr;           ///<内存终止地址
	unsigned int block_num;         ///<基本内存块数
	unsigned int free_num;          ///<空闲基本内存块数
	unsigned int block_size;        ///<基本内存块大小
}acoral_block_ctr_t;

/**
 * @brief 内存管理系统初始化
 * @note 初始化两级内存管理系统，第一级为伙伴系统，第二级为任意大小内存分配系统（名字里带"2")和资源池系统
 */
void system_mem_module_init();


/**
 * @brief 用户指定的内存大小不一定合适，可以先用这个函数进行一下调整
 *
 * @param size 用户想使用内存的大小
 * @return unsigned int 实际将要分配的内存大小
 */
unsigned int buddy_malloc_size(unsigned int size);



#endif
