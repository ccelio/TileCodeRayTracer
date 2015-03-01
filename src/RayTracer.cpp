//============================================================================
// Name        : RayTracer.cpp
// Author      : CCelio
// Date        : 2008 May
// Version     :
// Copyright   : 
// Description : A very basic ray tracer designed to run on both x86 processors
//                  and Tilera's TILE64 many-core processor. This was my first
//                  C++ program, so apologies for what follows. 
//
//                  A couple different parallelization techniques are used, but
//                  mostly boil down to splitting up work on a per-pixel basis.
//
//                  Some of the obsession with fixed-point in this code is
//                  because the TILE64 processor does not have a FPU.
//============================================================================

#include <iostream>
#include "vector3d.h"
#include <time.h>
#include "Scene.h"
#include "Camera.h"
#include "Ray.h"
#include "SceneObject.h"
#include <exception>

#if HARDWARE_IS_TILE
    extern "C" {
        #include <ilib.h>
    }
    #include <sys/archlib.h>
    #include <sys/profiler.h>
#endif

#include "RayTracer.h"
#include "PixelQueue.h"

#if USING_FIXED_POINT 
    #include "fixed_func.h"
    #include "fixed_class.h"
#endif

/**************************************/
/********** START THE CODE!  **********/
/**************************************/

/*************************************/
/********** GET COLLISION ************/
                        
CollisionObject* getCollision(Ray *temp_ray_ptr) {
    
    /****Find nearestObject that the eyeRay intersects****/
    sdecimal32 nearestDist = FLOAT_MAX_VALUE;
    
    CollisionObject *nearest_collision_ptr = NULL;
    CollisionObject *temp_collision_obj_ptr = NULL;
    
    int x_init, x_final;
    
#if HARDWARE_IS_TILE
    if(my_local_rank==FIEFDOM_MASTER_RANK) {
        x_init = my_scene.getSceneObjectStartIndex();
        x_final = my_scene.getSceneObjectFinalIndex();
    } else {
        x_init = 0;
        x_final = my_scene.getObjectCount();
    }
#else
    x_init = 0;
    x_final = my_scene.getObjectCount();
#endif
    
    for(int x = x_init; x < x_final; x++) {
        //check if ray intersects this object
        if((temp_collision_obj_ptr 
                = my_scene.getObject(x)->collision(temp_ray_ptr)) !=NULL 
                && temp_collision_obj_ptr->getDistance() < nearestDist) {
            //is dist the least?
            nearestDist = temp_collision_obj_ptr->getDistance();
            nearest_collision_ptr = temp_collision_obj_ptr;

            //TODO free temp_collision objects
        } else if(temp_collision_obj_ptr!=NULL) {
            free(temp_collision_obj_ptr);
        }
    }

    return nearest_collision_ptr;               
}

/***************************************************/
/***************************************************/
/********** PARALLIZED COLLISION CODE **************/
/***************************************************/
/***************************************************/
                                
/********** RAYTRACE_SERF Pixel Parrallel **********/
//COLLISION SIMD methods

#if HARDWARE_IS_TILE
RayPacket* bchan_serf_wait_for_ray() {
    
    size_t size;
    RayPacket *packet_ptr;
    packet_ptr =  (RayPacket*) ilib_bufchan_receive(port_receive_from_master, &size);
    ilib_bufchan_release_one(port_receive_from_master);
    
    return packet_ptr;
}

void rchan_serf_send_collision_info(CollisionObject* nearest_collision_ptr) {
    
    CollisionObjectPacket packet;
    
    if(nearest_collision_ptr==NULL) {
        packet.isNull= true;
    } else {
        packet.isNull = false;
        packet.collisionObject = *nearest_collision_ptr;
    }

    ilib_rawchan_send_buffer(port_serf_send, &packet, sizeof(packet));
}

void bchan_serf_send_collision_info(CollisionObject* nearest_collision_ptr) {
    
    CollisionObjectPacket packet;
    
    if(nearest_collision_ptr==NULL) {
        packet.isNull= true;
    } else {
        packet.isNull = false;
        packet.collisionObject = *nearest_collision_ptr;
    }

    ilib_bufchan_send(port_send_to_master, &packet, sizeof(packet));
}

/********************************************/
/***** Raytrace Serf Collision Parallel *****/
/********************************************/

int raytrace_serf_collision_parallel() {
    
    if(my_scene.initialize()) {
        return 1;
    }

    //then parse the scene (each serf only needs to know of part of the scene)
    if(my_scene.ParseObjectArray(my_local_rank, FIEFDOM_SIZE)) {
        return 1;
    }
    printf("Serf %d: StartIndex: %d, EndIndex: %d\n", my_local_rank, my_scene.getSceneObjectStartIndex(), my_scene.getSceneObjectFinalIndex());

    //benchmarking barrier!
    ilib_msg_barrier(ILIB_GROUP_SIBLINGS);

    bool finished = false;
    int command;

#if IS_FOR_SIMULATION
    int debug_counter =0;                      
#endif

    while(!finished) {

        //wait for ray...
        #if PARTIONING_STRATEGY==4
        RayPacket* packet_ptr =  bchan_serf_wait_for_ray();     
        #else
        //TODO don't malloc this stuff!
#if IS_FOR_SIMULATION
        debug_counter++;
        printf("     Serf(%d), Waiting for Ray, Round:%d\n"
                , my_global_rank
                , debug_counter
                );   
#endif
        RayPacket ray_packet;
        ilib_rawchan_receive_buffer(port_serf_receive, &ray_packet, sizeof(RayPacket));
        RayPacket* packet_ptr = &ray_packet;
#if IS_FOR_SIMULATION
        printf("     Serf(%d), Waiting for Ray, Round%d : Sucessful\n"
                , my_global_rank
                , debug_counter
                );
#endif
        #endif
        
        command = packet_ptr->command;

        if(command == FINISHED_COMMAND) {
            finished=true;
        }

        if(!finished) {
            if(command==COLLISION_COMMAND) { 
                CollisionObject *nearest_collision_ptr = NULL;
                nearest_collision_ptr = getCollision(&packet_ptr->ray);

                //send this information to the FIEFDOM_MASTER
                #if PARTIONING_STRATEGY==4
                bchan_serf_send_collision_info(nearest_collision_ptr);
                #else
#if IS_FOR_SIMULATION
                printf("     Serf(%d), Round:%d,  Sending Collision Info packetsize(%d) isNull: %s \n"
                        , my_global_rank
                        , debug_counter
                        , sizeof(CollisionObjectPacket)
                        , nearest_collision_ptr ==NULL ? "T" : "F"
                        );
#endif
                rchan_serf_send_collision_info(nearest_collision_ptr);
#if IS_FOR_SIMULATION
                printf("     Serf(%d), Round:%d, Sending Collision Info packetsize(%d) isNull: %s : SUCCESSFUL \n"
                        , my_global_rank
                        , debug_counter
                        , sizeof(CollisionObjectPacket)
                        , nearest_collision_ptr ==NULL ? "T" : "F"
                        );
                #endif
                #endif
                //TODO free the collisionobject now that we're done with it! (?)
                
            } else {
                //assume its inShade command
                bool inshade = inShadeCollisionDetection(&packet_ptr->ray, packet_ptr->distToLight);
                
                //send shade information to FIEFDOM_MASTER
                #if PARTIONING_STRATEGY==4
                ilib_bufchan_send(port_send_to_master, &inshade, sizeof(bool));
                #else
                #if IS_FOR_SIMULATION
                printf("     Serf(%d), Sending InShade Info \n"
                        , my_global_rank
                        );
                #endif
                ilib_rawchan_send(port_serf_send, inshade);
                #if IS_FOR_SIMULATION
                printf("     Serf(%d), Sending InShade Info : SUCCESSFUL\n"
                        , my_global_rank
                        );
                #endif
                #endif
            }
        }
    }
    
    //benchmarking barrier!
    ilib_msg_barrier(ILIB_GROUP_SIBLINGS);

    return 0;
}

/***** END OF RAYRACE_SERF ******/

/********************************/
/******* MASTER FUNCTIONS *******/
/********************************/

//BUG!! raw channel total buffer space is 96 words
int rchan_send_ray_to_serfs(int _command, Ray *temp_ray_ptr, sdecimal32 dist_to_light) {

    RayPacket packet;
    packet.command = _command; //command code 
    packet.distToLight = dist_to_light;

    if(temp_ray_ptr==NULL) {
        packet.isNull = true; // null ray pointer
    } else {
        packet.isNull = false;
        packet.ray = *temp_ray_ptr;
    }

    for(int i=0; i < FIEFDOM_SIZE -1; i++) {
        ilib_rawchan_send_buffer(master_send_ports[i], &packet, sizeof(RayPacket)); 
    }
    
    return 0;
}

int bchan_send_ray_to_serfs(int _command, Ray *temp_ray_ptr, sdecimal32 dist_to_light) {

    RayPacket packet;
    packet.command = _command; //command code 
    packet.distToLight = dist_to_light;

    if(temp_ray_ptr==NULL) {
        packet.isNull = true; // null ray pointer
    } else {
        packet.isNull = false;
        packet.ray = *temp_ray_ptr;
    }

    for(int i=0; i < FIEFDOM_SIZE -1; i++) {
        ilib_bufchan_send(port_send_to_serfs[i], &packet, sizeof(packet));
    }
    return 0;
}

//BUG!! raw channel total buffer space is 96 words
//collisionObject is 136 size....

