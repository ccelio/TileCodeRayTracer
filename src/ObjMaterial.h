#ifndef OBJMATERIAL_H_
#define OBJMATERIAL_H_

#include "stdio.h"
#include "vector3d.h"
#include "ObjTexture.h"

namespace CelioRayTracer {

class ObjMaterial
{
public:
    ObjMaterial() {
        myColor = vector3d(1.0f,1.0f,1.0f); //default color is white
        diffuse_factor = 1.0;
        specular_factor = 1.0;
        reflective_factor = 0;  //between [0,1]
        refractive_factor = 0;  //between [0,1]
        absorption_factor = 0.0f;
        myObjTexture_ptr = NULL;
    };
    virtual ~ObjMaterial() {
    //  if(myObjTexture_ptr)
    //      free(myObjTexture_ptr); BUG why is free not working?
    };
    
   void setColor(vector3d c) { 
        myColor = Color(c);
   }

    void setAbsorptionFactor(float _ab) {
        printf("Absoprtion getting called!\n");
        absorption_factor = _ab;
        if((absorption_factor+ reflective_factor +refractive_factor) > 1)
             printf("***ERROR. Setting Absorption Factor.\n");
    }
    
    void setDiffuseFactor(float _dif) {
        diffuse_factor = _dif;
    }
    void setSpecularFactor(float _spec) {
        specular_factor = _spec;
    }
    
    void setReflectiveFactor(float _refl) {
        reflective_factor = _refl;
        if((absorption_factor+ reflective_factor +refractive_factor) > 1)
             printf("***ERROR. Setting Reflective Factor.\n Ab: %f\n Refl: %f\n Refr: %f\n"
                     , absorption_factor, reflective_factor,refractive_factor);
    }

    void setRefractiveFactor(float _refract) {
        refractive_factor = _refract;
        if((absorption_factor+ reflective_factor +refractive_factor) > 1)
             printf("***ERROR. Setting Refractive Factor.\n Ab: %f\n Refl: %f\n Refr: %f\n"
                     , absorption_factor, reflective_factor,refractive_factor);
    }

    vector3d getColor() {return myColor;}
    ObjTexture* getTexture() {return myObjTexture_ptr;}
    void setTexture(ObjTexture* _texture_ptr) { myObjTexture_ptr = _texture_ptr;};
    float getDiffuseFactor() const {return diffuse_factor;} //BUG why const???
    float getSpecularFactor() {return specular_factor;}
    float getAbsorptionFactor() {return absorption_factor;}
    float getReflectiveFactor() {return reflective_factor;}
    float getRefractiveFactor() {return refractive_factor;}
    
private:

    /*Object's Properties*/
    Color myColor;
    ObjTexture* myObjTexture_ptr;
    
    float absorption_factor;    //between [0,1]
    
    float diffuse_factor;       //between [0,1]
    float specular_factor;      //between [0,1]
    float reflective_factor;    //between [0,1]
    float refractive_factor;    //between [0,1]
    
};

} //end namespace
#endif /*OBJMATERIAL_H_*/
