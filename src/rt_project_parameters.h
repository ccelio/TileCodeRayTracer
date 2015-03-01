#ifndef RT_PROJECT_PARAMETERS_H_
#define RT_PROJECT_PARAMETERS_H_

#include "fixed_class.h"

/* RayTracing Parameters */

//#define LOG_FILE_TITLE "OSX FixedPoint Testing\n"
#define LOG_FILE_TITLE "OSX Awesome Picture\n"
// "Is for simulation"; aka, does it run inside of a software simulator.
#define IS_FOR_SIMULATION false
#define PARTIONING_STRATEGY 0
    //0 = NO_PARTIONING (only for x86, which runs single core)
    //1 = STATIC_PARTIONING
    //2 = SELF_SCHEDULING
    //3 = DISTRIBUTED WORK QUEUE
    //4 = SELF_SCHEDULING, parallized collision code, buffer channels
    //5 = "fair" static partioning
    //6 = "fair" static partioning, parallized collision code
    //7 = SELF_SCHEDULING, parallized collision code, raw channels
#define HARDWARE_TARGET "OSX C++"
#define HARDWARE_IS_TILE false
#define X11_ON false
#define REFLECTIONS_ON true
#define SHADOWS_ON true

#define SCENE 1
    // scenes are defined statically :(
    //1 == Mesueum
    //2 == Two Mirrors & Pyramids
    //TODO allow ability to read in a scene file generated from something like
    //Blender.

// TILE64 processor lacks FPUs :(
#define USING_FIXED_POINT false
#define POINT 16 //ie, 16.16 fixed point 

#if USING_FIXED_POINT
    typedef fixed sdecimal32;
#else
    typedef float sdecimal32; 
#endif
    
#if HARDWARE_IS_TILE
#if IS_FOR_SIMULATION
    #define CORE_NUM 2
    #define SCREEN_HORIZONTAL_RESOLUTION 5
    #define SCREEN_VERTICAL_RESOLUTION 5
    #define SPAWN_X_CORNER 0
    #define SPAWN_Y_CORNER 0
    #define SPAWN_WIDTH 8
    #define SPAWN_HEIGHT 8
#else
    #define CORE_NUM 56
    #define SCREEN_HORIZONTAL_RESOLUTION 500
    #define SCREEN_VERTICAL_RESOLUTION 504
    #define SPAWN_X_CORNER 0
    #define SPAWN_Y_CORNER 0
    #define SPAWN_WIDTH 8
    #define SPAWN_HEIGHT 8
#endif
#else
    //MacOS x86 C++ target
    #define CORE_NUM 1
    #define SCREEN_HORIZONTAL_RESOLUTION 500
    #define SCREEN_VERTICAL_RESOLUTION 504
#endif
    
#define GLOBAL_MASTER_RANK 0    //location to home shared data
                        //for some reason, Master_rank==0 
                        //seems to be the best to home shared data

#define MAX_RECURSION_LEVEL 50
#define FLOAT_MAX_VALUE 65535 //for 16.16 fixed point//1E9 //what is the max float value?



#endif /*RT_PROJECT_PARAMETERS_H_*/
