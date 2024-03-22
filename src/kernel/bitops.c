/**
 * @file bitops.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief 位图操作
 * @version 2.0
 * @date 2024-03-22
 * @copyright Copyright (c) 2024
 */

#include "bitops.h" 

unsigned int acoral_ffs(unsigned int word)
{
	unsigned int k;
	k = 31;
	if (word & 0x0000ffff) { k -= 16; word <<= 16; }
	if (word & 0x00ff0000) { k -= 8;  word <<= 8;  }
	if (word & 0x0f000000) { k -= 4;  word <<= 4;  }
	if (word & 0x30000000) { k -= 2;  word <<= 2;  }
	if (word & 0x40000000) { k -= 1; }
        return k;
}

unsigned int acoral_find_first_bit(const unsigned int *b,unsigned int length)
{
	unsigned int v;
	unsigned int off;

	for (off = 0; v = b[off], off < length; off++) {
		if (v)
			break;
	}
	return acoral_ffs(v) + off * 32;
}

void acoral_set_bit(int nr,unsigned int *bitmap)
{
	unsigned int oldval, mask = 1UL << (nr & 31);
	unsigned int *p;
	p =bitmap+(nr>>5);
	oldval = *p;
	*p = oldval | mask;
}

void acoral_clear_bit(int nr,unsigned int *bitmap)
{
	unsigned int oldval, mask = 1UL << (nr & 31);
	unsigned int *p;
	p =bitmap+(nr >> 5);
	oldval = *p;
	*p = oldval &(~mask);
}

unsigned int acoral_get_bit(int nr,unsigned int *bitmap)
{
	unsigned int oldval, mask = 1UL << (nr & 31);
	unsigned int *p;
	p =bitmap+(nr>>5);
	oldval = *p;
	return oldval & mask;
}
