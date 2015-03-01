#ifndef OBJTEXTURE_H_
#define OBJTEXTURE_H_

/**
 * @author CCelio
 * 
 */

#include "vector3d.h"
#include "rt_project_parameters.h"

namespace CelioRayTracer {

class ObjTexture
{
    
public:
    
    ObjTexture() { width = 1; height = 1;};
    ObjTexture(sdecimal32 w, sdecimal32 h) {
        
#if USING_FIXED_POINT
        width = fix2float<16>(w.intValue);
        height = fix2float<16>(h.intValue);
#else
        width = w;
        height = h;
#endif
    }

    virtual Color getTexturePixel(sdecimal32 x, sdecimal32 y) = 0;
    
    void setWidth(sdecimal32 w) { 
#if USING_FIXED_POINT
        width = fix2float<16>(w.intValue);
#else
        width = w;
#endif
        
    };
    void setHeight(sdecimal32 h) { 
#if USING_FIXED_POINT
        height = fix2float<16>(h.intValue);
#else
        height = h;
#endif
        };
    virtual ~ObjTexture() {};
    
protected:
    //any lower class texture calls need to mod on these values
    //to make sure they don't look outside the texture....
    float width;
    float height;
};

}; //end of namespace
#endif /*OBJTEXTURE_H_*/