//Combined Receiving information from serfs, 
//and returns the nearest CollisionObject
//CollisionObject* rchan_master_receive_reduce_collision_info(CollisionObjectPacket* packet_ptr, CollisionObject* initial_obj_ptr) {
//true if collision occured, false if not
bool rchan_master_receive_reduce_collision_info(CollisionObject* nearest_ptr, CollisionObject* initial_obj_ptr) {

    //OPTIMIZE is it better to do an array here, or to
    //have one packet buffer and copy any collision object that
    //is is the temp_nearest
    CollisionObjectPacket temp_packets[FIEFDOM_SIZE-1];
    int nearest_index=0;
    sdecimal32 nearestDist = FLOAT_MAX_VALUE;
    
    if(initial_obj_ptr!=NULL) {
        nearestDist = initial_obj_ptr->getDistance();
    }

    //receive packets from serfs
    for(int i=1; i < FIEFDOM_SIZE; i++) {

        ilib_rawchan_receive_buffer(port_sink, &temp_packets[i-1], sizeof(CollisionObjectPacket));

        if(!(temp_packets[i-1].isNull) 
            && temp_packets[i-1].collisionObject.getDistance() < nearestDist) {
            nearestDist = temp_packets[i-1].collisionObject.getDistance();
            nearest_index = i-1;
        }               
    }
    
    //NOTE: you *must* copy the data this pointer points to
    //because new messages can overwrite the buffer this points to
    //COPY the collision into the *nearest_ptr buffer (allocated on the stack).
    if(nearest_index!=0) {
        *nearest_ptr = temp_packets[nearest_index].collisionObject;
        return true; 
    } else if(initial_obj_ptr!=NULL) {
        *nearest_ptr = *initial_obj_ptr;
        return true;
    } else {
        //no collision occured
        return false;
    }
}

//the "return value" is written into nearest_ptr buffer
bool bchan_master_receive_reduce_collision_info(CollisionObject* nearest_ptr, CollisionObject* initial_ptr) {

    size_t size;
    CollisionObjectPacket *packet_ptr;
    sdecimal32 nearestDist = FLOAT_MAX_VALUE;
    CollisionObject* return_ptr = initial_ptr;

    if(initial_ptr!=NULL) {
        nearestDist = initial_ptr->getDistance();
    }

    for(int i=1; i < FIEFDOM_SIZE; i++) {

        packet_ptr =  (CollisionObjectPacket*) ilib_bufchan_receive(port_receive_from_serfs[i-1], &size);   

        if(!(packet_ptr->isNull) 
                && packet_ptr->collisionObject.getDistance() < nearestDist) {
            nearestDist = packet_ptr->collisionObject.getDistance();
            return_ptr = &(packet_ptr->collisionObject); //you *must* copy this later
        }               

        ilib_bufchan_release_one(port_receive_from_serfs[i-1]);
    }

    //NOTE: you *must* copy the data this pointer points to
    //because new messages can overwrite the buffer this points to
    if(return_ptr!=NULL) {
        *nearest_ptr = *return_ptr;
        return true; //nearest isn't null
    } else {
        return false;
    }
}

/*************************/
/* Reduce Collision Info */
/*************************/
/*
CollisionObject* bchan_reduce_collision_info() {

    / ****Find nearestObject that the eyeRay intersects**** /
    sdecimal32 nearestDist = FLOAT_MAX_VALUE;
    
    CollisionObject *nearest_collision_ptr = NULL;
    CollisionObject *temp_collision_obj_ptr = NULL;
    
    for(int x=0; x< FIEFDOM_SIZE; x++) {

        if((temp_collision_obj_ptr = collision_array[x]) !=NULL 
                && temp_collision_obj_ptr->getDistance() < nearestDist) {
            //is dist the least?
            nearestDist = temp_collision_obj_ptr->getDistance();
            nearest_collision_ptr = temp_collision_obj_ptr;
        }
    }

    return nearest_collision_ptr;               
}
*/
/*************************/
/*  Reduce InShade Info  */
/*************************/
bool rchan_reduce_inshade_info(bool init_val) {

    bool data;
    bool return_value = init_val;

    for(int i=1; i < FIEFDOM_SIZE; i++) {
        ilib_rawchan_receive_buffer(port_sink, &data, sizeof(bool));
        if(data) return_value = true;
    }

    return return_value;
}

bool bchan_reduce_inshade_info(bool init_val) {

    //get Shade Info
    size_t size;
    bool *data;
    bool return_value = init_val;

    //you have to go thru the entire list of guys to clear the buffers
    for(int i=1; i < FIEFDOM_SIZE; i++) {
        data  = (bool*) ilib_bufchan_receive(port_receive_from_serfs[i-1], &size);
        if(*data) return_value = true;      
        ilib_bufchan_release_one(port_receive_from_serfs[i-1]);
    }

    return return_value;
}

#endif //#if HARDWARE_IS_TILE
/*****************************************/
/*****************************************/
/********** CALCULATE PIXEL **************/
/*****************************************/
/*****************************************/

CelioRayTracer::vector3d calculatePixel(CelioRayTracer::Ray *temp_ray_ptr
                                        , int recursion_level) {
    
    if(temp_ray_ptr==NULL) 
        return NULL_COLOR; //this denotes, for example, that there exists no refractive ray.
    
    if(recursion_level > MAX_RECURSION_LEVEL)
        return NULL_COLOR;

    /* Lots of Magic happens here.
     * loop through all the objects in the scene to see
     * which one we collide with */

    //if parallized collision strategy....
#if PARTIONING_STRATEGY == 4 || PARTIONING_STRATEGY == 7
    CollisionObject *nearest_collision_ptr = NULL;
    CollisionObject nearest_collision;

    #if PARTIONING_STRATEGY == 4 
        bchan_send_ray_to_serfs(COLLISION_COMMAND, temp_ray_ptr); 
    #else //Part_strag==7
        rchan_send_ray_to_serfs(COLLISION_COMMAND, temp_ray_ptr); 
    #endif
    
    nearest_collision_ptr = getCollision(temp_ray_ptr);
    
    //collections and reconciles all collision data
    //pass in the master's collision info too
    #if PARTIONING_STRATEGY == 4 
        if(bchan_master_receive_reduce_collision_info(&nearest_collision, nearest_collision_ptr)) {
            delete nearest_collision_ptr;
            nearest_collision_ptr = &nearest_collision;
        } else {
            delete nearest_collision_ptr;
            nearest_collision_ptr = NULL;
        };
        
    #else //Part_strat==7
        //this function copies the collision object into the nearest_collision buffer
        //but we also need to make sure nearest_collision_ptr points to it (or NULL) 
    
        if(rchan_master_receive_reduce_collision_info(&nearest_collision, nearest_collision_ptr)) {
            //great a collision occured
            delete nearest_collision_ptr; //unallocate the getCollision() object
            nearest_collision_ptr = &nearest_collision;
        } else {
            //no collision occured
            delete nearest_collision_ptr;
            nearest_collision_ptr = NULL;   
        };
    #endif
    
#else       
    //do not paralleize collision code
    CollisionObject *nearest_collision_ptr = NULL;
    nearest_collision_ptr = getCollision(temp_ray_ptr);
#endif

    /* No Object Collision -> Return Background Color */
    if(nearest_collision_ptr==NULL) {
        return NULL_COLOR;
    } 
    
    //this is the final color, to combine colors we multiply together
    //keep values between between [0,1]
    CelioRayTracer::Color final_color; 
    
    CelioRayTracer::Color object_color = 
                        CelioRayTracer::Color(nearest_collision_ptr->getColor());
    CelioRayTracer::Color reflective_color;// = null;
    CelioRayTracer::Color refractive_color;// = null;
    
    if (nearest_collision_ptr->hitALightSource()) {
        //TODO could we have a reflective light source? ie, continue on with calculations?
        sdecimal32 intensity = nearest_collision_ptr->getIntensityFactor(); 
        //BUG do I need to get the collision object?
        Color light_color = Color(nearest_collision_ptr->getColor());
        final_color = intensity*light_color;
        return final_color;
    }

    //if i am here, we hit something and its not a light!
    object_color = Color(nearest_collision_ptr->getColor());
    
    /****** COSINE SHADING & SPECULAR SHADING ******/
    //intensity = diffuse_factor * (L.N) + specular_factor * (V.R)n
    //intensity = cosineShading + SpecularShading
    //loop thru each light object
    
    CelioRayTracer::SceneObject *light_obj_ptr;
    
    //TODO this is yet another bug b/c i don't have all of the objects in the scene
    for(int i=0; i < my_scene.getObjectCount(); i++) {
        
        light_obj_ptr = my_scene.getObject(i);
        
        if(light_obj_ptr->checkIsaLightSource()) {
            
            #if SHADOWS_ON
            //check if IntersectionPoint is in shade
            bool ip_inShade = inShade(light_obj_ptr, nearest_collision_ptr);
            #else
            bool ip_inShade = false;
            #endif
            
            if(!ip_inShade) {
                cosineShade(&final_color, &object_color
                                , light_obj_ptr     //light information
                                , nearest_collision_ptr);       //collision information
                
                /****************************/
                /* Specular Shading */
                
                CelioRayTracer::vector3d L = light_obj_ptr->getOrigin() 
                                - nearest_collision_ptr->getIntersectionPoint();
                L.normalize();
                
                CelioRayTracer::Color light_color = light_obj_ptr->getMaterial()->getColor();
                CelioRayTracer::vector3d N = nearest_collision_ptr->getNormalRay().getDirection();
                N.normalize();

                //vector3 R = L - 2.0f * DOT( L, N ) * N;
                CelioRayTracer::vector3d R;
                R = L - 2.0f * L.dot(N) * N;
                
                CelioRayTracer::vector3d V = temp_ray_ptr->getDirection();
                                                               
                //TODO is this really be calculated correctly?
                //i assume so, as the scene looks good...
                sdecimal32 dot = V.dot(R);
                
                if(dot > (sdecimal32) 0) {
                    //pow(dot, 20) is WAY too slow (7.5% of total run time!, or 1.6us/pixel)
                    //this change actually got us a 15% speed increase!
                    sdecimal32 pow_factor = dot;
                    for(int i=0; i < 19; i++) {
                        pow_factor *= dot;
                    }
                    sdecimal32 spec_factor = pow_factor * nearest_collision_ptr->getSpecularFactor();
                    final_color += spec_factor * light_color;
                }
            }
        }
    }
    
    /****** CHECK REFLECTED RAY ******/
    
    #if REFLECTIONS_ON
    if(nearest_collision_ptr->isReflective()) {
        //recurse
        CelioRayTracer::Ray temp_ray = nearest_collision_ptr->getReflectedRay();

        reflective_color = calculatePixel(&(temp_ray),(recursion_level+1));
        final_color += nearest_collision_ptr->getReflectiveFactor() * reflective_color * object_color;
        
    }
    #endif
    
    /****** CHECK REFRACTED RAY ******/
    /* TODO add support for refracted rays
    if(nearest_collision_ptr.getTranparent()) {
        
        refractive_color = new NormalizedColor(calculatePixel(nearest_collision_ptr.getRefractedRay(), (recursion_level+1)));

        if(refractive_color!=null) {
            final_color.x+= (refractive_color.getRed());
            final_color.y += (refractive_color.getGrn());
            final_color.z += (refractive_color.getBlue());
        }
    }*/
                                    
    /*Yes, this check is necessary */
    //TODO make this a vector3d function
/* #if USING_FIXED_POINT 
    CelioRayTracer::vector3d::fixed one(1.0f);
    final_color.x = (final_color.x > one) ? one : final_color.x;
    final_color.y = (final_color.y > one) ? one : final_color.y;
    final_color.z = (final_color.z > one) ? one : final_color.z;
#else
    final_color.x = (final_color.x > 1.0f) ? 1.0f : final_color.x;
    final_color.y = (final_color.y > 1.0f) ? 1.0f : final_color.y;
    final_color.z = (final_color.z > 1.0f) ? 1.0f : final_color.z;
#endif
*/

#if !(PARTIONING_STRATEGY==4 || PARTIONING_STRATEGY==7)
    //TODO anything else to deallocate?
    delete nearest_collision_ptr;
#endif
    return final_color;
}

    /********************END OF COLLISION ********************/

