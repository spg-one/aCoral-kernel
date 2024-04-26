#include <stdio.h>
#include "acoral.h"
#include "user.h"

static int thread_c1; 
void c1(){
    printf("in c1..\n");
    int i = 1;
    while(i){
        i++;
        printf("i = %d\n",i);
        if(i >= 10){
            acoral_suspend_self();
            break;
        }
    }
    while(i){
        i++;
        printf("i = %d\n",i);
        if(i >= 20){
            acoral_suspend_self();
        }
    }
}

void c2(){
    printf("in c2..\n");
    acoral_resume_thread_by_id(thread_c1);
}

void c3(){
     printf("in c3..\n");
    int i = 1;
    while(i){
        i++;
        printf("i = %d\n",i);
        if(i == 10){
            acoral_delay_self(5000);
            printf("out c3..\n");
            break;
        }
    }
}

void test_comm_thread(){
    // thread_c1 = acoral_create_thread("c1",c1,NULL,0,ACORAL_SCHED_POLICY_COMM,20,ACORAL_HARD_PRIO,NULL);
    // acoral_create_thread("c2",c2,NULL,0,ACORAL_SCHED_POLICY_COMM,22,ACORAL_HARD_PRIO,NULL);
    acoral_create_thread("c3",c3,NULL,0,ACORAL_SCHED_POLICY_COMM,25,ACORAL_HARD_PRIO,NULL);
}