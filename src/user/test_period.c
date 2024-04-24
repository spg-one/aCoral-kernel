#include <stdio.h>
#include "acoral.h"

void p1(){
    printf("in p1..\n");
    static int i = 0;
    i++;
    printf("i = %d , out p1..\n",i);
    return;
}

void test_period_thread(){
    acoral_period_policy_data_t p1data={
        .period_time_mm = 2000
    };

    acoral_create_thread("p1",p1,NULL,0,ACORAL_SCHED_POLICY_PERIOD,21,ACORAL_HARD_PRIO,&p1data);
}