/********** COSINE SHADING **************/

/** Cosine Shading
 * 
 * shade = dot_product( light_vector, normal_vector )
 * if ( shade < 0 )
 *  shade = 0
 * point_color = object_color * ( ambient_coefficient + 
 * diffuse_coefficient * shade )
 *
 */

CelioRayTracer::Color* cosineShade(CelioRayTracer::Color* final_color_ptr
                                    , CelioRayTracer::Color* object_color_ptr
                                    , CelioRayTracer::SceneObject *light_object_ptr
                                    , CelioRayTracer::SceneObject::CollisionObject *collision_ptr) {
    // TODO Create Shading method, take in nearest Distance, distance to light source..?
        
    //light_ray = light - intersection_point
    CelioRayTracer::vector3d light_ray = CelioRayTracer::vector3d();
    CelioRayTracer::Color light_color = CelioRayTracer::Color(light_object_ptr->getMaterial()->getColor()); 
    //BUG wrong color

    //TODO this could be made to be objectMaterial specific..... (the diffuse_coefficient)
    //double diffuse_coefficient = 1- scene.ambient_coefficient;
    //TODO build in ambient_coefficient
//  sdecimal32 diffuse_coefficient = my_scene.getObject(collision_ptr->getCollisionObjectIndex())->getMaterial()->getDiffuseFactor();
    sdecimal32 diffuse_coefficient = collision_ptr->getDiffuseFactor();
    sdecimal32 light_intensity = light_object_ptr->getIntensity();
    //double factor=1.0; //this is the return val that scales the "shading" of the object
        
    //light_ray = CelioRayTracer::vector3d(collision.getIntersectionPoint());
    //light_ray.scaleAdd(-1, light_object_ptr.getOrigin());
    light_ray = light_object_ptr->getOrigin() - collision_ptr->getIntersectionPoint();
    light_ray.normalize();
        
    if(diffuse_coefficient > (sdecimal32) 0) {
            
        sdecimal32 cosine_dot_factor = collision_ptr->getNormalRay().getDirection().dot(light_ray);
        //if dot product > 0 .... else do not continue.
        if(cosine_dot_factor > (sdecimal32) 0) {
                
            sdecimal32 factor = cosine_dot_factor = cosine_dot_factor * diffuse_coefficient * light_intensity;
                
            final_color_ptr->x += factor * object_color_ptr->x * light_color.x;
            final_color_ptr->y += factor * object_color_ptr->y * light_color.y;
            final_color_ptr->z += factor * object_color_ptr->z * light_color.z;
                
        }

        //TODO is this check necessary at this moment???
        //BUG i think this code is wrong for fixed_point!
        //TODO "CapColorValues" roll this into a vector3d function
        final_color_ptr->x = (final_color_ptr->x > (sdecimal32) 1.0f) ? (sdecimal32) 1.0f : final_color_ptr->x;
        final_color_ptr->y = (final_color_ptr->y > (sdecimal32) 1.0f) ? (sdecimal32) 1.0f: final_color_ptr->y;
        final_color_ptr->z = (final_color_ptr->z > (sdecimal32) 1.0f) ? (sdecimal32) 1.0f : final_color_ptr->z;
    }
        
    return final_color_ptr;//pointless because you gave me it!
}
    

/********** IN SHADING **************/

/*Find if the ray towards the light intersects anything*/
//TODO is it possible to roll this collision detection into the other ecollision
//detection method? this would be to improve L1 Instruction Cache misses (currently 25%).
bool inShadeCollisionDetection(CelioRayTracer::Ray* light_ray_ptr, sdecimal32 dist_to_light) {
    
    bool inShade = false;
    CelioRayTracer::SceneObject* temp_obj;
    CelioRayTracer::SceneObject::CollisionObject *temp_collision_obj_ptr = NULL;
        
    int x_init, x_final;
    if(my_local_rank==FIEFDOM_MASTER_RANK) {
        x_init = my_scene.getSceneObjectStartIndex();
        x_final = my_scene.getSceneObjectFinalIndex();
    } else {
        x_init = 0;
        x_final = my_scene.getObjectCount();
    }   
    
    for(int x=x_init; x<x_final && !inShade; x++) {
        temp_obj = my_scene.getObject(x);
            
        if(!(temp_obj->checkIsaLightSource())
            && (temp_collision_obj_ptr = temp_obj->collision(light_ray_ptr)) != NULL 
            && temp_collision_obj_ptr->getDistance() < dist_to_light) {
                inShade = true;
        }
                
        if(temp_collision_obj_ptr!=NULL) {
            delete temp_collision_obj_ptr; //sets it to null too =)
        }
    }

    return inShade;
}

/* return if we are shaded from *light_ptr
 * TODO does not take into account the distance to the light source */
bool inShade(CelioRayTracer::SceneObject *light_ptr
    , CollisionObject *collision_ptr) {
        
    //Calculate lightRay
    CelioRayTracer::vector3d dir;
    dir = light_ptr->getOrigin() - collision_ptr->getIntersectionPoint();
        
    sdecimal32 dist_to_light = dir.length(); 
        
    CelioRayTracer::Ray lightRay(collision_ptr->getIntersectionPoint(),dir);

    #if PARTIONING_STRATEGY==4
        //parallelize collision detection
        bchan_send_ray_to_serfs(INSHADE_COMMAND, &lightRay, dist_to_light);
        bool temp;
        temp = inShadeCollisionDetection(&lightRay, dist_to_light);
        return bchan_reduce_inshade_info(temp);
    #endif
    #if PARTIONING_STRATEGY==7
        //parallelize collision detection
        rchan_send_ray_to_serfs(INSHADE_COMMAND, &lightRay, dist_to_light);
        bool temp = inShadeCollisionDetection(&lightRay, dist_to_light);
        return rchan_reduce_inshade_info(temp);
    #endif
    #if !(PARTIONING_STRATEGY==4 || PARTIONING_STRATEGY==7)
        return inShadeCollisionDetection(&lightRay, dist_to_light);
    #endif
        
}


/******* DISTRIBUTED WORKQUEUE SERF WORK *********/

#if PARTIONING_STRATEGY==3
/*********************************************************/
/* Instead use self-scheduling via Distributed workQueue */
/*********************************************************/
    
