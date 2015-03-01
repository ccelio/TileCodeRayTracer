#include "Camera.h"
#include "Ray.h"
#include "vector3d.h"
#include "PixelQueue.h"

namespace CelioRayTracer
{

Camera::Camera()
{
    //vector coordinates
    //TODO how to set these vectors is entirely non-intuitive
    //if the two specified vectors are 90degrees apart, circles will
    //turn into ovals, etc.
   // units are in feet
    screen_width        = 1; //dx
    screen_height       = 1; //dz
    screen_halfwidth    = screen_width / (sdecimal32) 2.0;
    screen_halfheight   = screen_height / (sdecimal32) 2.0;
    
    screen_origin       = vector3d(-4.f,-4.f, 1.5f);

    vector_horizontal   = vector3d(.1f,-.08f,0.f);  //x is horizontal
    vector_outwards     = vector3d(.08f,.1f,.01f);  //y is outwards
    vector_vertical     = vector3d();               //z is vertical

    vector_vertical.cross(vector_horizontal, vector_outwards);

    vector_outwards.normalize();
    vector_horizontal.normalize();
    vector_vertical.normalize();
    
    eye_distance        = 1; //eye is one foot behind screen
    //this = s*t1 + t2;
    eye_origin = (-eye_distance)*vector_outwards + screen_origin;
    //eye_origin = -vector_outwards + screen_origin;
    
    new_dir = vector_outwards;
    new_origin = screen_origin;
}

void Camera::setSceneTwoMirrors() {

    printf("Changing Camera Scene.\n");
    screen_origin       = vector3d(0,0, 2.5);
    
    //vector_outwards   = new Vector3d(.08,.1,-.03);    //y is outwards
    vector_outwards     = vector3d(.00,1,-.00); //y is outwards
    vector_vertical     = vector3d();       //z is vertical
    vector_horizontal   = vector3d(1,-.00,0);   //x is horizontal
    
    //vector_vertical.cross(vector_outwards, vector_horizontal);
    vector_vertical.cross(vector_horizontal, vector_outwards);

    
    //bug this isn't calculating right hand correctly 
    vector_outwards.normalize();
    vector_horizontal.normalize();
    vector_vertical.normalize();

    eye_distance        = 1; //eye is one foot behind screen
    //this = s*t1 + t2;
    eye_origin = (-eye_distance)*vector_outwards + screen_origin;
    
    new_dir = vector_outwards;
    new_origin = screen_origin;
    
    
}

Ray* Camera::createEyeRay(sdecimal32 dx_percent, sdecimal32 dy_percent){
    
    //scalar_x = dx [%] * screen_width - (screen_width / 2);
    //OPTIMIZE if width/height ==1, don't need to do this multiply
    sdecimal32 scalar_x = dx_percent*screen_width - screen_halfwidth;
    sdecimal32 scalar_y = dy_percent*screen_height - screen_halfheight;
    vector3d pixel = vector3d();
    pixel = screen_origin + scalar_x*vector_horizontal;
    pixel = pixel + scalar_y*vector_vertical;
        
    //Ray(origin, Pixel_(final), origin_(end));
    Ray *temp_ptr = new Ray(eye_origin, pixel, eye_origin); //BUG be careful of new
    return temp_ptr;
}

Camera::~Camera()
{
}

}
