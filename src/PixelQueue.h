#ifndef QUEUE_H_
#define QUEUE_H_

#if HARDWARE_IS_TILE
/*************************************
 * 
 *            PIXELQUEUE.H
 * 
 * ***********************************/

#include "vector3d.h"
#include "rt_project_parameters.h"

extern "C" {
    #include <ilib.h>
}

/* This was written in .c for hw2, so
 * that might explain its identity crisis
 */

namespace CelioRayTracer
{

class PixelQueue
{

public:

    class PixelObject {
        public:
            PixelObject() {
                x = 0;
                z = 0;
            }
            PixelObject(int _x, int _z) {
                x = _x;
                z = _z;
            }
            PixelObject(PixelObject* ptr) {
                x = ptr->getX();
                z = ptr->getZ();
            }
            void setLoc(int _x, int _z) {
                x = _x;
                z = _z;
            }
            int inline getX() { return x; };
            int inline getZ() { return z; };
            virtual ~PixelObject() {
            }
        private:
            int x;
            int z;
    };
    /* This typedef allows the queue code to be content 
     * 'agnostic.' 
     */
    typedef PixelObject queueContent;

    struct queueElement{
        //let's point to the content so we don't waste memory
        queueContent *content;  
        queueElement *next;
    };
    

    PixelQueue();
    queueContent* peek();
    void enqueue(queueContent *new_object_ptr);
    queueContent* dequeue();
    int queue_size();
    bool isQueueEmpty();
    virtual ~PixelQueue();
    
    ilibMutex mutex;    
private:
    
    queueElement* first;
    queueElement* last; 
};

}

#endif //if tile hardware
#endif /*QUEUE_H_*/
