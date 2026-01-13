# getpinfo System Call and ps Utility (xv6)

## Overview

In this part of the project, a new system call `getpinfo` was implemented in the xv6 kernel, along with a simple user-level program `ps`.  
The purpose is to allow user programs to retrieve basic information about all active processes.

Up to this point, no changes to the scheduler have been made.

## System Call: getpinfo

### Description

int getpinfo(struct pstat *ps);

The system call copies information about all active processes from kernel space to user space.  
On success, it returns `0`. On failure, it returns `-1`.

### Data Structures

The kernel process structure (`struct proc`) was extended with a `priority` field, initialized to `0` for all processes.

A new structure `struct pstat` was introduced, containing arrays (size `NPROC`) with the following fields:
- pid
- ppid
- state
- priority
- memory size
- process name
- used flag

Each array index corresponds to a process slot.

### Implementation Summary

The system call:
1. Receives a user pointer to `struct pstat`
2. Iterates over the global process table (`proc[]`)
3. Copies information for each active process
4. Transfers the filled structure to user space using `copyout`

All accesses to process structures are protected using per-process locks.

### Modified Files

- `proc.h` – extended `struct proc`, defined `struct pstat`
- `proc.c` – initialized process priority
- `sysproc.c` – implemented `sys_getpinfo`
- `syscall.h` – assigned syscall number
- `syscall.c` – added syscall dispatch entry
- `user.h` – declared the syscall
- `usys.pl` – generated user-level syscall stub

## User Program: ps

A simple user program `ps` was implemented to test the system call.

The program:
- calls `getpinfo`
- prints information for all active processes

Example output:
PID PPID PRIO STATE SZ NAME
1 0 0 SLEEPING 16384 init
2 1 0 SLEEPING 20480 sh
3 2 0 RUNNING 16384 ps

## Testing

The implementation was tested as follows:
- Kernel build and boot using QEMU
- Execution of the official xv6 test suite:

usertests
makefile

Result:
ALL TESTS PASSED

- Manual execution of the `ps` program

The system call and user program function correctly without affecting existing kernel behavior.