/*
 * Driver code for airballoon problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

#define N_LORD_FLOWERKILLER 8
#define NROPES 16

#define N_OTHER_THREADS 2 // in order to account for the single Dandelion and Marigold threads
#define INTACT_ROPE 1
#define CUT_ROPE 0

/* Data structures for rope mappings */

volatile int ropes[NROPES];
volatile int ropes_left;
volatile int stakes_map[NROPES]; // important for keeping track of rope to stake mappings
volatile int hooks_map[NROPES]; // not necessary but used on principle of abstraction
volatile int threads_done;

/* Synchronization primitives */

struct lock* ropes_lock[NROPES]; // each rope has a lock associated
struct lock* stakes_lock[NROPES]; // each stake has a lock associated
struct lock* ropes_left_lock;
struct lock* threads_lock;
struct cv* balloon_cv;
struct cv* threads_cv;

/*
 * Describe your design and any invariants or locking protocols
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?
 */

/*
 * All shared data structures must be accessed through a lock. In order to avoid a big lock solution, each individual data structure
 * has its own lock associated. Notably, each rope and stake must have a lock as these are the core elements modified by the Dandelion,
 * Marigold, and FlowerKiller threads. These threads know when they are done as they monitor the ropes_left variable through the
 * ropes_left_lock. The balloon thread knows when it's done by waiting on balloon_cv, signalled when the last thread finishes the 
 * threads_lock. Finally, main thread will know it's done as it will wait on threads_cv, and will clean up the program after balloon
 * finishes and calls cv_signal on threads_cv.
 */

static
void
dandelion(void *p, unsigned long arg)
{
    (void)p;
    (void)arg;
    kprintf("Dandelion thread starting\n");

    lock_acquire(ropes_left_lock);
    while (ropes_left > 0) {
		ropes_left--;
		lock_release(ropes_left_lock);

        int index = random() % NROPES;

		/* We shuffle index until we find a hook where the rope is not cut */
		lock_acquire(ropes_lock[hooks_map[index]]);
        while (ropes[hooks_map[index]] == CUT_ROPE) {
			lock_release(ropes_lock[hooks_map[index]]);
            index = random() % NROPES;
			lock_acquire(ropes_lock[hooks_map[index]]);
        }
        ropes[hooks_map[index]] = CUT_ROPE;
		lock_release(ropes_lock[hooks_map[index]]);

        kprintf("Dandelion severed rope %d\n", index);
        thread_yield();

        lock_acquire(ropes_left_lock);
    }
    lock_release(ropes_left_lock);

    lock_acquire(threads_lock);
    threads_done++;
    kprintf("Dandelion thread done\n");
	if (threads_done == N_LORD_FLOWERKILLER + N_OTHER_THREADS) cv_signal(balloon_cv, threads_lock); // check if this is the last thread
    lock_release(threads_lock); // need to print before releasing lock in order to prevent menu bug

    thread_exit();
}

static
void
marigold(void *p, unsigned long arg)
{
    (void)p;
    (void)arg;
    
    kprintf("Marigold thread starting\n");
    lock_acquire(ropes_left_lock);
    while(ropes_left > 0) {
		ropes_left--;
		lock_release(ropes_left_lock);

        int index = random() % NROPES;

        /* We shuffle index until we find a stake where the rope is not cut */
		lock_acquire(stakes_lock[index]);
		lock_acquire(ropes_lock[stakes_map[index]]);
        while (ropes[stakes_map[index]] == CUT_ROPE) {
			lock_release(stakes_lock[index]);
		    lock_release(ropes_lock[stakes_map[index]]);
            index = random() % NROPES;
			lock_acquire(stakes_lock[index]);
		    lock_acquire(ropes_lock[stakes_map[index]]);
        }
        ropes[stakes_map[index]] = CUT_ROPE;
		lock_release(stakes_lock[index]);
		lock_release(ropes_lock[stakes_map[index]]);
        
        kprintf("Marigold severed rope %d from stake %d\n", stakes_map[index], index);
        thread_yield();

        lock_acquire(ropes_left_lock);
    }
    lock_release(ropes_left_lock);

    lock_acquire(threads_lock);
    threads_done++;
    kprintf("Marigold thread done\n");
	if (threads_done == N_LORD_FLOWERKILLER + N_OTHER_THREADS) cv_signal(balloon_cv, threads_lock); // check if this is the last thread
    lock_release(threads_lock); // need to print before releasing lock in order to prevent menu bug
    
    thread_exit();
}

