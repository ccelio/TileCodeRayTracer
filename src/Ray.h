#ifndef RAY_H_
#define RAY_H_

//can the silly svn see me here? linei 4 of Ray.h?

#include "vector3d.h"
#include "stdio.h"

namespace CelioRayTracer
{

class Ray
{
public:
    Ray() { 
        origin = vector3d(0.0f, 0.0f, 0.0f);
        direction = vector3d(1.f, 0.f, 0.f); 
    };

    //OPTIMIZE pass by reference or by value?
    Ray(vector3d _origin, vector3d _direction) {
        origin = _origin;
        direction = _direction;
        direction.normalize();
    };
    Ray(vector3d _origin, vector3d point_final, vector3d point_init) {
        origin = _origin;
        direction = point_final - point_init;
        direction.normalize();
    }
    
    vector3d inline getOrigin() { return origin; };
    vector3d inline getDirection() {return direction; };
    virtual ~Ray() {};
private:
    vector3d origin;
    vector3d direction;
};

}

#endif /*RAY_H_*/

