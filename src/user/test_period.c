#include <stdio.h>
#include "acoral.h"
#include <assert.h>

void p1(){
    printf("in p1..\n");
    static int i = 0;
    i++;
    printf("i = %d , out p1..\n",i);
    return;
}

void test_period_thread(){
    acoral_period_policy_data_t p1data={
        .time=2000,
        .prio=21,
        .prio_type=ACORAL_HARD_PRIO
    };

    Assert(p1data.prio>30,"priority need to be greater than 30");

    acoral_create_thread(p1,0,NULL,"p1",NULL,ACORAL_SCHED_POLICY_PERIOD,&p1data);
}