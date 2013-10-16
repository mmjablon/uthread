//
//  lib-ult.c
//
//  Created by Megan Jablonski on 10/10/13.
//  Copyright (c) 2013 Megan Jablonski. All rights reserved.
//


/*
 compile with flag -lpthread
 in order to use sem_init, sem_wait and sem_post functions
 */

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <ucontext.h>

/**** Queue code ****/

//define a queue of user thread records
struct thread_info{
	int pri_num;
	struct thread_info *next;
	ucontext_t *context;
};

//declare the queue functions
void insert(struct thread_info *th);
void printQueue();
void deallocate();

struct thread_info *head=NULL, *tail=NULL;
int num_kernel_threads=0;
struct thread_info *toDeallocate=NULL;

void insert(struct thread_info *th){
	if(head!=NULL && head->pri_num <= th->pri_num){
        struct thread_info *current_node = head;
        while(current_node->next != NULL && current_node->next->pri_num <= th->pri_num){
            current_node = current_node->next;
        }
        
        th->next = current_node->next;
		current_node->next = th;
	}else if(head==NULL){
		head = th;
		tail = th;
	}else{
		th->next = head;
		head = th;
    }
    
}

void deallocate(){
    struct thread_info *current_node;
    while(toDeallocate!= NULL){
        current_node=toDeallocate;
        toDeallocate=current_node->next;

        free(current_node->context->uc_stack.ss_sp);
        free(current_node->context);
        free(current_node);
    }
}

//a function to print the queue for debugging purposes
void printQueue(){
	struct thread_info *th = head;
	while(th != NULL){
		printf("%i, ", th->pri_num);
		th = th->next;
	}
	printf("\n");
}

/**** Thread library code ****/
typedef enum { false, true } bool;
bool initialized = false;
int max_num_kernel_threads=0;

sem_t queue_lock;
sem_t kernel_lock;

//declare the function used to create a user thread
int uthread_create(void func(), int pri_num);

/*
 This function is called before any other uthread library functions can be called.
 You may initializes the uthread system (for example, data structures) in this function.
 Parameter max_number_of_klt specifies the maximum number of kernel threads (created
 by calling clone()) that can be used to use to run the user-level threads created by this library.
 max_num_of_klt should be no less than 1.
 */

void system_init(int max_number_of_klt)
{
	if(max_number_of_klt < 1){
		fprintf(stderr, "The system must be initialized to have 1 or more kernel threads.\n");
		exit(EXIT_FAILURE);
	}
	if(initialized == false){
		initialized = true;
		max_num_kernel_threads = max_number_of_klt;
		sem_init(&queue_lock,0,1);
		sem_init(&kernel_lock,0,1);
	}else{
		fprintf(stderr, "The uthread system has already been initialized.\n");
		exit(EXIT_FAILURE);
	}
}

//code run by a kernel thread
int kernel_thread(void *arg)
{
	struct thread_info *th;
	ucontext_t *context;
    
	//fetch a user thread from the queue
	sem_wait(&queue_lock);
    
	/*when testing I found that sometimes all the user threads can run
     on one of the kernel threads before the second kernel thread even gets started.
     To handle this I simply kill the tread if the queue is empty.*/
    
	if(head==NULL){
		sem_post(&queue_lock);
		num_kernel_threads--;
		exit(EXIT_SUCCESS);
	}else{
		th = head;
		context=head->context;
		head=head->next;
        if(head==NULL) tail=NULL;
        
        //add the context object to the queue to be destoryed
        th->next = toDeallocate;
        toDeallocate = th;
		sem_post(&queue_lock);
        
		//run the user thread
		setcontext(context);
	}
}

/*
 This function creates a new user-level thread which runs func() without any argument.
 The thread is associated with a priority number “pri_num”. This function returns 0
 if succeeds, or -1 otherwise.
 */
int uthread_create(void func(), int pri_num)
{
	if(initialized == false){
		fprintf(stderr, "The uthread system has not been initialized.\n");
		return -1;
	}
    
	//construct a thread record
	struct thread_info *th;
	th=(struct thread_info *)malloc(sizeof(struct thread_info));
	th->next=NULL;
	th->pri_num=pri_num;
    
	th->context = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(th->context);
	th->context->uc_stack.ss_sp=malloc(16384);
	th->context->uc_stack.ss_size=16384;
	makecontext(th->context, func, 0);
    
	//add the thread record into the queue
	sem_wait(&queue_lock);
	insert(th);
	sem_wait(&kernel_lock);
    
	//create a kernel thread to run the user threads if there is no kernel thread yet
	if(num_kernel_threads < max_num_kernel_threads){
		num_kernel_threads++;
		void *child_stack;
		child_stack=(void *)malloc(16384); child_stack+=16383;
		sem_post(&queue_lock);
		clone(kernel_thread, child_stack, CLONE_VM|CLONE_FILES, NULL);
	}else{
		sem_post(&queue_lock);
	}
    
	sem_post(&kernel_lock);
	return 0;
}

/*
 The calling thread requests to yield the kernel thread that it is currently running,
 and its priority number is changed to “pri_num”. The scheduler implemented by the
 library needs to decide which thread should be run next on the yielded kernel thread.
 This function returns 0 unless something wrong occurs.
 */
int uthread_yield(int pri_num){
	if(initialized == false){
		fprintf(stderr, "The uthread system has not been initialized.\n");
		return -1;
	}
    
	int success = 0;
	if(head==NULL || head->pri_num > pri_num){
		//do nothing, next thread in queue so keep running
	}else{
		//construct a new thread record
		struct thread_info *th;
		th=(struct thread_info *)malloc(sizeof(struct thread_info));
		th->next=NULL;
		th->pri_num=pri_num;
        
		th->context = (ucontext_t*) malloc(sizeof(ucontext_t));
		getcontext(th->context);
		th->context->uc_stack.ss_sp=malloc(16384);
		th->context->uc_stack.ss_size=16384;
		
		//place thread in the queue
		sem_wait(&queue_lock);
		insert(th);
		
		struct thread_info *next;
		ucontext_t *context;
		//fetch a user thread from the queue
		next=head;
		context=head->context;
		head=head->next;
		if(head==NULL) tail=NULL;
        
        next->next = toDeallocate;
        toDeallocate = next;
        
		sem_post(&queue_lock);
		
		success = swapcontext(th->context, context);
	}
	return success;
}

/*
 The calling user-level thread ends its execution. This function should pick one
 of the ready threads to run on the kernel thread previously held by the calling
 thread. If there is no any other user threads ready to run, the kernel thread should exit.
 */
void uthread_exit(){
	if(initialized == false){
		fprintf(stderr, "The uthread system has not been initialized.\n");
		exit(EXIT_FAILURE);
	}
    
	sem_wait(&queue_lock);
	if(head==NULL){
		sem_wait(&kernel_lock);
		//last thread, exit kernel thread
		num_kernel_threads--;
		sem_post(&kernel_lock);
		sem_post(&queue_lock);
        deallocate();
		exit(EXIT_SUCCESS);
	}else{
		ucontext_t *context;
		struct thread_info *th;
        
		//fetch a user thread from the queue
		context=head->context;
		th=head;
		head=head->next;
		if(head==NULL) tail=NULL;
		deallocate();
        //add the context object to the queue to be destoryed
        th->next = toDeallocate;
        toDeallocate = th;
		sem_post(&queue_lock);
        
		setcontext(context);
	}
}