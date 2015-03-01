#include "SceneSphere.h"
#include "SceneObject.h"
#include <math.h>
#include "stdio.h"
#include "vector3d.h"

    /*
     * http://www.cs.unc.edu/~rademach/xroads-RT/RTarticle.html
     * this collision code borrows off of the above tutorial.
     * i have done my best to explain the math below, but 
     * the above website has a better, graphical explanation.
     * 
     * |x - sphere.origin|^2 = r^2
     * 
     * x = ray.origin + dist*ray.dir        
     * 
     * dist is a scalar magnitude between the eye.origin 
     * 
     * EO is the vector from Eye to Origin (sphere).
     * eo is the magnitude (it is also a hypotensus)
     * 
     * V is the Ray vector
     * v is the magnitude from eye to the normal of the origin (inside the sphere).
     * 
     * b is magnitude of the vector that is normal to V, and goes through the origin
     * 
     * (eo, v, and b make a right triangle).
     * 
     * (r, b, and d is a right triangle inside the sphere).
     * 
     * if d is positive (d^2 is not imaginary), then an intersection occurred.
     */


namespace CelioRayTracer
{

SceneSphere::SceneSphere() : SceneObject()
{
    radius = 1.0f;
    radius_squared = radius*radius;
}

SceneSphere::SceneSphere(vector3d _origin, sdecimal32 _radius) : SceneObject(_origin)
{   
    radius = _radius;
    radius_squared = _radius*_radius;
}

SceneObject::CollisionObject* SceneSphere::collision(Ray* eyeRay)
{   
    //OE = Sphere.Origin - eyeRay.Origin
    vector3d OE;
    OE = origin -  eyeRay->getOrigin();
    
    sdecimal32 v = OE.dot(eyeRay->getDirection());  //real part of root
    //what if v is negative?????
    if(v < (sdecimal32) 0)
        return NULL;
    //i don't think v should ever be negative....
    
    //double c = EO.dot(EO) //ie, magnitude.
    
    //d^2 = r^2 - b^2
    // b^2 = c^2 - 
    sdecimal32 d_squared = radius_squared - ( (sdecimal32) OE.dot(OE) - v*v);
    
    if(d_squared < (sdecimal32) 1E-9) { //ie, < 0 is imaginary, close to zero means it might be intersecting itself :(
        //dist is imaginary, therefore, no intersection
        return NULL;
    } else {
        //d = d_squared
        //P = E + (v-d) * V
    //  if(Math.sqrt(d_squared) < 1E-10)
    //      return null;
        
        //now get both roots
#if USING_FIXED_POINT
        sdecimal32 root1 = v - fixed_sqrt(d_squared); //the distance to the intersection
        sdecimal32 root2 = v + fixed_sqrt(d_squared);
        

        float root2_float = fix2float<16>(v.intValue) + sqrt(fix2float<16>(d_squared.intValue));
#else
        sdecimal32 root1 = v - sqrt(d_squared); //the distance to the intersection
        sdecimal32 root2 = v + sqrt(d_squared);
#endif
        
        bool insideHit = false;
        sdecimal32 distance = 65535.0f; //1e09; //this may be changed to something more interesting
        
        //OPTIMIZE out the else statements that we should never get into
        if(root2 > (sdecimal32) 0) {//supposedly if root2 is negative, no hit occurred?
            if(root1 < (sdecimal32) 0) {
                if(root2 < distance) {
                    //inside hit
                    distance = root2;
                    insideHit = true;
                }else {
                    printf("root2>dist Distance: %d, FAILURE distance schenesphere. WE SHULD NEVER BE HERE????\n", root2);
                    return NULL; //??????
                }
            } else {
                if(root1 < distance) {
                    distance = root1;
                    insideHit = false;
                } else {
                    printf("root1 > dist root1: %f > distance: %f FAILURE distance schenesphere.\n"
                            ,  fix2float<16>(root1.intValue), distance);
                    return NULL; //??????
                }
            }
        }else {
            printf("FAILURE Negative ROot 2: %f , RootFloat: %f\n", fix2float<16>(root2.intValue), root2_float);
            return NULL; //??????
        }
    
#if USING_FIXED_POINT
        vector3d intersection_point = (v-fixed_sqrt(d_squared))*eyeRay->getDirection() 
                                    + eyeRay->getOrigin();
#else
        vector3d intersection_point = (v-sqrt(d_squared))*eyeRay->getDirection() 
                                    + eyeRay->getOrigin();
#endif
            
        //normal_ray = P - O
        
        vector3d normal_ray; //TODO make this work with pointers
        normal_ray = intersection_point - origin;
        normal_ray.normalize();
        
        SceneObject::CollisionObject* returnStruct= 
                                new SceneObject::CollisionObject(
                                      intersection_point
                                    , normal_ray
                                    , eyeRay->getDirection()
                                    #if USING_FIXED_POINT
                                    , (v-fixed_sqrt(d_squared)) //distance
                                    #else
                                    , (v-sqrt(d_squared))   //distance
                                    #endif
                                    , myMaterial.getColor()
                                    , insideHit //inside hit
                                    , this);
        
/*      SceneObject::CollisionObject* returnStruct= 
                                new SceneObject::CollisionObject(
                                      intersection_point
                                    , normal_ray
                                    , eyeRay->getDirection()
#if USING_FIXED_POINT
                                    , (v-fixed_sqrt(d_squared)) //distance
#else
                                    , (v-sqrt(d_squared))   //distance
#endif
                                    , myMaterial.getColor()
                                    , myMaterial.getReflectiveFactor()
                                    , myMaterial.getRefractiveFactor() 
//                                  , this
                                    , my_object_index
                                    , insideHit); //inside hit
*/
        if(isaLightSource)
            returnStruct->setHitALightSource(true);
        
        return returnStruct;
    }
}

SceneSphere::~SceneSphere()
{
}

} //end of name-space
