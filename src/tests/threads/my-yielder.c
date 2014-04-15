#include <stdio.h>
#include <debug.h>
#include "threads/thread.h"

static void yielder()
{
	thread_yield();
}
void rtt()
{
	msg ("\nexecuting thread t1.....\n\n");
	tid_t t1 = thread_create("first yielder", PRI_DEFAULT, yielder, NULL);
	msg ("\nexecuting thread t2........\n\n");
	tid_t t2 = thread_create("second yielder", PRI_DEFAULT, yielder, NULL);
	thread_yield();
}
