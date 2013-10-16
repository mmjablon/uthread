#include "lib-ult.c" //the uthread library code is in lib-ult.c
#include <stdio.h>
#include <stdlib.h>

int n_threads=0;
int n_threads2=0;
int myid=0;
int myid2=10;

void do_something(){
    int id;
    id=myid;
    myid++;
    printf("This is ult %d\n", id);
    if(n_threads<5){
        uthread_create(do_something,1);
        n_threads++;
        uthread_create(do_something,1);
        n_threads++;
    }
    printf("This is ult %d again\n", id);
    uthread_yield(2);
    printf("This is ult %d one more time\n", id);
    uthread_exit();
}

void do_something2(){
    int id;
    id=myid2;
    myid2++;
    printf("This is ult %d\n", id);
    if(n_threads2<5){
        uthread_create(do_something2,5);
        n_threads2++;
        uthread_create(do_something2,5);
        n_threads2++;
    }
    printf("This is ult %d again\n", id);
    uthread_yield(2);
    printf("This is ult %d one more time\n", id);
    uthread_exit();
}

main() {
    int i;
    system_init(2); //only 1 kernel thread
    uthread_create(do_something,0);
    uthread_create(do_something2,5);
}