int serf_distributed_main() {
//    printf("Begin Main Loop:  Serf (%d, %d, %d) \n"
//          , my_global_rank
//          , my_local_rank
//          , my_fiefdom_index);
    
    //we want every core to have his own private
    //copy of the scene
    if(my_scene.initialize()) {
        return 1;
    }

ilib_msg_barrier(ILIB_GROUP_SIBLINGS);

bool finished = false;
bool no_boss_work = false;

    while(!finished) {
        
        ilib_mutex_lock(&bossWorkQueue->mutex);

        if(bossWorkQueue->isQueueEmpty()) { 
            ilib_mutex_unlock(&bossWorkQueue->mutex);
            no_boss_work= true;
        } else {
            //TODO benchmark which way is faster
            //access shared memory after the fact to get x,z
            //or take longer inside mutexes with constructor to copy the pixel obj
            CelioRayTracer::PixelQueue::PixelObject* pixelObj = bossWorkQueue->dequeue();
            ilib_mutex_unlock(&bossWorkQueue->mutex);

            int x = pixelObj->getX();
            int z = pixelObj->getZ();
            CelioRayTracer::Ray *temp_ray_ptr = my_camera.createEyeRay(
                                    ((float) x) / SCREEN_HORIZONTAL_RESOLUTION,
                                    ((float) z) / SCREEN_VERTICAL_RESOLUTION);
            pixels[x][z] = calculatePixel(temp_ray_ptr, 0);
        
#if IS_FOR_SIMULATION
            printf("   Serf(%d, %d, %d): Finished Pixel (%d, %d) \n"
                    , my_global_rank
                    , my_local_rank
                    , my_fiefdom_index
                    , x
                    , z);
#endif
            if(temp_ray_ptr!=NULL) free(temp_ray_ptr);
            if(pixelObj!=NULL) free(pixelObj);
        }
        
        if(no_boss_work) {
            no_boss_work = false;
            //wait for more work
            
            ilib_mutex_lock(&fiefdom_finished->mutex);
            finished = *fiefdom_finished->boolVal;
            ilib_mutex_unlock(&fiefdom_finished->mutex);
        }
    }
            
    //wait for all processors to finish before stopping the clock
    ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
    return 0;
}
/******************************************/
#endif
/******************************************/

/***********************************/
/***********************************/
/********** RAYTRACE_MAIN **********/
/***********************************/
/***********************************/

int raytrace_main() {
    
    printf("Global Rank(%d) begins\n", my_global_rank);
    
    #if !IS_FOR_SIMULATION
    if(my_global_rank == GLOBAL_MASTER_RANK) {
        printf("Opening Log.\n");
        if(init_log()) {
            return 1;
        }
    }       
    #endif

    //we want every core to have his own private
    //copy of the scene
#if SCENE==1
    if(my_scene.initialize()) {
        return 1;
    }
#else
    if(my_scene.initializeTwoMirrors(&my_camera)) {
        return 1;
    }
#endif

#if PARTIONING_STRATEGY==4 || PARTIONING_STRATEGY==7
    //limit which scene objects we look at
    my_scene.SetObjectIndices(my_local_rank, FIEFDOM_SIZE);
    printf("Boss: StartIndex: %d, EndIndex: %d\n", my_scene.getSceneObjectStartIndex(), my_scene.getSceneObjectFinalIndex());
#endif

    /*** START BENCHMARKING ***/
#if HARDWARE_IS_TILE
    ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
    profiler_enable();
#endif
    if(my_global_rank == GLOBAL_MASTER_RANK) {
        printf("****** Start Ray Tracing. *******\n");
        clock_t start_time =clock();

#if HARDWARE_IS_TILE
        start_cycle_count = get_cycle_count();
#endif
    }
        
    /******************************************/
    #if (PARTIONING_STRATEGY==1 || PARTIONING_STRATEGY==0)
    /******************************************/
    
    int dz = SCREEN_VERTICAL_RESOLUTION / CORE_NUM;
    int z_min = my_global_rank * dz;
    int z_max = my_global_rank * dz + dz;
    
//  printf("Rank(%d), z e ( %d, %d)\n", my_global_rank, z_min, z_max);

    //for every pixel....
    for(int x=0; x < SCREEN_HORIZONTAL_RESOLUTION; x++) {
        for(int z=z_min; z < z_max; z++) {
            //screen is X-Z axis (Z is up). Y is into the screen

            //create Ray from eye through the screen
            CelioRayTracer::Ray *temp_ray_ptr = my_camera.createEyeRay( 
                                    ((float) x) / SCREEN_HORIZONTAL_RESOLUTION,
                                    ((float) z) / SCREEN_VERTICAL_RESOLUTION);
//          printf("Rank (%d) (%d, %d)\n",my_global_rank, x, z);
            pixels[x][z] = calculatePixel(temp_ray_ptr, 0);
            free(temp_ray_ptr);
        }
    }
    
    /******************************************/
    #endif
    /******************************************/

    /******************************************/
    #if (PARTIONING_STRATEGY==5)
    /******************************************/
    /**      'smart' static partioning       **/
    /******************************************/
    
    int start_x = my_global_rank % STATIC_PARTION_X_WIDTH;
    int start_z = my_global_rank / STATIC_PARTION_X_WIDTH;
    
    //for every pixel....
    for(int x=start_x; x < SCREEN_HORIZONTAL_RESOLUTION; x+=STATIC_PARTION_X_WIDTH) {
        for(int z=start_z; z < SCREEN_VERTICAL_RESOLUTION; z+=STATIC_PARTION_Z_WIDTH) {
            //screen is X-Z axis (Z is up). Y is into the screen

            //create Ray from eye through the screen
            CelioRayTracer::Ray *temp_ray_ptr = my_camera.createEyeRay( 
                                    ((float) x) / SCREEN_HORIZONTAL_RESOLUTION,
                                    ((float) z) / SCREEN_VERTICAL_RESOLUTION);
            pixels[x][z] = calculatePixel(temp_ray_ptr, 0);
            free(temp_ray_ptr);
        }
    }
    
    /******************************************/
    #endif
    /******************************************/

#if PARTIONING_STRATEGY==2 || PARTIONING_STRATEGY==4 || PARTIONING_STRATEGY==7
    /******************************************/
    /*    use self-scheduling via workQueue   */
    /******************************************/
    bool finished = false;

    while(!finished) {
        
        ilib_mutex_lock(&workQueue->mutex);

        if(workQueue->isQueueEmpty()) { 
            ilib_mutex_unlock(&workQueue->mutex);
            finished = true;
        } else {
            //TODO benchmark which way is faster
            //access shared memory after the fact to get x,z
            //or take longer inside mutexes with constructor to copy the pixel obj
            CelioRayTracer::PixelQueue::PixelObject* pixelObj = workQueue->dequeue();
            ilib_mutex_unlock(&workQueue->mutex);

            int x = pixelObj->getX();
            int z = pixelObj->getZ();
            
            #if IS_FOR_SIMULATION
            printf("  Rank(%d) Pixel(%d, %d)\n", 
                    my_global_rank, x, z);
            #endif
            
            CelioRayTracer::Ray *temp_ray_ptr = my_camera.createEyeRay(
                                    ((float) x) / SCREEN_HORIZONTAL_RESOLUTION,
                                    ((float) z) / SCREEN_VERTICAL_RESOLUTION);
            pixels[x][z] = calculatePixel(temp_ray_ptr, 0);
        
            if(temp_ray_ptr!=NULL) free(temp_ray_ptr);
            if(pixelObj!=NULL) free(pixelObj);
        }
    }

    //send finished signal to serfs
#if PARTIONING_STRATEGY==4
    bchan_send_ray_to_serfs(FINISHED_COMMAND, NULL);
#endif
#if PARTIONING_STRATEGY==7
    rchan_send_ray_to_serfs(FINISHED_COMMAND, NULL);
#endif


    /******************************************/
#endif
    /******************************************/
    /******************************************/
#if PARTIONING_STRATEGY==3 
    /******************************************/
    /*** Hierarchical Distributed WorkQueue ***/
    /******************************************/

//  int size;
//  ilib_mutex_lock(&workQueue->mutex);
//  size = workQueue->queue_size();
//  ilib_mutex_unlock(&workQueue->mutex);
    
    bool finished = false;

    while(!finished) {
        
        //TODO optimize this!
        int boss_work_count = MIN_BOSS_WORK_COUNT + 1; 
        int volatile dummy=50;
        int start_cycle;
        int end_cycle;
        while(boss_work_count > MIN_BOSS_WORK_COUNT) {
            
            ilib_mutex_lock(&bossWorkQueue->mutex);
            boss_work_count = bossWorkQueue->queue_size();
            ilib_mutex_unlock(&bossWorkQueue->mutex);
            
            start_cycle = get_cycle_count();
            end_cycle = start_cycle;
            while(end_cycle > dummy + start_cycle) {
                end_cycle = get_cycle_count();
            }
        }

//      ilib_mutex_lock(&bossWorkQueue->mutex);
//      printf("Boss%d his QueueSize: %d\n", my_fiefdom_index
//              , bossWorkQueue->queue_size());
//      ilib_mutex_unlock(&bossWorkQueue->mutex);
        
        
        //get work from global workQueue
        ilib_mutex_lock(&workQueue->mutex);
//      printf("Boss%d B_GlobalQueueSize: %d\n", my_fiefdom_index
//              , workQueue->queue_size());

        if(workQueue->isQueueEmpty()) { 
//          printf("Boss%d A_GlobalQueueSize: %d\n", my_fiefdom_index
//              , workQueue->queue_size());
//          printf("** BOSS: %d , WORK QUEUE IS EMPTY\n", my_fiefdom_index);
            ilib_mutex_unlock(&workQueue->mutex);
            finished = true;
            //TODO i don't think i really need to like this!
            ilib_mutex_lock(&fiefdom_finished->mutex);
            *(fiefdom_finished->boolVal) = true;
            ilib_mutex_unlock(&fiefdom_finished->mutex);
        } else {
            CelioRayTracer::PixelQueue::PixelObject* pixelObj = workQueue->dequeue();
            
            ilib_mutex_lock(&bossWorkQueue->mutex);

            //add global work into the boss queue
            int i=0;
            bool global_is_empty = false;
            while(i < BATCH_GRAB_WORK_COUNT && !global_is_empty) {
                bossWorkQueue->enqueue(workQueue->dequeue());
                global_is_empty = workQueue->isQueueEmpty();
                ++i;
            }

            ilib_mutex_unlock(&bossWorkQueue->mutex);
            ilib_mutex_unlock(&workQueue->mutex);
        }
    }
             
#endif //distributed workqueue

    /*** STOP BENCHMARKING ***/
    //TODO benchmark individual cores, before barrier
#if HARDWARE_IS_TILE
    ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
    
    profiler_disable();
#endif
    if(my_global_rank == GLOBAL_MASTER_RANK) {
        clock_t stop_time =clock();
        run_time_s = ( double (stop_time - start_time)) / CLOCKS_PER_SEC; //total_time_ns;// / 1.0E9;

#if HARDWARE_IS_TILE
        end_cycle_count = get_cycle_count();
        printf("StartCycle: %lld\n", start_cycle_count);
        printf("EndCycle: %lld\n", end_cycle_count);
        run_time_s = (end_cycle_count-start_cycle_count)/CLOCK_SPEED; //period is important 
#endif
        //run_time_s= stop_time - start_time; //resolution is in seconds (not good enough)
        run_time_ns = run_time_s * 1.0E9;
        run_time_us = run_time_s * 1.0E6;
        printf("Finished.\n");
        printf("Total_Time (s) (Time.h)    : %f\n", ( double (stop_time - start_time)) / CLOCKS_PER_SEC);
        printf("Total_Time (s) (Archlib.h) : %llf\n", run_time_s);
        printf("AverageRoundTime (us/pixel): %f\n", run_time_us / (SCREEN_HORIZONTAL_RESOLUTION * SCREEN_VERTICAL_RESOLUTION));
    }
    #if !IS_FOR_SIMULATION
    if(my_global_rank == GLOBAL_MASTER_RANK) {
        printf("PrintScreen to Log.\n");
        printPixelsToLog();
    }
    #endif

    return 0;
}

