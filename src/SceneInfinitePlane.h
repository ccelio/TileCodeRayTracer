#ifndef SCENEINFINITEPLANE_H_
#define SCENEINFINITEPLANE_H_

#include "SceneObject.h"

//i have a serious problem with points on a surface are
//found to "collide" with the surface, screwing up
//shading calculations, and causing black dots to be everywhere
//so i fix this by offsetting the intersection point
//from the surface
#define INTERSECTION_OFFSET_DIST 1E-3
    
namespace CelioRayTracer
{

class SceneInfinitePlane : public SceneObject
{
public:
    SceneInfinitePlane();
    SceneInfinitePlane(vector3d o, vector3d n, vector3d h);
    CollisionObject* collision(Ray* eyeRay);
    vector3d computeNormal(vector3d eyeDir);
    virtual ~SceneInfinitePlane();
private:
    vector3d normal;
    vector3d vertical;
    vector3d horizontal;

    vector3d reverseNormal;

    sdecimal32 distance_to_origin;
};

}

#endif /*SCENEINFINITEPLANE_H_*/
