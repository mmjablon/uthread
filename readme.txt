ComS 352
Proj 1

For purpose of the course it is all implemented in lib-ult.c.

To stick as closely to running with the given test example run with:

gcc -pthread example.c -o test
./example

uthread is a c user-level threading library with many to many mapping between user and kernel threads. It implements nonpreemptive priority-based scheduling between user and kernel threads. To break a tie, the thread that has waited the longest runs.
u thread library has the following functions:

(1)void system_init(int max_number_of_klt)
	This function is called before any other uthread library functions can be called. You may initializes the uthread system (for example, data structures) in this function. Parameter max_number_of_klt specifies the maximum number of kernel threads (created by calling clone()) that can be used to use to run the user-level threads created by this library. max_num_of_klt should be no less than 1.

(2)int uthread_create(void func(), int pri_num)	This function creates a new user-level thread which runs func() without any argument. The thread is associated with a priority number “pri_num”. This function returns 0 if succeeds, or -1 otherwise.(3)int uthread_yield(int pri_num)	The calling thread requests to yield the kernel thread that it is currently running, and its priority number is changed to “pri_num”. The scheduler implemented by the library needs to decide which thread should be run next on the yielded kernel thread. This function returns 0 unless something wrong occurs.
   (4) void uthread_exit()The calling user-level thread ends its execution. This function should pick one of the ready threads to run on the kernel thread previously held by the calling thread. If there is no any other user threads ready to run, the kernel thread should exit.

Tested on pyrite.