/**************************/
/**************************/
/********** MAIN **********/
/**************************/
/**************************/

int main() {
    
    
    /************************/

    /*** Fixed Point Test ***/
    
    fixed a = 32.4;
    fixed b = 4444;
    
    printf("\nA: %f, B: %f\n\n", fix2float<16>(a.intValue), fix2float<16>(b.intValue));
    //printf("\nA: %f, B: %f\n\n", a, fix2float<16>(b.intValue));
    
    
    
    
    

    /************************/
    

#if HARDWARE_IS_TILE
    ilib_init();
#endif
    
    fflush(NULL); 

    if(CORE_NUM == 1) {
        std::cout << "Single-Core RayTracing!" << std::endl <<std::endl; 
    } else {

#if HARDWARE_IS_TILE
        if(ilib_group_size(ILIB_GROUP_SIBLINGS)==1) {
                printf("\nMulti-Core RayTracing!\n\n");
        }
#endif
    }

//#if IS_FOR_SIMULATION
    //clear the profiler
    #if HARDWARE_IS_TILE
    profiler_disable();
    profiler_clear();
    #endif
//#endif

#if HARDWARE_IS_TILE
    if(ilib_group_size(ILIB_GROUP_SIBLINGS) == 1)
#endif 
    {
    printf("  %d NUMBER OF CORES\n", CORE_NUM);
#if PARTIONING_STRATEGY==3 || PARTIONING_STRATEGY==4 || PARTIONG_STRATEGY==7
    printf("  %d NUMBER OF FIEFDOMS\n",  NUMBER_OF_FIEFDOMS);
    printf("  %d CORES PER FIEFDOM\n", FIEFDOM_SIZE);
#endif
    }
    
    //#1 SET UP ALL SHARED MEMORY (this is before we spawn procs)
    
    //Set up the shared pixel field that all cores will write too
#if HARDWARE_IS_TILE
    //only allocate shared memory if we are the only proc running
    if(ilib_group_size(ILIB_GROUP_SIBLINGS) == 1) 
    //I'm assuming if its not TILE hardware, there is only one core
    {
        printf("Setting up Static Partioning Shared Memory.\n");
        pixels = (CelioRayTracer::vector3d**) malloc_shared(SCREEN_HORIZONTAL_RESOLUTION*sizeof(CelioRayTracer::vector3d*));
        if(!pixels)
            ilib_die("Out of Memory.\n");

        for(int i=0; i < SCREEN_HORIZONTAL_RESOLUTION; i++) {
            pixels[i] = (CelioRayTracer::vector3d*) malloc_shared(SCREEN_VERTICAL_RESOLUTION*sizeof(CelioRayTracer::vector3d)); //should sizeof(vector*) ??
            if(!pixels[i])
                ilib_die("Out of Memory.\n");
        }

        ilib_mem_fence(); //wait for pointers to be placed into DRAM
    
    }
#endif
    
    
    //SET UP THE GLOBAL WORKQUEUE
    #if PARTIONING_STRATEGY==2 || PARTIONING_STRATEGY==3 || PARTIONING_STRATEGY==4 || PARTIONING_STRATEGY==7 //self-scheduling
    //assume that HARDWARE_IS_TILE if PART_STRATEGY==2
    if(ilib_group_size(ILIB_GROUP_SIBLINGS)==1) {
        printf("Setting up Self-Scheduling WorkQueue Shared Memory.\n");
        workQueue = (CelioRayTracer::PixelQueue *) malloc_shared(sizeof(CelioRayTracer::PixelQueue));
        if(workQueue==NULL) ilib_die("Failed to Allocate workQueue.\n");
        ilib_mutex_init(&(workQueue->mutex));

        //put all of the pixels into the workQueue
        ilib_mutex_lock(&workQueue->mutex);
        for(int x=0; x < SCREEN_HORIZONTAL_RESOLUTION; x++) {
            for(int z=0; z < SCREEN_VERTICAL_RESOLUTION; z++) {
                CelioRayTracer::PixelQueue::PixelObject* new_pixel_ptr;
                new_pixel_ptr = (CelioRayTracer::PixelQueue::PixelObject *) malloc_shared(sizeof(CelioRayTracer::PixelQueue::PixelObject));
                if(!new_pixel_ptr) ilib_die("Malloc failure.");
                new_pixel_ptr->setLoc(x,z);
                workQueue->enqueue(new_pixel_ptr);
            }
        }
        ilib_mutex_unlock(&workQueue->mutex);

        ilib_mem_fence();
    }   
    #endif //of allocating work-queue

    //SPAWN NEW PROCESSES: (feed them the shared pointers through the init_struct)
#if HARDWARE_IS_TILE
    if (ilib_group_size(ILIB_GROUP_SIBLINGS) == 1 && CORE_NUM != 1) {
        spawn_processes();
    }

    my_global_rank = ilib_group_rank(ILIB_GROUP_SIBLINGS);

    if(CORE_NUM!=1) {
        //broadcast all shared-memory pointers to all of the tiles
        //  broadcast_shared_pointers();
        //aquire shared memory pointers through the InitStruct
        size_t init_size = 0;
        InitStruct* init_info_struct = (InitStruct*) ilib_proc_get_init_block(&init_size); 
        if(init_size==0) ilib_die("Failure to get 'init_info_struct'.\n");
        pixels = init_info_struct->sharedmem_pixels_ptr;
        workQueue = init_info_struct->sharedmem_workqueue_ptr;
    }
#if PARTIONING_STRATEGY==3 
    //GROUP MANAGEMENT: Seperate out all cores into smaller groups
    //for hierarchical partioning scheme
    
    distribute_procs_to_fiefdoms();
    
    if(my_local_rank == BOSS_RANK) {
        boss_setup_workqueue();
    }   

    //this shouldn'tn be necessary, but stupid guys are going ahead somehow
    ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
    broadcast_distributed_workqueue_pointer();
    
#endif //end of partioning strategy==3
#if PARTIONING_STRATEGY==4 || PARTIONING_STRATEGY==7
    //GROUP MANAGEMENT: Seperate out all cores into smaller groups
    //for hierarchical partioning scheme
    //scheme 4 ==buffer channels, scheme 7 ==raw channels

    if(my_global_rank==GLOBAL_MASTER_RANK) {
        printf("Distributing Processes to Fiefdoms.\n");
    }
    distribute_procs_to_fiefdoms();

    /* Direct Message Communication Set Up */
    if(my_global_rank==GLOBAL_MASTER_RANK) {
        printf("Setup for Parallel Collision Communications.\n");
    }
    
    #if PARTIONING_STRATEGY==4
        bchan_set_up_parallel_collision_communications();            
    #else
        rchan_set_up_parallel_collision_communications();            
    #endif
    
    if(my_global_rank==GLOBAL_MASTER_RANK) {
        printf("Communication Setup Finished.\n");
    }
#endif //end of partitioning strategy==4, partioning scheme 7
#endif //end of hardware_is_tile

    /* And then some magic happens.... */
#if PARTIONING_STRATEGY==0 || PARTIONING_STRATEGY==1 || PARTIONING_STRATEGY==2 || PARTIONING_STRATEGY==5 
    raytrace_main();   
#endif

    //distributed self-scheduling
#if PARTIONING_STRATEGY==3
    ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
    if(my_local_rank==0)
        raytrace_main();    
    else 
        serf_distributed_main();
#endif

    //hierarchical, SIMD collision detection
#if PARTIONING_STRATEGY==4 || PARTIONING_STRATEGY==7
    if(my_local_rank==FIEFDOM_MASTER_RANK)
        raytrace_main();                                                          
    else 
        raytrace_serf_collision_parallel();
#endif

    /* And then we're done! */
    if(my_global_rank==GLOBAL_MASTER_RANK)
        printf("Program Done.\n");

#if HARDWARE_IS_TILE
    ilib_finish();
#endif

    return 0;
}

/************************************/
/************************************/
/*********** END OF MAIN ************/
/************************************/
/************************************/

#if HARDWARE_IS_TILE

/**********************************/
/*      boss setup workqueue      */
/**********************************/


