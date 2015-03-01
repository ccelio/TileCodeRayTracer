#ifndef CAMERA_H_
#define CAMERA_H_

#include "rt_project_parameters.h"
#include "vector3d.h"
#include "Ray.h"

namespace CelioRayTracer
{

class Camera
{
public:
    Camera();
    Ray* createEyeRay(sdecimal32 dx_percent, sdecimal32 dy_percent);
    sdecimal32 inline getScreenWidth() {return screen_width;}
    sdecimal32 inline getScreeHeight() {return screen_height;}
    void setSceneTwoMirrors();
    virtual ~Camera();
private:
    //1 is 1 foot, that's how I will dimension everything
    //"screen is y along width, z along height. x is straight into the screen
    sdecimal32 screen_width;
    sdecimal32 screen_height;
    sdecimal32 screen_halfwidth;
    sdecimal32 screen_halfheight;
    
    
    vector3d screen_origin;
    
    vector3d new_origin;
    vector3d new_dir;
    
    vector3d vector_outwards; //points out of the camera
    vector3d vector_vertical;
    vector3d vector_horizontal;
    
    sdecimal32 eye_distance;
    vector3d eye_origin;
    
};

}

#endif /*CAMERA_H_*/

