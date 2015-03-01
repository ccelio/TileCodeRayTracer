#include "SceneFinitePlane.h"
#include "vector3d.h"
#include "SceneObject.h"

/**
 * @author CCelio
 *
 */

namespace CelioRayTracer
{

SceneFinitePlane::SceneFinitePlane() : SceneObject()
{
}


SceneFinitePlane::SceneFinitePlane(vector3d _origin, vector3d _normal, vector3d _horizontal
                , float v_dist, float h_dist) : SceneObject(_origin) {
    
    plane_origin = _origin;
    normal = _normal;
    horizontal = _horizontal;
    vertical.cross(normal, horizontal);
    
    normal.normalize();
    horizontal.normalize();
    vertical.normalize();
    
    reverseNormal = -normal;
    reverseNormal.normalize();
    
    v_distance = v_dist;
    h_distance = h_dist;
    //optimization, since plane never moves, we can calculate this statically
    distance_to_origin = - _origin.dot(normal); 
    
    /*
    Vector3d new_super_origin = new Vector3d();
    Vector3d h = new Vector3d();
    new_super_origin.scaleAdd(v_distance/2.0, vertical, plane_origin);
    h.scaleAdd(h_distance/2.0,horizontal);
    new_super_origin.add(h);
    super.changeOrigin(new_super_origin);
    */
    
    
}
SceneFinitePlane::SceneFinitePlane(vector3d _origin
                                    , vector3d _vertical_corner
                                    , vector3d _horizontal_corner)
                                    : SceneObject(_origin) {
    /* NOTE: Make sure origin is the corner between the other two. ie, 
     * the "origin" should be on the connecting edges between the vertical 
     * and horizontal corners.
     */
    plane_origin = _origin;
    horizontal = _horizontal_corner - _origin;
    vertical = _vertical_corner - _origin;
    
    normal.cross(horizontal, vertical);
    
    v_distance = vertical.length();
    h_distance = horizontal.length();
    
    vertical.normalize();
    horizontal.normalize();
    normal.normalize();
    reverseNormal= -normal;
    reverseNormal.normalize();
    
    distance_to_origin = - _origin.dot(normal); 

    vector3d new_super_origin;
    vector3d h;
    h = h_distance*horizontal;
    new_super_origin = v_distance*vertical + plane_origin + h;
    
    this->changeOrigin(new_super_origin);
}

SceneFinitePlane::~SceneFinitePlane()
{
}

SceneObject::CollisionObject* SceneFinitePlane::collision(Ray* eyeRay) {
    //equation of the ray, intersecting plane at point P = E + t*D
    //equation of a plane is P*N = -d, d is the distant from absolute origin to the normal's origin(?)
    //solve for scalar t, 
    //t = (-d - E*N) / (N*D)  must verify both numerator and denominator aren't zero, and t is positive (i think).

    sdecimal32 numerator = -distance_to_origin - eyeRay->getOrigin().dot(normal);
    sdecimal32 denom = eyeRay->getDirection().dot(normal);
    
    //if(numerator==0 || denom==0) //probably never occurs?
    if(denom== 0)
        return NULL; //no intersection
    
    sdecimal32 t = numerator / denom; 
    
    //if(t < 0 || t < 1E-10) //b/c sometimes rays intersected with their origin...(for reflected rays)
    if(t < 1E-5) //1E-10) //or greater than some max value?
        return NULL;
    
    //a collision has occurred with the infinite plane.... now let's test to see if its within the bounds.
    //P = E + t*D
    vector3d intersection_point;
    intersection_point = t*eyeRay->getDirection() + eyeRay->getOrigin();
    
    //must calculate offset, etc.
    //r is distance from plane.origin to point P (intersection)
    //x is horizontal distance from origin to P
    //y is vertical distance from origin to P
    
    //first construct vector PO
    vector3d PO;
    PO = intersection_point - plane_origin;
    
    //double r = PO.length();
    sdecimal32 x = PO.dot(horizontal);
    sdecimal32 y = PO.dot(vertical); 
    
    if(x< 0 || x > h_distance || y < 0 || y > v_distance)
        return NULL;
    
    //a collision has occurred with Finite Plane
    
    Color c = myMaterial.getColor();
    
    if(myMaterial.getTexture()!=NULL) {
        c = myMaterial.getTexture()->getTexturePixel(x, y);
    }
    
    vector3d temp_normal = computeNormal(eyeRay->getDirection());
    vector3d new_intersection_point = intersection_point
                                        + temp_normal*INTERSECTION_OFFSET_DIST;

    CollisionObject *returnStruct =  new CollisionObject(new_intersection_point
                                            , temp_normal
                                            , eyeRay->getDirection()
                                            , t //return the distance to the intersection point
                                            , c //color
                                            , false //never have an "inside hit" with a plane
                                            , this);

/*  CollisionObject *returnStruct =  new CollisionObject(new_intersection_point
                                            , temp_normal
                                            , eyeRay->getDirection()
                                            , t //return the distance to the intersection point
                                            , c //return the color
                                            , myMaterial.getReflectiveFactor()
                                            , myMaterial.getRefractiveFactor()
//                                          , this
                                            , my_object_index
                                            , false); //never have an "inside hit" with a plane

*/                                      

    if(isaLightSource)
        returnStruct->setHitALightSource(true);
    
    return returnStruct;
    
}

//BUG why wouldn't I have a static triangle?
vector3d SceneFinitePlane::computeNormal(vector3d eyeDir) {
     
     if(normal.dot(eyeDir) < 0) {
         return normal;
     } else {
         return reverseNormal;
     }
}
        
} //namespace