int boss_setup_workqueue() {
    printf("Boss of Fiefdom: %d\n   Setting up Self-Scheduling Distributed WorkQueue Shared Memory.\n", my_fiefdom_index);
    bossWorkQueue = (CelioRayTracer::PixelQueue *) malloc_shared(sizeof(CelioRayTracer::PixelQueue));
    if(bossWorkQueue==NULL) ilib_die("Failed to Allocate bossWorkQueue.\n");
    ilib_mutex_init(&(bossWorkQueue->mutex));

    //shared bool value that tells serfs that the fiefdom is finished
    fiefdom_finished = (LockedBool*) malloc_shared(sizeof(LockedBool));
    ilib_mutex_init(&(fiefdom_finished->mutex));
    fiefdom_finished->boolVal = (bool*) malloc_shared(sizeof(bool));
    *(fiefdom_finished->boolVal) = false;
    //TODO delete mutex on boolVal
    ilib_mem_fence();

    //pre-load boss arrays
    //put all of the pixels into the workQueue
    ilib_mutex_lock(&bossWorkQueue->mutex);
    //get some pixels before we start
    ilib_mutex_lock(&workQueue->mutex);

    for(int i=0; i < BOSS_PRELOAD_COUNT; i++) {
//      printf("Boss(%d, of %d) -- Loading %d element into boss queue.\n", my_local_rank, my_fiefdom_index, i);
        //assumes that workQueue has enough values to give to all bosses
        bossWorkQueue->enqueue(workQueue->dequeue());
    }

//  int size = workQueue->queue_size();
//  printf("Boss (%d), workQueueSize: %d\n", my_fiefdom_index, size);
    //TODO is the ordering of release correct?      
    ilib_mutex_unlock(&workQueue->mutex);
    ilib_mutex_unlock(&bossWorkQueue->mutex);

    ilib_mem_fence();
    
    return 0;
}

/**********************************/
/*  end of  boss setup workqueue  */
/**********************************/

int broadcast_distributed_workqueue_pointer(void) {

    //broadcast the shared memory address from bossworkQueu to other fiefdom tiles.
    //ignoring status, result objects for brevity.
    if(ilib_msg_broadcast(my_fiefdom, BOSS_RANK, &bossWorkQueue, sizeof(bossWorkQueue), NULL)
            != ILIB_SUCCESS)
        ilib_die("Failed to Broadcast Pixel Pointer.\n");
    

    if(ilib_msg_broadcast(my_fiefdom, BOSS_RANK, &fiefdom_finished, sizeof(fiefdom_finished), NULL)
            != ILIB_SUCCESS)
        ilib_die("Failed to Broadcast fiefdom_finished Pointer.\n");
    return 0;
} 

int broadcast_shared_pointers(void) {

    //broadcast the shared memory address from GLOBAL_MASTER_RANK  to other tiles.
    //ignoring status, result objects for brevity.
    if(ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, GLOBAL_MASTER_RANK, &pixels, sizeof(pixels), NULL)
            != ILIB_SUCCESS)
        ilib_die("Failed to Broadcast Pixel Pointer.\n");

    #if !STATIC_PARTIONING
    //broadcast the workQueue if we are doing self-scheduling
    if(ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, GLOBAL_MASTER_RANK, &workQueue, sizeof(workQueue), NULL)
            != ILIB_SUCCESS)
        ilib_die("Failed to Broadcast Pixel Pointer.\n");
    #endif

    return 0;
}
#endif


int main_test() {

#if HARDWARE_IS_TILE
    ilib_init();
#endif
    std::cout << "RayTracing Test!!!1" << std::endl; 
    
    if(init_log()) {
        return 1;
    }
    
    printf("Write to pixel matrix.\n");
    float r, g, b;
    for(int i=0; i < SCREEN_HORIZONTAL_RESOLUTION; i++) {
        for(int j=0; j < SCREEN_VERTICAL_RESOLUTION; j++) {
            //test writing to a log
            r = ((float) i) / SCREEN_HORIZONTAL_RESOLUTION;
            g = ((float) j) / SCREEN_VERTICAL_RESOLUTION;
            b = 0.5f;
            pixels[i][j] = CelioRayTracer::vector3d(r,g,b);
        }
    }

    printf("PrintScreen to Log.\n");
    printPixelsToLog();
    
    printf("Finished.\n");

#if HARDWARE_IS_TILE
    ilib_finish();
#endif
    return 0;
}
             
/********************************/
/********************************/
/* Distribute Procs to Fiefdoms */
/********************************/
/********************************/
#if HARDWARE_IS_TILE

int distribute_procs_to_fiefdoms() {

    if(CORE_NUM != NUMBER_OF_FIEFDOMS * FIEFDOM_SIZE)
        printf("*** incorrect fiefdom parameters. Proper Core Count:%d\n"
                , NUMBER_OF_FIEFDOMS * FIEFDOM_SIZE);
    //TODO come up with a better way of cluster fiefdoms into 
    //more local groups, instead of lines
    
    fiefdoms = (ilibGroup*) malloc(NUMBER_OF_FIEFDOMS*sizeof(ilibGroup));
    if(!fiefdoms) ilib_die("Malloc Failed.");
    fiefdom_ids = (int **) malloc(NUMBER_OF_FIEFDOMS*sizeof(int *));
    if(!fiefdom_ids) ilib_die("Malloc Failed.");

    for(int i=0; i < NUMBER_OF_FIEFDOMS; i++) {
        fiefdom_ids[i] = (int *) malloc(FIEFDOM_SIZE*sizeof(int));
        if(!fiefdom_ids[i]) ilib_die("Malloc Failed.");
    }

    int temp_id = 0;
    for(int i=0; i < NUMBER_OF_FIEFDOMS; i++) {
        for(int j=0; j < FIEFDOM_SIZE; j++) {
            fiefdom_ids[i][j] = temp_id;
            ++temp_id;
        }
    }

//  if(my_global_rank==GLOBAL_MASTER_RANK) {
//     printf("***FIEFDOM_IDS***\n");
//     for(int i=0; i < NUMBER_OF_FIEFDOMS; i++) {
//      printf("Fiefdom: %d: %d \n", i, fiefdoms[i]);
//      for(int j=0; j < FIEFDOM_SIZE; j++) {
//          printf("   id: %d\n", fiefdom_ids[i][j]);
//      }
//     }
//     }
    
    
    ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
    for(int i=0; i < NUMBER_OF_FIEFDOMS; i++) {
        if(ilib_group_include(ILIB_GROUP_SIBLINGS, FIEFDOM_SIZE, fiefdom_ids[i], &fiefdoms[i])!=ILIB_SUCCESS)
            ilib_die("INCLUSION FAILED");
    }

    my_fiefdom = fiefdoms[my_global_rank/FIEFDOM_SIZE]; //use this to pass around for ilib functions
    my_fiefdom_index = my_global_rank/FIEFDOM_SIZE; //this is an easier thing to keep track of

    
//  if(my_global_rank==GLOBAL_MASTER_RANK) {
//    printf("***FIEFDOM_IDS***\n");
//  printf("GlobalGroup: %d\n", ILIB_GROUP_SIBLINGS);
//  for(int i=0; i < NUMBER_OF_FIEFDOMS; i++) {
//      printf("Fiefdom: %d: %d \n", i, fiefdoms[i]);
//      for(int j=0; j < FIEFDOM_SIZE; j++) {
//          printf("   id: %d\n", fiefdom_ids[i][j]);
//      }
//  }
//  }

    my_local_rank = ilib_group_rank(my_fiefdom);

//  for(int i=0; i < CORE_NUM; i++) {
//      ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
//      if(my_global_rank==i)
//          printf("GRank(%d)\n   FRank(%d, of %d)\n", my_global_rank, my_local_rank, my_fiefdom);  
//  }

    return 0;
}
#endif

/**********************************/
/**********************************/
/* End of Dist. Procs to Fiefdoms */
/**********************************/
/**********************************/


/***************************/
/***** SPAWN PROCESSES *****/
/***************************/

#if HARDWARE_IS_TILE

int spawn_processes() {

    // A structure to be filled with process creation parameters
    ilibProcParam spawn_params;
    
    // We Want to create more processes; leaving the binary
    // file NULL will cause iLib to exec using the current binary
    spawn_params.num_procs = CORE_NUM;
    spawn_params.binary_name = NULL;
    spawn_params.argv = NULL;
    
    //set the Geometry
    spawn_params.tiles.x = SPAWN_X_CORNER; 
    spawn_params.tiles.y = SPAWN_Y_CORNER; 
    spawn_params.tiles.width = SPAWN_WIDTH; 
    spawn_params.tiles.height = SPAWN_HEIGHT; 
    
    //set the initialize parameters block to contain the pixel, workQueue pointers
    InitStruct init_info_struct;
    init_info_struct.sharedmem_pixels_ptr = pixels;
    init_info_struct.sharedmem_workqueue_ptr = workQueue;
    spawn_params.init_block = &init_info_struct; 
    spawn_params.init_size = sizeof(init_info_struct); 

    //Create new processes, kill this one
    //proc_exe(# of params, params[]);
    ilib_proc_exec(1, &spawn_params);

    //if we get here, exec failed
    ilib_die("ilib_proc_exec() failed!");
}
#endif

/***************************/
/* END OF  SPAWN PROCESSES */
/***************************/


