#include "SceneInfinitePlane.h"
#include "vector3d.h"

namespace CelioRayTracer
{

SceneInfinitePlane::SceneInfinitePlane() : SceneObject()
{
}

SceneInfinitePlane::SceneInfinitePlane(vector3d _origin
                                        , vector3d _normal
                                        , vector3d _horizontal)
                                        : SceneObject(_origin) {
    normal = _normal;
    horizontal = _horizontal;
    normal.normalize();
    horizontal.normalize();

    vertical.cross(normal, horizontal);
    vertical.normalize();

    reverseNormal = -normal;

    distance_to_origin = -origin.dot(normal);
}


SceneObject::CollisionObject* SceneInfinitePlane::collision(Ray* eyeRay) {
    
    //equation of the ray, intersecting plane at point P = E + t*D
    
    //equation of a plane is P*N = -d, d is the distant from absolute origin to the normal's origin(?)
    
    //solve for scalar t, 
    
    //t = (-d - E*N) / (N*D)  must verify both numerator and denominator aren't zero, and t is positive (i think).
    
    sdecimal32 numerator = -distance_to_origin - (sdecimal32) eyeRay->getOrigin().dot(normal);
    //bug may be computing this backwards?
    sdecimal32 denom = eyeRay->getDirection().dot(normal);
    
    if(denom== (sdecimal32) 0)
        return NULL; //no intersection
    
    sdecimal32 t = numerator / denom; 
    
    //if this is set to low (> 1E-7) the shading algorithm messes up because
    //because it thinks it is hitting itself.
    if(t < (sdecimal32) 1E-10)  
        return NULL;
    
    //else a collision has occurred.
    //P = E + t*D
    vector3d intersection_point = t * eyeRay->getDirection() + eyeRay->getOrigin();
    
    Color c = myMaterial.getColor();
    
    if(myMaterial.getTexture()!=NULL) {
        
        //must calculate offset, etc.
        //r is distance from plane.origin to point P (intersection)
        //x is horizontal distance from origin to P
        //y is vertical distance from origin to P
        
        //first construct vector PO
        vector3d PO;
        PO = intersection_point - origin;
        
        //fixed r = PO.length();
        sdecimal32 x = PO.dot(horizontal);
        sdecimal32 y = PO.dot(vertical);
        
        c = myMaterial.getTexture()->getTexturePixel(x, y);
    } 
    
    //fix "hitting self" bug by off-setting the intersection 
    //away from the surface
    vector3d temp_normal = computeNormal(eyeRay->getDirection());
    vector3d new_intersection_point = intersection_point
                                        + temp_normal*INTERSECTION_OFFSET_DIST;
    
    CollisionObject *returnStruct =  new CollisionObject(new_intersection_point
                                            , temp_normal
                                            , eyeRay->getDirection()
                                            , t //return the distance to the intersection point
                                            , c //return the color
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

vector3d SceneInfinitePlane::computeNormal(vector3d eyeDir) {
     
     if(normal.dot(eyeDir) < 0) {
         return normal;
     } else {
         return reverseNormal;
     }
}
        
SceneInfinitePlane::~SceneInfinitePlane()
{
}

}
