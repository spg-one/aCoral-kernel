/**
 * @file event.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，事件资源相关
 * @version 1.0
 * @date 2023-04-21
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2023-04-21 <td>Standardized
 *  </table>
 */

#include "event.h"
#include "mem.h"
#include "message.h"
#include "thread.h"
#include "list.h"
#include "resource.h"

#include <stdio.h>

void acoral_evt_init(acoral_evt_t *evt)
{
	acoral_init_list(&evt->wait_queue);
}

_Bool acoral_evt_queue_empty(acoral_evt_t *evt)
{
	return acoral_list_empty(&evt->wait_queue);
}

acoral_thread_t *acoral_evt_high_thread(acoral_evt_t *evt)
{
	acoral_list_t *head;
	acoral_thread_t *thread;
	head = &evt->wait_queue;
	if (acoral_list_empty(head))
		return NULL;
	thread = list_entry(head->next, acoral_thread_t, ipc_waiting_hook);
	return thread;
}

void acoral_evt_queue_add(acoral_evt_t *evt, acoral_thread_t *new)
{
	acoral_list_t *head, *tmp;
	acoral_thread_t *thread;
	new->evt = evt;
	head = &evt->wait_queue;
	for (tmp = head->next; tmp != head; tmp = tmp->next)
	{
		thread = list_entry(tmp, acoral_thread_t, ipc_waiting_hook);
		/*如果线程资源已经不在使用，即release状态则释放*/
		if (thread->prio > new->prio)
			break;

		if (tmp == tmp->next)
			break;
	}
	acoral_list_add(&new->ipc_waiting_hook, tmp->prev);
}

void acoral_evt_queue_del(acoral_thread_t *thread)
{
	acoral_list_del(&thread->ipc_waiting_hook);
	thread->evt = NULL;
}
