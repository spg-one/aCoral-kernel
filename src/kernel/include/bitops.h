/**
 * @file bitops.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief lib层
 * @version 1.0
 * @date 2023-04-19
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2023-04-19 <td>Standardized 
 *  </table>
 */


#ifndef ACORAL_BITOPS_H
#define ACORAL_BITOPS_H

/**
 * @brief 查找长度为length的整型数组中，最低非0bit的位置。
 *        bit从低到高排序为 b[0]:bit0 -> b[0]:bit31 -> b[1]:bit[0] -> ... b[length-1]:bit31
 * @param b 整型数组指针
 * @param length 数组长度
 * @return 最低非0bit的位置
 */
unsigned int acoral_find_first_bit(const unsigned int *b,unsigned int length);

/**
 * @brief 设置位图中特定位为1
 * 
 * @param nr 要设置的位的位置
 * @param p 位图
 */
void acoral_set_bit(int nr,unsigned int *p);
void acoral_clear_bit(int nr,unsigned int *p);
unsigned int acoral_ffs(unsigned int word);
unsigned int acoral_get_bit(int nr,unsigned int *bitmap);
#endif
