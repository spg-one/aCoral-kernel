/**
 * @file sched.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，aCoral调度相关函数
 * @version 1.0
 * @date 2022-07-28
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-28 <td>Standardized
 *  </table>
 */

#include "hal.h"
#include "thread.h"
#include "int.h"
#include "list.h"
#include "bitops.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "log.h"

