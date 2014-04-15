	/* This file is derived from source code for the Nachos
	   instructional operating system.  The Nachos copyright notice
	   is reproduced in full below. */

	/* Copyright (c) 1992-1996 The Regents of the University of California.
	   All rights reserved.

	   Permission to use, copy, modify, and distribute this software
	   and its documentation for any purpose, without fee, and
	   without written agreement is hereby granted, provided that the
	   above copyright notice and the following two paragraphs appear
	   in all copies of this software.

	   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
	   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
	   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
	   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
	   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
	   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
	   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
	   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
	   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
	   MODIFICATIONS.
	*/

	#include "threads/synch.h"
	#include <stdio.h>
	#include <string.h>
	#include "threads/interrupt.h"
	#include "threads/thread.h"

	/* Initializes semaphore SEMA to VALUE.  A semaphore is a
	   nonnegative integer along with two atomic operators for
	   manipulating it:

	   - down or "P": wait for the value to become positive, then
		 decrement it.

	   - up or "V": increment the value (and wake up one waiting
		 thread, if any). */
	void
	sema_init (struct semaphore *sema, unsigned value) 
	{
	  ASSERT (sema != NULL);

	  sema->value = value;
	  list_init (&sema->waiters);
	}

	/* Down or "P" operation on a semaphore.  Waits for SEMA's value
	   to become positive and then atomically decrements it.

	   This function may sleep, so it must not be called within an
	   interrupt handler.  This function may be called with
	   interrupts disabled, but if it sleeps then the next scheduled
	   thread will probably turn interrupts back on. */
	void
	sema_down (struct semaphore *sema) 
	{
	  enum intr_level old_level;

	  ASSERT (sema != NULL);
	  ASSERT (!intr_context ());

	  old_level = intr_disable ();
	  while (sema->value == 0) 
		{
		  list_push_back (&sema->waiters, &thread_current ()->elem);
		  thread_block ();
		}
	  sema->value--;
	  intr_set_level (old_level);
	}

	/* Down or "P" operation on a semaphore, but only if the
	   semaphore is not already 0.  Returns true if the semaphore is
	   decremented, false otherwise.

	   This function may be called from an interrupt handler. */
	bool
	sema_try_down (struct semaphore *sema) 
	{
	  enum intr_level old_level;
	  bool success;

	  ASSERT (sema != NULL);

	  old_level = intr_disable ();
	  if (sema->value > 0) 
		{
		  sema->value--;
		  success = true; 
		}
	  else
		success = false;
	  intr_set_level (old_level);

	  return success;
	}

	/* Up or "V" operation on a semaphore.  Increments SEMA's value
	   and wakes up one thread of those waiting for SEMA, if any.

	   This function may be called from an interrupt handler. 
	   Also it will check for the threads that are waiting in the semaphore.
	   If the thread that is waiting in the semaphore waiters list has priority
	   greater than the currently running thread,we have to unblock it and yield that thread*/
	void
	sema_up (struct semaphore *sema) 
	{
	  enum intr_level old_level;
	  ASSERT (sema != NULL);
	  old_level = intr_disable ();
	  int Current_max = PRI_MIN;
      int max_val = Current_max -1;
	  bool isEmpty = !list_empty (&sema->waiters);
	  /* Check if the list of waiting threads in semaphore is not empty */
	  if (isEmpty)
	  {
		struct list_elem *cur = list_begin (&sema-> waiters );
		 /* call a function that will scan the list of waiting threads in semaphores
			and returns the  thread having maximum priority. */
		max_val = unblock_max_priority_thd(cur,max_val,sema);
	  }//end of if
	  sema->value++;
	  check_max_priority(max_val);
	  intr_set_level (old_level);
	}//end of sema_up function

	/* This function that will scan the list of waiting threads in semaphores
	   finds the thread that has max priority and if that thread has priority 
	   greater than the one that is currently running,then remove it from the 
	   waitlist and unblock it.
	  
	   Return the  max priority number to the caller function i.e sema_up*/
	 
	 int unblock_max_priority_thd(struct list_elem *cur,int max_val,
									struct semaphore *sema)
	 {
		struct list_elem *max_priority_thd_elem;
		struct thread *max_priority_thd;
		
		for(cur = list_begin(&sema->waiters);cur!=list_end(&sema->waiters);
			cur=list_next(cur))
		{
			struct thread *cur_thd = list_entry (cur, struct thread, elem);
				int get_p=thread_highest_priority(cur_thd);
				if(max_val< get_p)
				{
					max_priority_thd_elem = cur;
					/* update the max priority */
					max_val=get_p;
					max_priority_thd=cur_thd;
				}//end of if
		}//end of for
		/* remove the thread from the list of waitlist in semaphore */
		list_remove(max_priority_thd_elem);
		/* now unblock this thread */
		thread_unblock(max_priority_thd);

		return max_val;
	 }//end of unblock_max_priority_thd function 


	static void sema_test_helper (void *sema_);

	/* Self-test for semaphores that makes control "ping-pong"
	   between a pair of threads.  Insert calls to printf() to see
	   what's going on. */
	void
	sema_self_test (void) 
	{
	  struct semaphore sema[2];
	  int i;

	  printf ("Testing semaphores...");
	  sema_init (&sema[0], 0);
	  sema_init (&sema[1], 0);
	  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
	  for (i = 0; i < 10; i++) 
		{
		  sema_up (&sema[0]);
		  sema_down (&sema[1]);
		}
	  printf ("done.\n");
	}

	/* Thread function used by sema_self_test(). */
	static void
	sema_test_helper (void *sema_) 
	{
	  struct semaphore *sema = sema_;
	  int i;

	  for (i = 0; i < 10; i++) 
		{
		  sema_down (&sema[0]);
		  sema_up (&sema[1]);
		}
	}
	
	/* Initializes LOCK.  A lock can be held by at most a single
	   thread at any given time.  Our locks are not "recursive", that
	   is, it is an error for the thread currently holding a lock to
	   try to acquire that lock.

	   A lock is a specialization of a semaphore with an initial
	   value of 1.  The difference between a lock and such a
	   semaphore is twofold.  First, a semaphore can have a value
	   greater than 1, but a lock can only be owned by a single
	   thread at a time.  Second, a semaphore does not have an owner,
	   meaning that one thread can "down" the semaphore and then
	   another one "up" it, but with a lock the same thread must both
	   acquire and release it.  When these restrictions prove
	   onerous, it's a good sign that a semaphore should be used,
	   instead of a lock. */
	void
	lock_init (struct lock *lock)
	{
	  ASSERT (lock != NULL);

	  lock->holder = NULL;
	  sema_init (&lock->semaphore, 1);
	}

	/* Acquires LOCK, sleeping until it becomes available if
	   necessary.  The lock must not already be held by the current
	   thread.And also it will take care of the donation of priority.

	   This function may sleep, so it must not be called within an
	   interrupt handler.  This function may be called with
	   interrupts disabled, but interrupts will be turned back on if
	   we need to sleep. */
	void
	lock_acquire (struct lock *lock)
	{
	  ASSERT (lock != NULL);
	  ASSERT (!intr_context ());
	  ASSERT (!lock_held_by_current_thread (lock));
	  enum intr_level old_level;
	  old_level = intr_disable ();
	  /* call a function that will check if the thread 
		 can donate its priority */
	  check_for_donation(lock);	
	  intr_set_level(old_level);
	  sema_down (&lock->semaphore);
	  lock->holder = thread_current ();
	}//end of lock_accquire function


	/* This function will check if the thread that has the max priority 
	   currently has the lock or not.If the thread does not have the lock then
	   it will have to donate its priority to the thread that currently has the
	   lock.Also it has to consider nested situations. i.e donating the 
	   priority to the thread that holds the lock currently and all its child 
	   threads.*/
	 
	 void check_for_donation(struct lock *lock)
	 {
	  if(lock->holder !=NULL)
		{
			int get_p=thread_highest_priority(lock->holder);
			int cur_thd_p=thread_get_priority();
			if(cur_thd_p > get_p)
			{
				struct thread *cur_t=thread_current();
				donation_of_priority(cur_t,lock->holder,lock);
			}
		}
	 }

	/* In this function,thread cur_t donates its priority to the lock holder
	   thread i.e the thread who is currently holding the lock and updates the
	   donation structure variables and also the donation_givenlist and 
	   donation_takenlist.
	
	   And it also calls the function that will check if nested donations are to
	   be made*/
	
	void donation_of_priority(struct thread *cur_t,struct thread *lholder,
							  struct lock *lock)
	{
		/* initialize the donation */
		struct donation *src_donation=(struct donation *)malloc 
		(sizeof(struct donation));
		/* initialize the d_donation */
		struct donation *donation_dest=(struct donation *)malloc 
		(sizeof(struct donation));

		/* Get the priority of the donator's thread */
		int donator_priority=thread_highest_priority(cur_t);
		/* assign this priority to the thread who is getting the donation */
		donation_dest->donated_pri=donator_priority;
		/* also assign the start_donatoion this priority */
		src_donation->donated_pri=donator_priority;

		/* also assign locks to these donations  */
		donation_dest->lock=lock;
		src_donation->lock=lock;
		/* record the name of the thread whom it had given priority to */
		donation_dest->take_donation=lholder;
		
		/* update the donation lists */
		list_push_back(&cur_t->donation_givenlist,&donation_dest->donate_this_elem);
		list_push_back(&lholder->donation_takenlist,&src_donation->donate_this_elem);
		
		/* check for nested donations */
		nested_donation_check(cur_t,lholder,lock);
	}//end of the donation_of_priority function

	/* this function will check for nested donations and will donate the 
	   priority to all the children threads if they exist */
	
	void nested_donation_check(struct thread *cur_t,struct thread *lholder,
		struct lock *lock)
	{
		struct list_elem *cur = list_begin (&lholder->donation_givenlist);
		for(cur = list_begin(&lholder->donation_givenlist);cur!=
			list_end(&lholder->donation_givenlist);cur=list_next(cur))
		{
			struct donation *don=list_entry(cur,struct donation,donate_this_elem);
			struct thread *new_t=don->take_donation;
			donation_of_priority(cur_t,new_t,don->lock);
		}	
	}//end of function nested_donation_check

	/* Tries to acquires LOCK and returns true if successful or false
	   on failure.  The lock must not already be held by the current
	   thread.

	   This function will not sleep, so it may be called within an
	   interrupt handler. */
	bool
	lock_try_acquire (struct lock *lock)
	{
	  bool success;

	  ASSERT (lock != NULL);
	  ASSERT (!lock_held_by_current_thread (lock));

	  success = sema_try_down (&lock->semaphore);
	  if (success)
		lock->holder = thread_current ();
	  return success;
	}

	/* Releases LOCK, which must be owned by the current thread.
	   Also before releasing the lock, it will take that thread off from the 
	   donation_givenlist and donation_takenlist which is related to the lock 
	   that has to be released.

	   An interrupt handler cannot acquire a lock, so it does not
	   make sense to try to release a lock within an interrupt
	   handler. */
	
	void
	lock_release (struct lock *lock) 
	{
	  ASSERT (lock != NULL);
	  ASSERT (lock_held_by_current_thread (lock));
	  struct list_elem *cur_given=
			list_head(&thread_current()->donation_givenlist)->next;
	  delete_donations_givenlist(cur_given,lock);
      delete_donations_takenlist(lock);
	  lock->holder = NULL;
	  sema_up (&lock->semaphore);
	}//end of release function


	/* This function will check for the list_elem that are currently in the
	   donation_takenlist and are related to the lock that has to be released.
	   If such list_elem exist,then they have to be removed from the list.	*/
	
	void delete_donations_takenlist(struct lock *lock)
	{
		struct list_elem *cur = list_begin (&thread_current()->donation_takenlist);
		for(cur = list_begin(&thread_current()->donation_takenlist);
		cur!=list_end(&thread_current()->donation_takenlist);cur=list_next(cur))
		{
			struct donation *cur_d = list_entry ( cur , struct donation, donate_this_elem);
					if(cur_d->lock == lock)
					{
						cur = list_remove(cur)->prev;
					}
		
		}//end of for
	}//end of delete_donations function
	
	/* This function will check for the list_elem that are currently in the
	   donation_givenlist and are related to the lock that has to be released.
	   If such list_elem exist,then they have to be removed from the list.	*/
	
	void delete_donations_givenlist(struct list_elem *cur,struct lock *lock)
	{
		while(cur->next != NULL)
		{
			struct donation *cur_d = list_entry ( cur , struct donation, donate_this_elem);
				if(cur_d->lock == lock)
				{
					cur = list_remove(cur);
				}
				else
				{
					cur=cur->next;
				}	
		}
	}//end of delete_donations_givenlist

	/* Returns true if the current thread holds LOCK, false
	   otherwise.  (Note that testing whether some other thread holds
	   a lock would be racy.) */
	bool
	lock_held_by_current_thread (const struct lock *lock) 
	{
	  ASSERT (lock != NULL);

	  return lock->holder == thread_current ();
	}
	
	/* One semaphore in a list. */
	struct semaphore_elem 
	  {
		struct list_elem elem;              /* List element. */
		struct semaphore semaphore;         /* This semaphore. */
	  };

	/* Initializes condition variable COND.  A condition variable
	   allows one piece of code to signal a condition and cooperating
	   code to receive the signal and act upon it. */
	void
	cond_init (struct condition *cond)
	{
	  ASSERT (cond != NULL);

	  list_init (&cond->waiters);
	}

	/* Atomically releases LOCK and waits for COND to be signaled by
	   some other piece of code.  After COND is signaled, LOCK is
	   reacquired before returning.  LOCK must be held before calling
	   this function.

	   The monitor implemented by this function is "Mesa" style, not
	   "Hoare" style, that is, sending and receiving a signal are not
	   an atomic operation.  Thus, typically the caller must recheck
	   the condition after the wait completes and, if necessary, wait
	   again.

	   A given condition variable is associated with only a single
	   lock, but one lock may be associated with any number of
	   condition variables.  That is, there is a one-to-many mapping
	   from locks to condition variables.

	   This function may sleep, so it must not be called within an
	   interrupt handler.  This function may be called with
	   interrupts disabled, but interrupts will be turned back on if
	   we need to sleep. */
	void
	cond_wait (struct condition *cond, struct lock *lock) 
	{
	  struct semaphore_elem waiter;

	  ASSERT (cond != NULL);
	  ASSERT (lock != NULL);
	  ASSERT (!intr_context ());
	  ASSERT (lock_held_by_current_thread (lock));
	  
	  sema_init (&waiter.semaphore, 0);
	  list_push_back (&cond->waiters, &waiter.elem);
	  lock_release (lock);
	  sema_down (&waiter.semaphore);
	  lock_acquire (lock);
	}

	/* If any threads are waiting on COND (protected by LOCK), then
	   this function signals one of them to wake up from its wait.
	   LOCK must be held before calling this function.
	   Also it will check if the waiters are present in the condition, 
	   if yes then it will call a function that will scan the list and
	   process further.

	   An interrupt handler cannot acquire a lock, so it does not
	   make sense to try to signal a condition variable within an
	   interrupt handler. */
	void
	cond_signal (struct condition *cond, struct lock *lock UNUSED) 
	{
	  ASSERT (cond != NULL);
	  ASSERT (lock != NULL);
	  ASSERT (!intr_context());
	  ASSERT (lock_held_by_current_thread (lock));
	  bool isEmpty = !list_empty (&cond->waiters);
	  if(isEmpty)
	  {
		int Current_max = PRI_MIN;
		int max_val = Current_max -1;
		struct list_elem *cur = list_begin (&cond-> waiters);
		proceed_release(max_val,cur,cond);
	  }
	}//end of function cond_single

	
	/* This function will scan the waiters list and check the list of 
	   semaphores and the semaphore who has highest priority waiter will 
	   change its state to sema_up. */
	   
	void proceed_release(int max_val,struct list_elem *cur,struct condition *cond)
	{
		struct thread *cur_thd;
		struct semaphore *sema_of_cur;
		struct semaphore *sema_m;
		struct list_elem *cur_nested_p;
		struct list_elem *sema_elem_m;
		
		for(cur = list_begin(&cond-> waiters);cur!=list_end(&cond-> waiters);
			cur=list_next(cur))
		{
			sema_of_cur = &list_entry(cur, struct semaphore_elem, elem)->semaphore;
			cur_nested_p = list_head(&sema_of_cur->waiters)->next;
			while (cur_nested_p->next != NULL)
			{
				/* get the current thread */
				cur_thd = list_entry(cur_nested_p, struct thread, elem);
				/* get the current thread's max priority */
				int get_p=thread_highest_priority(cur_thd);
				
				if( max_val < get_p)
				{							
					sema_elem_m = cur;
			
					sema_m = sema_of_cur;
							
					max_val = get_p;		
				}
				
				cur_nested_p = cur_nested_p->next;
			}	
		}	
		list_remove(sema_elem_m);
		sema_up(sema_m);
	}//end of proceed_release function

	
	/* Wakes up all threads, if any, waiting on COND (protected by
	   LOCK).  LOCK must be held before calling this function.

	   An interrupt handler cannot acquire a lock, so it does not
	   make sense to try to signal a condition variable within an
	   interrupt handler. */
	void
	cond_broadcast (struct condition *cond, struct lock *lock) 
	{
	  ASSERT (cond != NULL);
	  ASSERT (lock != NULL);

	  while (!list_empty (&cond->waiters))
		cond_signal (cond, lock);
	}