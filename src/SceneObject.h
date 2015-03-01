
#ifndef SCENEOBJECT_H_
#define SCENEOBJECT_H_

/**
 * @author CCelio
 * 
 */

#include "fixed_class.h"
#include "vector3d.h"
#include "ObjMaterial.h"
#include "Ray.h"
#include "stdio.h"

#if USING_FIXED_POINT
    typedef fixed_point<16> sdecimal32;
#else
    typedef float sdecimal32;
#endif
    

namespace CelioRayTracer {
//TODO bloated class. do i really have to declare collision obj in here?

class SceneObject
{
    
public:
    /**************** COLLISION OBJECT *****************
     * contains all of the information relevant
     * to a collision. 
     ***************************************************/
    
    /* TODO C++ has bad overhead in returning on object from a function */
    class CollisionObject
    {
    public:
        CollisionObject()
        {
            //distance = 0;
            //color = vector3d();
        };
        
 //     CollisionObject(sdecimal32 dist, Color c) : distance(dist), color(c) {};

        CollisionObject(vector3d point
                        , vector3d normal
                        , vector3d incoming_ray
                        , sdecimal32 dist
                        , Color c
                        , bool _insideHit
                        , SceneObject* scene_obj_ptr) {
//      CollisionObject(vector3d point, vector3d normal, vector3d incoming_ray, sdecimal32 dist
//                      , Color c, sdecimal32 reflectiveness, sdecimal32 transparency
//                      , SceneObject* _scene_object_ptr, bool _insideHit) {
//                      , int _scene_object_index, bool _insideHit) {

//          scene_object_ptr = _scene_object_ptr;
//          scene_object_index = _scene_object_index;
            
            normal_ray = Ray(point, normal);
            intersection_point = point;
            insideHit_Var = _insideHit;
            sdecimal32 n_dot_incoming = normal.dot(incoming_ray);
            distance = dist;
            color = c;

            ObjMaterial* material_ptr = scene_obj_ptr->getMaterial();
            
            diffuse_factor = material_ptr->getDiffuseFactor();
            specular_factor = material_ptr->getSpecularFactor();
            intensity_factor = scene_obj_ptr->getIntensity();
            reflective_factor = material_ptr->getReflectiveFactor();
            reflective_material = (reflective_factor > (sdecimal32) 0) ? true : false;
//          reflective_material = (reflectiveness > 0) ? true : false;
//          transparent_material = (transparency > 0) ? true : false;
//          transparent_material = (tscene_obj_ptr->getMaterial()->getTransparency() >  0) ? true : false;
//          transparent_factor = transparency;
            
            if(reflective_material) {
                //TODO should i use operator overload to do this
                vector3d reflected = vector3d(-2*normal.x * n_dot_incoming + incoming_ray.x,
                                                -2*normal.y * n_dot_incoming + incoming_ray.y,
                                                -2*normal.z * n_dot_incoming + incoming_ray.z);
            
                reflected_ray = Ray(point, reflected);
//              reflective_factor = scene_obj_ptr->getMaterial()->getReflectiveFactor();
             
        }
        
//          if(scene_obj_ptr->getMaterial()->getReflectiveFactor()
//              !=reflective_factor) {
//              printf("**** ERROR : %f vs. Actual %f \n"
//                          , reflective_factor
//                          , scene_obj_ptr->getMaterial()->getReflectiveFactor());
//          }

//          if(reflective_factor < 0.40 && reflective_material) {
//              printf("***** ERROR, ReflMaterial:%d , Refl:Factor: %f\n"
//                      , reflective_material
//                      , reflective_factor
//                   );
//          }
        };
        
//      SceneObject* getCollisionObject() {return scene_object_ptr; };
        int getCollisionObjectIndex() {return scene_object_index; };
        //TODO return only the pointers
        //actually, pointers are dangerous if we pass the collisioObject
        //between cores
        Ray inline getNormalRay() {return normal_ray; };
        Ray inline getReflectedRay() {  return reflected_ray; };
        sdecimal32 inline getDistance() { return distance;  };
//      Ray inline getRefractedRay() { return refracted_ray; };
        Color inline getColor() { return color; };
        bool inline isReflective() {return reflective_material; };
        vector3d inline getIntersectionPoint() {return intersection_point; };
//      bool inline getTranparent() { return transparent_material; };
//      sdecimal32 inline getTransparentFactor() {  return transparent_factor; };
        sdecimal32 inline getReflectiveFactor() {return reflective_factor; };
        sdecimal32 inline getSpecularFactor() {return specular_factor; };
        sdecimal32 inline getDiffuseFactor() {return diffuse_factor; };
        sdecimal32 inline getIntensityFactor() {return intensity_factor; };
        void inline setHitALightSource(bool x) { hitALightSource_Var = x; };
        bool inline hitALightSource() { return hitALightSource_Var; };
        bool insideHit() { return insideHit_Var;};
        virtual ~CollisionObject(){};
    protected:
        sdecimal32 distance; //magnitude between the ray's origin and the intersection_point
        Color color;    //color of the object we have collided with

        int scene_object_index; //unused
        
        //TODO make these guys pointers?
        Ray normal_ray; //this is the ray that is normal to the object the "eyeray" intersected
        Ray reflected_ray; //this is the ray that bounces 90degrees off of the intersected object
 //     Ray refracted_ray; //this is the ray that bounces 90degrees off of the intersected object

        vector3d intersection_point; //this is the location of the intersection, or collision, between the 
                                    //ray and the scene_object
        
        bool reflective_material;
        sdecimal32 reflective_factor;
        
        sdecimal32 specular_factor;
        sdecimal32 diffuse_factor;   
        sdecimal32 intensity_factor;
        
        bool hitALightSource_Var;
        bool insideHit_Var; //needed for transparency calculations.
    };
    
    /**************** END OF COLLISION OBJECT *****************/

    SceneObject();
    SceneObject(vector3d _o);

    /**
     * figure out if a collision occurs
     */
    /*virtual CollisionObject* collision(Ray* eyeRay) {
        printf("Congragulations. Collision in SceneObject.\n"); // pure virtual/abstract method
        return NULL;
    }*/
    virtual CollisionObject* collision(Ray* eyeRay) = 0;
    
    ObjMaterial* getMaterial() {return &myMaterial;}
    void moveOrigin(sdecimal32 dx, sdecimal32 dy, sdecimal32 dz) { 
        origin.x +=dx;
        origin.y +=dy;
        origin.z +=dz;
    }
    void changeOrigin(vector3d o) {origin = o;}
    vector3d inline getOrigin() {return origin;}

    /*deprecated */
    void inline setIndex(int _index) { my_object_index = _index;}
    int inline getIndex() {return my_object_index;}

    /** If a Light Source **/
    void inline setAsLightSource() {isaLightSource = true;}
    bool inline checkIsaLightSource() {return isaLightSource;}
    void inline setIntensity(sdecimal32 d) {intensity = d;}
    sdecimal32 inline getIntensity() { return intensity;}
    
    virtual ~SceneObject();
    
protected:
    /*Object's Properties*/
    ObjMaterial myMaterial;
    vector3d origin;
    bool isaLightSource;
    sdecimal32 intensity; //only used if a light source
    int my_object_index; //Scene->getObject(index) points to this object
                //this is used so that in passing messages involving
                //scene object references we don't pass the object's
                //actual memory pointer, which means we have to muck
                //around with shared memory
};

}; //end of namespace
#endif /*SCENEOBJECT_H_*/

