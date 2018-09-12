/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include "producerconsumer_driver.h"
#include <synch.h>  /* for P(), V(), sem_* */

/* Declare any variables you need here to keep track of and
   synchronise your bounded. A sample declaration of a buffer is shown
   below. You can change this if you choose another implementation. */

static struct pc_data buffer[BUFFER_SIZE];
static struct cv* full;
static struct cv* empty;
static struct lock* locka;

int counter = 0;

/* consumer_receive() is called by a consumer to request more data. It
   should block on a sync primitive if no data is available in your
   buffer. */

struct pc_data consumer_receive(void)
{
	

	
	lock_acquire(locka);
	while(counter == 0){
		cv_wait(empty,locka);
	}


	struct pc_data thedata;
	
        //(void) buffer; /* remove this line when you start */
        int i = 0;
        thedata.item1 = 0;
        thedata.item2 = 0;
        /* FIXME: this data should come from your buffer, obviously... */
        while(i < BUFFER_SIZE){
          if(buffer[i].item1 != 0 || buffer[i].item2 != 0){
            thedata.item1 = buffer[i].item1;
            thedata.item2 = buffer[i].item2;
	    buffer[i].item1 = 0;
	    buffer[i].item2 = 0;
	    cv_signal(full,locka);
	    counter--;
	    
            break;
          }
          i++;

        }

	
	lock_release(locka);
        return thedata;
}

/* procucer_send() is called by a producer to store data in your
   bounded buffer. */

void producer_send(struct pc_data item)
{
        //(void) item; /* Remove this when you add your code */
	
	
	lock_acquire(locka);
	while(counter == BUFFER_SIZE){
		cv_wait(full, locka);
	}


        int i = 0;
        while(i < BUFFER_SIZE){
          if(buffer[i].item1 == 0 && buffer[i].item2 == 0){
            buffer[i].item1 = item.item1;
            buffer[i].item2 = item.item2;
	    cv_signal(empty,locka);
	    counter++;
            break;
          }
          i++;
        }
	
	lock_release(locka);
}




/* Perform any initialisation (e.g. of global data) you need
   here. Note: You can panic if any allocation fails during setup */

void producerconsumer_startup(void)
{
	int i = 0;
	while(i < BUFFER_SIZE){
		buffer[i].item1 = 0;
		buffer[i].item2 = 0;
		i++;
	}
	locka = lock_create("locka");
	full = cv_create("full");
	empty = cv_create("empty");

}

/* Perform any clean-up you need here */
void producerconsumer_shutdown(void)
{
	lock_destroy(locka);
	cv_destroy(full);
	cv_destroy(empty);
	
}

