#ifndef SCENESPHERE_H_
#define SCENESPHERE_H_

#include "SceneObject.h"
#include "stdio.h"

namespace CelioRayTracer
{

class SceneSphere : public SceneObject
{
public:
    SceneSphere();
    SceneSphere(vector3d _origin, sdecimal32 _radius);
    CollisionObject* collision(Ray* eyeRay);
    virtual ~SceneSphere();
private:
    sdecimal32 radius;
    sdecimal32 radius_squared;
};

}

#endif /*SCENESPHERE_H_*/
