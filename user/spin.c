#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  // Default: run long enough to observe demotion.
  int secs = 5;

  // Optional: allow "spin 10" (seconds).
  if(argc >= 2)
    secs = atoi(argv[1]);

  printf("spin: busy-waiting for %d second(s)...\n", secs);

  // Burn CPU. We purposely avoid sleep().
  // We use uptime() as a time source (ticks since boot).
  uint start = uptime();
  uint end = start + secs * 100;   // xv6 tick is ~10ms -> ~100 ticks/sec

  volatile uint64 x = 0;

  while(uptime() < end){
    // Tight loop to consume CPU. The volatile prevents easy optimization.
    x = x * 1664525 + 1013904223;
    x ^= (x >> 13);
  }

  printf("spin: done (x=%ld)\n", (long)x);
  exit(0);
}
