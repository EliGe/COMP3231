#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "bar.h"
#include "bar_driver.h"



/*
 * **********************************************************************
 * YOU ARE FREE TO CHANGE THIS FILE BELOW THIS POINT AS YOU SEE FIT
 *
 */

/* Declare any globals you need here (e.g. locks, etc...) */
/*struct bottleStatus {
        int use;
};

static struct bottleStatus bottles[NBOTTLES];*/



static struct barorder *CurrentOrder[NCUSTOMERS];


static struct cv* full;
static struct cv* empty;
static struct lock* locka;
static struct lock* bottlelock[NBOTTLES];

//static struct cv* finished;
static struct lock* lockb;

//static struct semaphore* mutex;

int OrderCounter = 0;
/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */

/*
 * order_drink()
 *
 * Takes one argument referring to the order to be filled. The
 * function makes the order available to staff threads and then blocks
 * until a bartender has filled the glass with the appropriate drinks.
 */

void order_drink(struct barorder *order)
{
		//kprintf("%d\n",order->requested_bottles[0]);
		//kprintf("O         %d\n",order->glass.contents[0]);
        lock_acquire(locka);
        while(OrderCounter == NCUSTOMERS){
        	cv_wait(full, locka);
        }
        for (int i = 0; i < NCUSTOMERS; i++)
        {
                if(CurrentOrder[i] == NULL){
                        CurrentOrder[i] = order;
                        cv_signal(empty,locka);
                        OrderCounter++;
                        break;                    
                }
        }

        lock_release(locka);
        //kprintf("%d\n",CurrentOrder->requested_bottles[0]);
        //kprintf("G         %d\n",CurrentOrder->glass.contents[0]);
        
}



/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY BARTENDER THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it returns a pointer to the order.
 *
 */

struct barorder *take_order(void)
{
        struct barorder *ret = NULL;
        lock_acquire(locka);
        while(OrderCounter == 0){
        	cv_wait(empty,locka);
        }
        for (int i = 0; i < NCUSTOMERS; i++)
        {
        	if (CurrentOrder[i] != NULL)
        	{
        		ret = CurrentOrder[i];
        		
        		cv_signal(full,locka);
			OrderCounter--;
        		break;
        	}
        }
        lock_release(locka);
	//	kprintf("================%d\n",ret->requested_bottles[0]);
        return ret;
}


/*
 * fill_order()
 *
 * This function takes an order provided by take_order and fills the
 * order using the mix() function to mix the drink.
 *
 * NOTE: IT NEEDS TO ENSURE THAT MIX HAS EXCLUSIVE ACCESS TO THE
 * REQUIRED BOTTLES (AND, IDEALLY, ONLY THE BOTTLES) IT NEEDS TO USE TO
 * FILL THE ORDER.
 */

void fill_order(struct barorder *order)//cus
{

        /* add any sync primitives you need to ensure mutual exclusion
           holds as described */

        /* the call to mix must remain */

        //sort the requested_bottles

	for(int i = 0; i < DRINK_COMPLEXITY; i++){
		for(int j = 0; j < (DRINK_COMPLEXITY-i); j++){
			if(order->requested_bottles[j] > order->requested_bottles[j+1]){
				int temp = order->requested_bottles[j+1]
				order->requested_bottles[j+1] = order->requested_bottles[j];
				order->requested_bottles[j] = temp;
			}
		}
	}

        //lock the bottle

	for(int i = 0; i < DRINK_COMPLEXITY; i++){
		
		if(!lock_do_i_hold(bottlelock[order->requested_bottles[i]-1])){
			lock_acquire(bottlelock[order->requested_bottles[i]-1]);
		}
		
	}

        mix(order);

        for(int i = 0; i < DRINK_COMPLEXITY; i++){

        	if(lock_do_i_hold(bottlelock[order->requested_bottles[i]-1])){
        		lock_release(bottlelock[order->requested_bottles[i]-1]);
        	}
        }

}


/*
 * serve_order()
 *
 * Takes a filled order and makes it available to (unblocks) the
 * waiting customer.
 */

void serve_order(struct barorder *order)//pro
{




	for (int i = 0; i < NCUSTOMERS; i++)
	{
		if (CurrentOrder[i] == order)
		{
			CurrentOrder[i] = NULL;
			
			break;
		}
	}

	
}



/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */


/*
 * bar_open()
 *
 * Perform any initialisation you need prior to opening the bar to
 * bartenders and customers. Typically, allocation and initialisation of
 * synch primitive and variable.
 */

void bar_open(void)
{

        for (int i = 0; i < NCUSTOMERS; i++)
        {
        	CurrentOrder[i] = NULL;
        }

        for (int j = 0; j < NBOTTLES; j++)
        {
        	bottles[j] = 0;
        }
        locka = lock_create("locka");
	full = cv_create("full");
	empty = cv_create("empty");
	
	bottlelock[0] = lock_create("bottlelock[0]");
	bottlelock[1] = lock_create("bottlelock[1]");
	bottlelock[2] = lock_create("bottlelock[2]");
	bottlelock[3] = lock_create("bottlelock[3]");
	bottlelock[4] = lock_create("bottlelock[4]");
	bottlelock[5] = lock_create("bottlelock[5]");
	bottlelock[6] = lock_create("bottlelock[6]");
	bottlelock[7] = lock_create("bottlelock[7]");
	bottlelock[8] = lock_create("bottlelock[8]");
	bottlelock[9] = lock_create("bottlelock[9]");

	//finished = cv_create("finished");

        lockb = lock_create("lockb");
	//mutex = sem_create("mutex", NBARTENDERS);
}

/*
 * bar_close()
 *
 * Perform any cleanup after the bar has closed and everybody
 * has gone home.
 */

void bar_close(void)
{
	lock_destroy(locka);
	cv_destroy(full);
	cv_destroy(empty);
	//cv_destroy(finished);
	lock_destroy(lockb);
	//sem_destroy(mutex);
}

