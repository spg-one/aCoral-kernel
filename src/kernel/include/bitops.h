/**
 * @file bitops.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief 位图操作
 * @version 2.0
 * @date 2024-03-22
 * @copyright Copyright (c) 2024
 */


#ifndef ACORAL_BITOPS_H
#define ACORAL_BITOPS_H

/**
 * @brief 查找长度为length的整型数组中，最低非0bit的位置。
 *        bit从低到高排序为 b[0]:bit0 -> b[0]:bit31 -> b[1]:bit[0] -> ... b[length-1]:bit31
 * @param b 整型数组指针
 * @param length 数组长度
 * @param bit 找0还是1
 * @return int 最低非0bit的位置
 */
unsigned int acoral_find_first_bit_in_array(const unsigned int *b,unsigned int length, int bit);

/**
 * @brief 设置位图中特定位为1
 * 
 * @param nr 要设置的位的位置
 * @param p 位图
 */
void acoral_set_bit_in_bitmap(int nr,unsigned int *p);

/**
 * @brief 设置位图中特定位为0
 * 
 * @param nr 要设置的位的位置
 * @param p 位图
 */
void acoral_clear_bit_in_bitmap(int nr,unsigned int *p);

/**
 * @brief 查找位图中的一位
 * 
 * @param nr 要查找的位的位置
 * @param bitmap 位图
 * @return int 值
 */
unsigned int acoral_get_bit_in_bitmap(int nr,unsigned int *bitmap);

/**
 * @brief 二分法查找整型数为1的最低位
 * 
 * @param word 整型数
 * @param bit 找0还是1
 * @return int 位置
 */
unsigned int acoral_find_first_bit_in_integer(unsigned int word, int bit);

#endif
