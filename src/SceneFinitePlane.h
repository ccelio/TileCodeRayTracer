#ifndef SCENEFINITEPLANE_H_
#define SCENEFINITEPLANE_H_

#include "SceneObject.h"

//i have a serious problem with points on a surface are
//found to "collide" with the surface, screwing up
//shading calculations, and causing black dots to be everywhere
//so i fix this by offsetting the intersection point
//from the surface
#define INTERSECTION_OFFSET_DIST 1E-3

namespace CelioRayTracer
{

class SceneFinitePlane : public SceneObject
{
public:
    SceneFinitePlane();
    SceneFinitePlane(vector3d _origin
                    , vector3d _vertical_corner
                    , vector3d _horizontal_corner); 
    SceneFinitePlane(vector3d _origin
                    , vector3d _vert_axis
                    , vector3d _horiz_axis
                    , float height
                    , float width);
    CollisionObject* collision(Ray* eyeRay);
    vector3d computeNormal(vector3d eyeDir);
    virtual ~SceneFinitePlane();
    
private:
    vector3d plane_origin; //"bottom-left" used for calculating bounds.
                                    //super.origin is for the light-source, which 
                                    //is the middle of the plane.
    vector3d normal;
    vector3d vertical;
    vector3d horizontal;
    
    vector3d reverseNormal;
    
    sdecimal32 v_distance; //vertical distance, or "height"
    sdecimal32 h_distance; //horizontal distance, or "width"
    
    sdecimal32 distance_to_origin;

};

}

#endif /*SCENEFINITEPLANE_H_*/
