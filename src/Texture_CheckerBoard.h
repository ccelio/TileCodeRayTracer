#ifndef TEXTURE_CHECKERBOARD_H_
#define TEXTURE_CHECKERBOARD_H_

#include "vector3d.h"
#include "Color_Values.h"
#include "ObjTexture.h"

namespace CelioRayTracer {

static const float width_default = 2;
static const float height_default = 2;

class Texture_CheckerBoard : public ObjTexture
{
    
public:

    Texture_CheckerBoard() : ObjTexture(width_default,height_default) {
        light_color = COLOR_WHITE;
        dark_color = COLOR_BLACK;
    }
    
    Texture_CheckerBoard(Color l, Color d) : ObjTexture(width_default,height_default)  {
        light_color = l;
        dark_color = d;
    }
    
    void setLightColor(Color l) { light_color = l; };
    void setDarkColor(Color d) {dark_color = d; };

    Color getTexturePixel(sdecimal32 _x, sdecimal32 _y) {

#if USING_FIXED_POINT
        float x = fix2float<16>(_x.intValue);
        float y = fix2float<16>(_y.intValue);
#else
        float x = _x;
        float y = _y;
#endif
        
        //TODO optimize this for fixed point
        if( x >=0)
            x = fmod(x, width); // x % width;
        else
            //x = (((-x) % width) + width/2.0) % width;
            //OPTIMIZE  there has to be a better way to do this.
            x = fmod((fmod((-x), width) + width/2.0f), width);
        if(y >= 0)
            y = fmod(y, height);  //y % height;
        else
            //y = (((-y) % height) + height/2.0) % height;
            y = fmod((fmod((-y), height) + height/2.0f), height);
        
            if(x < width / 2) {
                if( y < height / 2) 
                    return light_color;
                else 
                    return dark_color;
            } else {
                if( y < height / 2) 
                    return dark_color;
                else 
                    return light_color;
            }
    }

private:
    Color light_color;
    Color dark_color;

};

};//end of namespace

#endif /*TEXTURE_CHECKERBOARD_H_*/
