#include "./include/hal_timer.h"
#include "autocfg.h"

#include "clint.h"

int hal_timer_init(int ticks_per_sec, void (*ticks_entry)(void *args), void *args){
	int result = -1;
	clint_timer_init();                           	/*这个主要用于将用于ticks的时钟初始化*/
	result = clint_timer_register(ticks_entry,args);	//SPG 这里不应该直接使用acoral_ticks_entry，因为这是kernel层函数，应该将其作为参数传进来
	if(result){
		return -1;
	}
    result = clint_timer_start(1000/ticks_per_sec,0);
	if(result){
		return -1;
	}
	return 0;
}
