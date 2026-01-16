# Project 2 - Operating Systems

Essentially the Project is split into 2 parts: 
1. Create a new syscall "int getpinfo(struct pstat*)" and create a program "ps.c" similar to the linux "ps" command
2. Make changes to the CPU scheduler of the kernel based on the instructions

# Part 1: Syscall "getpinfo" and Program "ps.c"

## The struct pstat and the syscall getpinfo

The new syscall that we are instructed to implement has a prototype "int getpinfo(struct pstat*)". As in all system
calls, it returns 0 on success and some negative value (in our case it's -1) on failure.

In order to implement the syscall, we first need to define the struct pstat:

struct pstat {
  int pid[NPROC];
  int ppid[NPROC];
  int state[NPROC];
  int priority[NPROC];
  uint64 sz[NPROC];
  char name[NPROC][16];
  int used[NPROC];
};

This struct keeps track of the following:
1. The process id of each active process' table slot
2. The process id of the processe's parent
3. The state of each process (RUNNING, RUNNABLE, UNUSED etc.)
4. The priority of each process (a number in [0,3] introduced in the Project's instructions)
5. The size of the process's memory in bytes
6. The name of each process (up to 15 characters long)
7. A flag indicating whether the corresponding process table slot is in use (i.e., not in the UNUSED state).

What the syscall needs to do is get the info of all the current running processes in the system and pass the info to the user.
In order to achieve this, in our implementation we do the following, as is written in "sysproc.c":

1. We create a struct pstat inside kernel space
2. We fetch the user address from the syscall
3. We initialize the kernel buffer using memset, in order to make sure all the contents of the struct are clesn
4. We copy the info of the processes into the buffer inside a mutex in order to avoid race conditions and interrupts
5. We copy the buffer from kernel memory to user space

Return 0 on success and -1 on fail

## The program ps.c

The program "ps.c" is a very simple program in my implementation. What it does is it just makes use of the new syscall
that was just created and prints all the currently running processes to the screen. When we mean "all currently running
processes" we mean the ones inside the xv6-riscv virtual machine and not the processes running in the user's system.

More specifically, for each process it prints:

1. The process id
2. The process id of its parent
3. The priority of the process
4. The state in which it curretly is
5. The size of the process in bytes
5. The name of the process

All this information is collected in ps.c using the syscall that was just created.

## Technical details and comments

For this part of the Project, the following files were modified:

1. proc.h - definition of struct proc
2. sysproc.c - implementation of syscall
3. usys.pl - entry addition for the new syscall
4. syscall.h - syscall number for the new syscall
5. syscall.c - function prototype and insertion to the table of syscalls
6. ps.c - implementation of the program "ps" using the new syscall
7. user.h - function prototype for the new syscall
8. Makefile - addition of the new program "ps" to user runnable programs

# Part 2: Changes to the scheduler of the kernel

## Helper functions :

For this part of the project we have introduced 3 helper functions and have made changes 
to the kernel's scheduler. The helper functions are the following:

1. static int mlfq_quantum(int priority);
This function defines how we set priorities to the running processes. We have
3 levels of priorities which are described by timer-ticks according to the project's instructions.

2. static int mlfq_exists_higher_runnable(int current_priority);
This function checks whether there exists a program with a higher priority than the one that is
currently being ran and is using the CPU.

3. int mlfq_tick(void)
This function is called on each timer tick and is used to decide when a process should yield the CPU.

## Implementation of the rules set in the Project's instructions.

We are going to walk over the 11 rules set in the instructions of the 
project and talk about how we have implemented each one.

1. 4 priority levels [0,3]: We achieve this because the mlfq_quantum array has 4 entries and we use
priority and loops with level<4 everywhere

2. In every 10ms tick of xv6, the higher priority process should be ran:
We do this using mlfq_tick, if there exists a higher priority process which is marked as "RUNNABLE", then the function
returns 1. In trap.c, when mlfq_tick returns 1 then the CPU is yielded. Then, the scheduler runs the process from level 0.

3. Whenever the current running process either terminates/blocks/sleeps/yields, the process that is ran is
the one of the highest priority:
Whenever a running process changes state and performs a context switch back, then it is the scheduler's duty
to decide again which process is going to run next. Since the scheduler is designed in such a way that it always
selects the process of the highest priority to be executed, we can confidently say that we cover this instruction.

4. When more than 1 processes are of the same priority, then we perform Round-robin selection:
Inside the scheduler, we perform a scan in each round-robin level, and then we update the cursor to go
to the next available slot. What this means is that, for example let's say we have 3 processes of priority 0, 
these processes exist in slots 2, 5, and 7. At first we run process 2, then the cursor points to the next slot etc.
At each iteration, the cursor moves ahead by one slot. The cursor will first reach slot 5, running the process 
in this slot, and then it will reach slot 7 running the process in that slot. The round robin technique guarantees
fairness to all processes that are going to run.

5. Every tick counts as a full CPU tick:
In the code, inside mlfq_tick we have: current->qticks++; in every timer tick.

6. We should split the priority quanta into 4 different ones, according to the ticks: 
This is implemented inside mlfq_quantum, where we define the array: quantum[4] = {4,8,16,32},
where 4,8,16,32 respresent timer ticks. These numbers are decided based on the instructions.

7. Every new process starts at priority level 0:
Inside allocproc in proc.c, the function that allocates the CPU to new processes we do:
found:
  ...
  p->priority = 0;   // new process starts at highest queue
  ...

8. If no higher prority process exists and the current running process doesn't yield, then the current
running process runs for its whole time share:
Inside mlfq_tick, we return 1 (meaning that we preempt the process and the scheduler should be called) when
either the process has been demoted (quantum ended) or there exists a runnable process with a higher priority.
This means that if none of these conditions are met, the current process just continuous running.

Let's look at an example:
Let's say we have 2 processes, A (PID: 1, PPRIO: 0) and B (PID: 2, PPRIO 3).
When A is ran, it has the highest priority. After it consumes 4 CPU ticks, it gets demoted to priority 1.
This means that mlfq_tick returns 1 since a demotion happened. Hence, at this point, the process has yielded the 
CPU and the scheduler should decide what process needs to be ran. When the scheduler checks what process needs to be ran,
it selects the process with the highest priority, in this case process A (PID: 1, PPRIO 1). When process A eventually
reaches priority level 3, same as B, it is no longer favoured over B by the CPU and the decision on which process
is to be ran each time happens according to the round robin of the scheduler.

9. In priorities 0,1,2, when a quantum expires, the process's priority gets demoted.
Inside the code in proc.c/mlfq_tick we do:
..
if(current->qticks >= quantum && current_priority < 3){
  // Time slice finished at this level -> demote to next (lower) queue.
  current->priority++;
  ..
}

10. If a process voluntarily yields the CPU before quantum expiration, then the quantum does not reset:
The quantum (qticks) resets only in one of 2 cases: a - if there is a demotion (quantum expires) and 
b - if there is a priority boost due to aging. It does not reset when it yields or when it becomes RUNNABLE.
What this means is that, let's say a process runs for 2 ticks and yields the CPU voluntarily. Internally, 
it will have qticks = 2 (and so remaining time is quantum - 2). This stays as is when the process is not
using the CPU, meaning that the next time it is to run, it will continue counting from 2 ticks again.

11. If a process waits 10 times its quantum in the current level it gets a priority boost:
Inside the code in proc.c/mlfq_tick we do:
    
if(p->state == RUNNABLE){
  ..
  int limit = 10 * mlfq_quantum(p->priority); // tick of 10ms
  if(p->priority > 0 && p->waitticks >= limit){
    // Boost priority by one level (towards 0).
    p->priority--;
    ..
  }
}

Essentially we do exactly as we are asked to, if the condition is met and the limit is reached,
then the process gets a priority boost towards 0. It is important that we check the condition
that the priority is not already 0, and since we do this inside the if statement we are covered.

## Files changed in Part 2 and small reference to trap.c

proc.h - definition of mlfq_tick
proc.c - implementation of helper functions and modification of the scheduler
trap.c - change in the condition where the CPU is yielded

Since we haven't talked about this yet, it is important to mention that inside trap.c we have 
made a small modification inside usertrap():

// give up the CPU if this is a timer interrupt.
if(which_dev == 2){
  // MLFQ: only yield when quantum expires or a higher-priority process is ready.
  // Otherwise, we yield the CPU in every tick and the quantum 4/8/16/32 has no point.
  if(mlfq_tick())
    yield();
}

In order to actually use the rules decided and written in proc.c, we actually need to modify
the condition of when the CPU is yielded. Instead of plain yield() inside the timer interrupt 
if statement, we call yield() if and only if the mlfq_tick() function has returned 1.