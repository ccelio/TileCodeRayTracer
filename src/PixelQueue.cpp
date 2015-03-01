/*************************************
 * 
 *            PIXELQUEUE.CPP
 * 
 * ***********************************/
#include "rt_project_parameters.h"

#if HARDWARE_IS_TILE
#include "PixelQueue.h"
extern "C" {
    #include <ilib.h>
}

namespace CelioRayTracer
{

PixelQueue::PixelQueue()
{
    first = NULL;
    last = NULL;    
}
/*returns the content held in the front 
 * of the queue.
* <<USE dequeue instead>>. That way the user
 * explicitly knows that "dequeue" returns
 * the pointer, and does *not* free the object.
 */
PixelQueue::queueContent* PixelQueue::peek()
{   //returns a pointer, so user program
    //must be careful in free'ing this
    if(first==NULL)
        return NULL;
    else
        return first->content;
}

/* Dequeue: remove first element,
 * make second element first
 * NOTE: dequeue does *not* free the first
 * queueElement. For this situation, I have
 * decided to let the user program take the
 * pointer, and use it, and free it on his own time.
 */
 
PixelQueue::queueContent* PixelQueue::dequeue()
{   
    queueElement* old_first;
    old_first = first;
    
    if(old_first->next == NULL) {
        first = NULL;
        last = NULL;
    } else {
        first = old_first->next;
    }
    //remember, user must free this pointer later.
    return old_first->content;
}

int PixelQueue::queue_size()
{
    if(first != NULL) {
        int counter = 0;
        queueElement* e_ptr;
        e_ptr = first;
        
        while(e_ptr !=NULL) {
            e_ptr = e_ptr->next;
            counter= counter+1;
        }
        
        return counter;
        
    } else {        
        return 0;
    }
}

void PixelQueue::enqueue(queueContent *new_object_ptr)
{
    //create new qElement, and add Contents to it
    queueElement *newElement_ptr;
    newElement_ptr = (queueElement*) malloc_shared(sizeof(queueElement)); 
    if(newElement_ptr==NULL) ilib_die("malloc failure.");

    newElement_ptr->content = new_object_ptr;
    newElement_ptr->next = NULL;
    
    //make the last element point to the newElement
    //make newElement the past thing in the queue
    if(last != NULL) {
        last->next = newElement_ptr;
        last = newElement_ptr;
    } else {
        //array in empty, let's initialize it with the new Element!
        first = newElement_ptr;
        last = newElement_ptr;
    }
}

bool PixelQueue::isQueueEmpty() 
{
    if(first != NULL) {
            return false; //return 'queue is not empty'
    }
    return true; //return 'queue is empty'
}


PixelQueue::~PixelQueue()
{
    delete first;
    delete last;
}

}

#endif