int printPixelsToLog(void)
{
    addToLogDouble("Run_Time", run_time_s);
    addToLogDouble("us/pixel", run_time_us / (SCREEN_HORIZONTAL_RESOLUTION * SCREEN_VERTICAL_RESOLUTION));
    addToLogStr("filename", LOG_FILE_NAME);
        
    
    char line_str[80];
    CelioRayTracer::vector3d *temp;
    //float r,g,b;

    printf("Greetings: %d, %d \n"
            , SCREEN_HORIZONTAL_RESOLUTION
            , SCREEN_VERTICAL_RESOLUTION
            );
    for(int i = 0; i < SCREEN_HORIZONTAL_RESOLUTION; i++) {
        for(int j = 0; j < SCREEN_VERTICAL_RESOLUTION; j++) {
            
            //string writing
            temp = &pixels[i][j];
#if USING_FIXED_POINT
            float r, g, b;
            r = fix2float<POINT>(temp->x.intValue);
            g = fix2float<POINT>(temp->y.intValue);
            b = fix2float<POINT>(temp->z.intValue);
            sprintf(line_str, "(%f, %f, %f)\n", r,g,b);
#else
            sprintf(line_str, "(%f, %f, %f)\n", temp->r, temp->g, temp->b);
#endif
            fputs(line_str, log_file);
        
            //TODO write binary file instead, to save lots of space
            //and allow me to make bigger pictures (since java chokes above ~8mb files)
            //binary writing
            //ofstream log_file; //only open for output
            //log_file.open(log_filename);
            //log_file.write((char *) r, sizeof(r));
            //log_file.write((char *) g, sizeof(g));
            //log_file.write((char *) b, sizeof(b));
            //printf("sizeB: %d", sizeof(b));
            
            //  fputc('1', log_file);
        }
    }
    
    //fputc('\n',log_file); //every round gets its own line in the log

    printf("Closing log file.\n\n");
    fclose(log_file);
    //log_file.close();
    
    return 0;
}



#if HARDWARE_IS_TILE
/***************************/
/*** SET UP CHANNEL COMM ***/
/***************************/

int bchan_set_up_master_send_ports() {

    for(int i=0; i < FIEFDOM_SIZE-1; i++) {
        //i+1 is the receiver rank
        if (ilib_bufchan_connect(my_fiefdom
                                , FIEFDOM_MASTER_RANK
                                , CHAN_TAG_FROM_MASTER+i+1
                                , i+1
                                , CHAN_TAG_FROM_MASTER+i+1) < 0) {
            ilib_die("Failed to define channel.");
        }
    }
    
    // Open our send ports.
    for(int i=0; i < FIEFDOM_SIZE-1; i++) {
        if (ilib_bufchan_open_sender(CHAN_TAG_FROM_MASTER+i+1
                    , &port_send_to_serfs[i]) < 0)
            ilib_die("Failed to open SendPort.");
    }

    return 0;
}

int bchan_set_up_serf_send_port() {

    if(ilib_bufchan_connect(my_fiefdom
                            , my_local_rank
                            , CHAN_TAG_FROM_SERF+my_local_rank
                            , FIEFDOM_MASTER_RANK
                            , CHAN_TAG_FROM_SERF+my_local_rank) < 0) {
        ilib_die("Failed to define serf send channel.");
    }

    //open our send port.
    if(ilib_bufchan_open_sender(CHAN_TAG_FROM_SERF+my_local_rank, &port_send_to_master) < 0)
        ilib_die("Failed to open send port.");

    return 0;
}

int bchan_set_up_master_receive_ports(void** master_buffers, size_t master_buffer_size) {
 
    for(int i=0; i < FIEFDOM_SIZE-1; i++) 
        if(ilib_bufchan_open_receiver(CHAN_TAG_FROM_SERF+i+1, master_buffers[i], master_buffer_size, &port_receive_from_serfs[i]) < 0) {
            ilib_die("Failed to open receiver port.");
        }
    return 0;
}

int bchan_set_up_serf_receive_port(void* serf_buffer, size_t serf_buffer_size) {
 
//  for(int i=0; i < FIEFDOM_SIZE-1; i++) 
        if(ilib_bufchan_open_receiver(CHAN_TAG_FROM_MASTER+my_local_rank, serf_buffer, serf_buffer_size, &port_receive_from_master)) {
            ilib_die("Failed to open receiver port.");
        }
    return 0;
}

int bchan_set_up_parallel_collision_communications() {

    //TODO check all mallocs
    if(my_local_rank==FIEFDOM_MASTER_RANK) {
        //set up master communications
        int num_packets = 2;
        int packet_size = sizeof(CollisionObject);
        size_t master_buffer_size = ilib_bufchan_calc_buffer_size(num_packets, packet_size);
        void ** master_buffers;

        master_buffers = (void **) malloc(FIEFDOM_SIZE * master_buffer_size);

        for(int i=0; i < FIEFDOM_SIZE-1; i++) {
            master_buffers[i] = (void *) malloc(master_buffer_size);
        }
    
        port_send_to_serfs = 
            (ilibBufChanSendPort *) malloc((FIEFDOM_SIZE-1)*sizeof(ilibBufChanSendPort));
        port_receive_from_serfs = 
            (ilibBufChanReceivePort *) malloc((FIEFDOM_SIZE-1)*sizeof(ilibBufChanReceivePort));
        
        bchan_set_up_master_send_ports();
        bchan_set_up_master_receive_ports(master_buffers, master_buffer_size);

    } else {
        //set up serf communications
        int num_packets = NUM_PACKETS;
        int packet_size = sizeof(Ray);
        size_t serf_buffer_size = ilib_bufchan_calc_buffer_size(num_packets, packet_size);
        void* serf_buffer = malloc(serf_buffer_size);

        bchan_set_up_serf_receive_port(serf_buffer, serf_buffer_size);
        bchan_set_up_serf_send_port();
    }
                    
    return 0;
}

/****************************/
/**** RAW CHANNEL SUPPORT ***/
/****************************/
//TODO make this its own file

int rchan_set_up_parallel_collision_communications() {

    printf("  Raw Channel Set Up: Master Command Channel id(#%d)\n", my_local_rank);
    rchan_set_up_master_command_channel();
    printf("  Raw Channel Set Up: Serf Data Channel id(#%d)\n", my_local_rank);
    rchan_set_up_serf_data_channel();
    return 0;
}

