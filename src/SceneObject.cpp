
#include "SceneObject.h"
#include "vector3d.h" 
#include "stdio.h"

namespace CelioRayTracer 
{

SceneObject::SceneObject()
{
    //TODO what is the difference between vector3d origin(); and vector3d origin = vector3d();
    //vector3d origin();
    //ObjMaterial myMaterial();
    origin = vector3d();
    myMaterial = ObjMaterial();
    myMaterial.setDiffuseFactor(0.25f);
    isaLightSource = false;
    intensity = 1.0;
}

SceneObject::SceneObject(vector3d _o) {
    origin = _o;
    //ObjMaterial myMaterial();
    myMaterial = ObjMaterial();
    isaLightSource = false;
    intensity = 1.0;
}       

SceneObject::~SceneObject()
{
}

} //end of namespace