static
void
flowerkiller(void *p, unsigned long arg)
{
    (void)p;
    (void)arg;

    kprintf("Lord FlowerKiller thread starting\n");
    lock_acquire(ropes_left_lock);
    while (ropes_left >= 2) {
		lock_release(ropes_left_lock);

		/* We read the necessary data structures based on two random indices. If the indices are invalid (i.e. either of the ropes at
		those indices are cut), then we shuffle the indices and recalculate. In total, four locks are required as we are accessing
		four different shared data points */

		/* The order of the lock acquisition is important. In order to prevent deadlocks, the smaller index lock is always acquired
		first. */
        int first_index = random() % NROPES;
		int second_index = random() % NROPES;
		while (first_index == second_index) {
			first_index = random() % NROPES;
			second_index = random() % NROPES;
		}
		if (first_index < second_index) {
			lock_acquire(stakes_lock[first_index]);
			lock_acquire(ropes_lock[stakes_map[first_index]]);
			lock_acquire(stakes_lock[second_index]);
			lock_acquire(ropes_lock[stakes_map[second_index]]);
		}
		else {
			lock_acquire(stakes_lock[second_index]);
			lock_acquire(ropes_lock[stakes_map[second_index]]);
			lock_acquire(stakes_lock[first_index]);
			lock_acquire(ropes_lock[stakes_map[first_index]]);
		}

		while (ropes[stakes_map[first_index]] == CUT_ROPE || ropes[stakes_map[second_index]] == CUT_ROPE) {
			if (first_index < second_index) {
				lock_release(stakes_lock[first_index]);
				lock_release(ropes_lock[stakes_map[first_index]]);
				lock_release(stakes_lock[second_index]);
				lock_release(ropes_lock[stakes_map[second_index]]);
			}
			else {
				lock_release(stakes_lock[second_index]);
				lock_release(ropes_lock[stakes_map[second_index]]);
				lock_release(stakes_lock[first_index]);
				lock_release(ropes_lock[stakes_map[first_index]]);
			}

			first_index = random() % NROPES;
		    second_index = random() % NROPES;
		    while (first_index == second_index) {
				first_index = random() % NROPES;
				second_index = random() % NROPES;
			}

			if (first_index < second_index) {
				lock_acquire(stakes_lock[first_index]);
				lock_acquire(ropes_lock[stakes_map[first_index]]);
				lock_acquire(stakes_lock[second_index]);
				lock_acquire(ropes_lock[stakes_map[second_index]]);
			}
			else {
				lock_acquire(stakes_lock[second_index]);
				lock_acquire(ropes_lock[stakes_map[second_index]]);
				lock_acquire(stakes_lock[first_index]);
				lock_acquire(ropes_lock[stakes_map[first_index]]);
			}
		}

		/* Upon getting valid indices, perform the swap on stakes_map */
        int first_rope = stakes_map[first_index];
        int second_rope = stakes_map[second_index];
        stakes_map[first_index] = second_rope;
        stakes_map[second_index] = first_rope;
		if (first_index < second_index) {
			lock_release(stakes_lock[first_index]);
			lock_release(ropes_lock[stakes_map[first_index]]);
			lock_release(stakes_lock[second_index]);
			lock_release(ropes_lock[stakes_map[second_index]]);
		}
		else {
			lock_release(stakes_lock[second_index]);
			lock_release(ropes_lock[stakes_map[second_index]]);
			lock_release(stakes_lock[first_index]);
			lock_release(ropes_lock[stakes_map[first_index]]);
		}

        kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n", first_rope, first_index, second_index);
        kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n", second_rope, second_index, first_index);
        thread_yield();

        lock_acquire(ropes_left_lock);
    }
    lock_release(ropes_left_lock);

    lock_acquire(threads_lock);
    threads_done++;
    kprintf("Lord FlowerKiller thread done\n");
	if (threads_done == N_LORD_FLOWERKILLER + N_OTHER_THREADS) cv_signal(balloon_cv, threads_lock); // check if this is the last thread
    lock_release(threads_lock); // need to print before releasing lock in order to prevent menu bug
    
    thread_exit();
}

static
void
balloon(void *p, unsigned long arg)
{
    (void)p;
    (void)arg;

    kprintf("Balloon thread starting\n");

	/* Continuously check for all threads to finish */
	lock_acquire(threads_lock);
	cv_wait(balloon_cv, threads_lock);

    kprintf("Balloon freed and Prince Dandelion escapes!\n");
    kprintf("Balloon thread done\n");
	cv_signal(threads_cv, threads_lock);
    lock_release(threads_lock); // need to print before releasing lock in order to prevent menu bug
    thread_exit();
}


// Change this function as necessary
int
airballoon(int nargs, char **args)
{

    int err = 0, i;

    (void)nargs;
    (void)args;

	/* Initialize static variables */
	ropes_left = NROPES;
    threads_done = 0;

	/* Initialize synchronization primitives */
	ropes_left_lock = lock_create("ropes_left");
    threads_lock = lock_create("threads");
	balloon_cv = cv_create("balloon_cv");
    threads_cv = cv_create("threads_cv");

	/* Initialize data structures and rope/stake locks */
    for (int i = 0; i < NROPES; i++) {
        ropes[i] = INTACT_ROPE;
        stakes_map[i] = i;  // each stake initially has a one-to-one mapping with each rope
        hooks_map[i] = i;
		ropes_lock[i] = lock_create("ropes");
		stakes_lock[i] = lock_create("stakes");
	}

    err = thread_fork("Marigold Thread",
              NULL, marigold, NULL, 0);
    if(err)
        goto panic;

    err = thread_fork("Dandelion Thread",
              NULL, dandelion, NULL, 0);
    if(err)
        goto panic;

    for (i = 0; i < N_LORD_FLOWERKILLER; i++) {
        err = thread_fork("Lord FlowerKiller Thread",
                  NULL, flowerkiller, NULL, 0);
        if(err)
            goto panic;
    }

    err = thread_fork("Air Balloon",
              NULL, balloon, NULL, 0);
    if(err)
        goto panic;

    lock_acquire(threads_lock);
	cv_wait(threads_cv, threads_lock); // wait on this cv until balloon indicates the problem is complete
    lock_release(threads_lock);
    
	/* Cleanup all synchonization primitives */
    lock_destroy(ropes_left_lock);
    lock_destroy(threads_lock);
	cv_destroy(balloon_cv);
    cv_destroy(threads_cv);
	for (int i = 0; i < NROPES; i++) {
		lock_destroy(ropes_lock[i]);
		lock_destroy(stakes_lock[i]);
    }

    kprintf("Main thread done\n");
    goto done;
panic:
    panic("airballoon: thread_fork failed: %s)\n",
          strerror(err));

done:
    return 0;
}