int rchan_set_up_master_command_channel() {


    if(my_local_rank==FIEFDOM_MASTER_RANK) {
        // Open our send ports.
        
        for(int i=0; i < FIEFDOM_SIZE-1; i++) {
            //i+1 is the receiver rank
        
            if (ilib_rawchan_connect(my_fiefdom
                                , FIEFDOM_MASTER_RANK
                                , CHANNEL_MASTER_COMMAND_TAG+i+1
                                , i+1
                                , CHANNEL_MASTER_COMMAND_TAG+i+1) < 0) {
                ilib_die("Failed to define channel.");
            }
        }
        

        master_send_ports = (ilibRawSendPort *) malloc((FIEFDOM_SIZE-1)*sizeof(ilibRawSendPort));

#if (FIEFDOM_SIZE) >= 2
      master_send_ports[0] = port_master_send1;
#endif
#if (FIEFDOM_SIZE) >=3
      master_send_ports[1] = port_master_send2;
#endif
#if (FIEFDOM_SIZE) >=4
      master_send_ports[2] = port_master_send3;
#endif
#if (FIEFDOM_SIZE) >=5
      master_send_ports[3] = port_master_send4;
#endif
#if (FIEFDOM_SIZE) >=6
      master_send_ports[4] = port_master_send5;
#endif
#if (FIEFDOM_SIZE) >=7
      master_send_ports[5] = port_master_send6;
#endif
#if (FIEFDOM_SIZE) >=8
      master_send_ports[6] = port_master_send7;
#endif
#if (FIEFDOM_SIZE) >=9
      master_send_ports[7] = port_master_send8;
#endif
#if (FIEFDOM_SIZE) >=10
      master_send_ports[8] = port_master_send9;
#endif
#if (FIEFDOM_SIZE) >=11
      master_send_ports[9] = port_master_send10;
#endif
#if (FIEFDOM_SIZE) >=12
      master_send_ports[10] = port_master_send11;
#endif
#if (FIEFDOM_SIZE) >=13
      master_send_ports[11] = port_master_send12;
#endif
#if (FIEFDOM_SIZE) >=14
      master_send_ports[12] = port_master_send13;
#endif
#if (FIEFDOM_SIZE) >=15
      master_send_ports[13] = port_master_send14;
#endif
#if (FIEFDOM_SIZE) >=16
      master_send_ports[14] = port_master_send15;
#endif
#if (FIEFDOM_SIZE) >=17
      master_send_ports[15] = port_master_send16;
#endif
#if (FIEFDOM_SIZE) >=18
      master_send_ports[16] = port_master_send17;
#endif
#if (FIEFDOM_SIZE) >=19
      master_send_ports[17] = port_master_send18;
#endif
#if (FIEFDOM_SIZE) >=20
      master_send_ports[18] = port_master_send19;
#endif
#if (FIEFDOM_SIZE) >=21
      master_send_ports[19] = port_master_send20;
#endif
#if (FIEFDOM_SIZE) >=22
      master_send_ports[20] = port_master_send21;
#endif
#if (FIEFDOM_SIZE) >=23
      master_send_ports[21] = port_master_send22;
#endif
#if (FIEFDOM_SIZE) >=24
      master_send_ports[22] = port_master_send23;
#endif
#if (FIEFDOM_SIZE) >=25
      master_send_ports[23] = port_master_send24;
#endif
#if (FIEFDOM_SIZE) >=26
      master_send_ports[24] = port_master_send25;
#endif
#if (FIEFDOM_SIZE) >=27
      master_send_ports[25] = port_master_send26;
#endif
#if (FIEFDOM_SIZE) >=28
      master_send_ports[26] = port_master_send27;
#endif
#if (FIEFDOM_SIZE) >=29
      master_send_ports[27] = port_master_send28;
#endif
#if (FIEFDOM_SIZE) >=30
      master_send_ports[28] = port_master_send29;
#endif
#if (FIEFDOM_SIZE) >=31
      master_send_ports[29] = port_master_send30;
#endif
#if (FIEFDOM_SIZE) >=32
      master_send_ports[30] = port_master_send31;
#endif
#if (FIEFDOM_SIZE) >=33
      master_send_ports[31] = port_master_send32;
#endif
#if (FIEFDOM_SIZE) >=34
      master_send_ports[32] = port_master_send33;
#endif
#if (FIEFDOM_SIZE) >=35
      master_send_ports[33] = port_master_send34;
#endif
#if (FIEFDOM_SIZE) >=36
      master_send_ports[34] = port_master_send35;
#endif
#if (FIEFDOM_SIZE) >=37
      master_send_ports[35] = port_master_send36;
#endif
#if (FIEFDOM_SIZE) >=38
      master_send_ports[36] = port_master_send37;
#endif
#if (FIEFDOM_SIZE) >=39
      master_send_ports[37] = port_master_send38;
#endif
#if (FIEFDOM_SIZE) >=40
      master_send_ports[38] = port_master_send39;
#endif
#if (FIEFDOM_SIZE) >=41
      master_send_ports[39] = port_master_send40;
#endif
#if (FIEFDOM_SIZE) >=42
      master_send_ports[40] = port_master_send41;
#endif
#if (FIEFDOM_SIZE) >=43
      master_send_ports[41] = port_master_send42;
#endif
#if (FIEFDOM_SIZE) >=44
      master_send_ports[42] = port_master_send43;
#endif
#if (FIEFDOM_SIZE) >=45
      master_send_ports[43] = port_master_send44;
#endif
#if (FIEFDOM_SIZE) >=46
      master_send_ports[44] = port_master_send45;
#endif
#if (FIEFDOM_SIZE) >=47
      master_send_ports[45] = port_master_send46;
#endif
#if (FIEFDOM_SIZE) >=48
      master_send_ports[46] = port_master_send47;
#endif
#if (FIEFDOM_SIZE) >=49
      master_send_ports[47] = port_master_send48;
#endif
#if (FIEFDOM_SIZE) >=50
      master_send_ports[48] = port_master_send49;
#endif
#if (FIEFDOM_SIZE) >=51
      master_send_ports[49] = port_master_send50;
#endif
#if (FIEFDOM_SIZE) >=52
      master_send_ports[50] = port_master_send51;
#endif
#if (FIEFDOM_SIZE) >=53
      master_send_ports[51] = port_master_send52;
#endif
#if (FIEFDOM_SIZE) >=54
      master_send_ports[52] = port_master_send53;
#endif
#if (FIEFDOM_SIZE) >=55
      master_send_ports[53] = port_master_send54;
#endif
#if (FIEFDOM_SIZE) >=56
      master_send_ports[54] = port_master_send55;
#endif
#if (FIEFDOM_SIZE) >=57
      master_send_ports[55] = port_master_send56;
#endif

        for(int i=0; i < FIEFDOM_SIZE -1; i++) {

            if (ilib_rawchan_open_sender(CHANNEL_MASTER_COMMAND_TAG+i+1
                , master_send_ports[i]) < 0)
                ilib_die("Failed to open SendPort.");
            printf("  Boss: Setting up Serf Channel #2\n");

        }

        /*
        printf("  Boss: Setting up Serf Channel #1\n");
        if (ilib_rawchan_open_sender(CHANNEL_MASTER_COMMAND_TAG+1
            , port_master_send1) < 0)
            ilib_die("Failed to open SendPort.");
        printf("  Boss: Setting up Serf Channel #2\n");
        if (ilib_rawchan_open_sender(CHANNEL_MASTER_COMMAND_TAG+2
            , port_master_send2) < 0)
            ilib_die("Failed to open SendPort.");
//      printf("  Boss: Setting up Serf Channel #3\n");
//      if (ilib_rawchan_open_sender(CHANNEL_MASTER_COMMAND_TAG+3
//          , port_master_send3) < 0)
//          ilib_die("Failed to open SendPort.");
*/  
    } else {
        //is a serf tile

        printf("  Serf(%d): Opening Receiver Channel\n", my_local_rank);
        
        if(ilib_rawchan_open_receiver(CHANNEL_MASTER_COMMAND_TAG+my_local_rank, port_serf_receive) < 0)
            ilib_die("Failed to open ReceivePort.");
    }
        
    return 0;
}


/* Set up the Serf Data Channel
 * consists of Master Sink
 * and 3 serf senders 
 * Note: this is only run by
 * the master tile */
int rchan_set_up_serf_data_channel() 
{
    ilibSink sink;

    if(my_local_rank==FIEFDOM_MASTER_RANK) {
        
        if (ilib_rawchan_start_sink(my_fiefdom  
                                , FIEFDOM_MASTER_RANK
                                , CHANNEL_SERF_DATA_TAG
                                , &sink) != ILIB_SUCCESS)
            ilib_die("Failed to start sink.");

        //add the serfs as senders for the sink
        for(int i=1; i < FIEFDOM_SIZE; i++) {
            printf("   Adding sink sender: %d\n", i);
            if (ilib_rawchan_add_sink_sender(my_fiefdom
                                    , i
//                                  , CHANNEL_SERF_DATA_TAG+i+1
                                    , CHANNEL_SERF_DATA_TAG
                                    , &sink) != ILIB_SUCCESS)
            ilib_die("Failed to add sink sender.");
        }
   
        if (ilib_rawchan_finish_sink(&sink) != ILIB_SUCCESS)
            ilib_die("Failed to finish sink.");

        /* Connect master as receiver to sink */
        if (ilib_rawchan_open_receiver(CHANNEL_SERF_DATA_TAG, port_sink) < 0)
            ilib_die("Failed to open ReceivePort.");
    
    } else {
        //else is a serf

        /* open serf's sender port  to sink */
//      if (ilib_rawchan_open_sender(CHANNEL_SERF_DATA_TAG + my_local_rank, port_serf_send) < 0)
        if (ilib_rawchan_open_sender(CHANNEL_SERF_DATA_TAG, port_serf_send) < 0)
            ilib_die("Failed to open SendPort.");

    }
    return 0;
}
#endif //tile is harware

/**************************/
/**** LOG FILE FUNCTION ***/
/**************************/
int init_log(void)
{
    printf("Opening log file...\n");
    log_file = fopen(LOG_FILE_NAME, log_file_mode);
    
    if(log_file == NULL) {
        //file error opening
        printf("Error Opening File %s\n", LOG_FILE_NAME);
        return 1; //error occured
    }

    fputs(LOG_FILE_TITLE, log_file);
    addToLogInt("Horizontal_Resolution", SCREEN_HORIZONTAL_RESOLUTION);
    addToLogInt("Vertical_Resolution", SCREEN_VERTICAL_RESOLUTION);
    addToLogStr("Hardware_Target", HARDWARE_TARGET);
    addToLogInt("Number_of_Cores", CORE_NUM);
    
    if(IS_FOR_SIMULATION)         
        fputs("IS_FOR_SIMULATION\n", log_file);
    else
        fputs("IS_FOR_HARDWARE\n", log_file);
    
    if(PARTIONING_STRATEGY==0)
        fputs("NO_PARTIONING\n", log_file);
    else if(PARTIONING_STRATEGY==1)
        fputs("DUMB_STATIC_PARTIONING\n", log_file);
    else if(PARTIONING_STRATEGY==2)
        fputs("SELF_SCHEDULING\n", log_file);
    else if(PARTIONING_STRATEGY==3)
        fputs("DISTRIBUTED_WORKQUEUE\n", log_file);
    else if(PARTIONING_STRATEGY==4)
        fputs("SIMD BUFFER COLLISION PARALLIZATION\n", log_file);
    else if(PARTIONING_STRATEGY==5)
        fputs("SMART_STATIC_PARTIONING\n", log_file);
    else if(PARTIONING_STRATEGY==7)
        fputs("SIMD RAW COLLISION PARTIONING\n", log_file);
    else fputs("CHRIS NEEDS TO ADD MORE CODE\n", log_file);
    
    return 0;
}

/**
 * TAGGING:
 * The Java client scans for ':' and '.'
 * for picking out the numerical values.
 * 
 * example:....>  Field_width:100.
 */
int addToLogInt(char *tag, int value) 
{   
    char value_str[7];
    char line[30];
    sprintf(value_str, "%i", value); //convert int to string
    strcpy(line, tag);
    strcat(line, ":");
    strcat(line, value_str);
    strcat(line, ".\n");
    
    fputs(line, log_file);
    return 0;
}
int addToLogDouble(char *tag, double value) 
{   
    char value_str[20];
    char line[80];
    sprintf(value_str, "%f", value); //convert int to string
    printf("value (to log): %s\n", value_str);
    strcpy(line, tag);
    strcat(line, ":");
    strcat(line, value_str);
    strcat(line, ".\n");
    
    fputs(line, log_file);
    return 0;
}

int addToLogStr(char *tag, char *value_str)
{
    printf("LogStr\n");
    char line[30+sizeof(tag) + sizeof(value_str)+3]; //BUG these sizeof's are not being calculated correctly
    //printf("Line: %d, Tag: %d, VStr: %d\n", sizeof(line), sizeof(tag), sizeof(value_str));
    strcpy(line, tag);
    strcat(line, ":");
    strcat(line, value_str);
    strcat(line, ".\n");
    fputs(line, log_file);

    return 0;
}
