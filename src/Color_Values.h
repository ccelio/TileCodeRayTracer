#ifndef COLOR_VALUES_H_
#define COLOR_VALUES_H_

#include "vector3d.h"

namespace CelioRayTracer {
    Color COLOR_WHITE      = Color(1.f,1.f,1.f);
    Color COLOR_RED        = Color(1.f,0.f,0.f);
    Color COLOR_YELLOW     = Color(1.f,1.f,0.f);
    Color COLOR_GREEN      = Color(0.f,1.f,0.f);
    Color COLOR_CYAN       = Color(0.f,1.f,1.f);
    Color COLOR_BLUE       = Color(0.f,0.f,1.f);
    Color COLOR_BLACK      = Color(0.f,0.f,0.f);

    Color COLOR_DARK_GREY  = Color(0.33f, 0.33f, 0.33f);
    Color COLOR_LIGHT_GREY = Color(2/3.f, 2/3.f, 2/3.f);
    Color COLOR_BROWN      = Color(0.2f, 0.2f, 0.0f);
}
#endif /*COLOR_VALUES_H_*/
