#ifndef VECTOR3D_FIXEDPOINT_H_
#define VECTOR3D_FIXEDPOINT_H_
#ifndef VECTOR3D_H_
#define VECTOR3D_H_

#include <math.h>
#include <stdio.h>
#include "rt_project_parameters.h"
#include "fixed_class.h"
#include "fixed_func.h"

/*
 * some of the optimization techniques shown here learned from:
 * http://www.devmaster.net/articles/raytracing_series/part2.php
 * (this is my first c++ program, so this class is heavily inspired by the above link).
 * 
 * (such as operators, unions, and friendship)
 * -http://www.learning-computer-programming.blogspot.com/2007/08/operator-overloading-using-friend.html
 */

//i'm getting 10% increase in speed with carmack hacks!
//ie, 13.3us - 14.1us per pixel
#define CARMACK_HACK_INV_SQRT false
#define CARMACK_HACK_SQRT false
//#define POINT 16 //ie, 16.16 fixed point 
//#define USING_FIXED_POINT false

namespace CelioRayTracer
{

class vector3d
{
public:
#if USING_FIXED_POINT
   typedef fixed_point<16> fixed;
   fixed x;
   fixed y;
   fixed z;
#else
   union {
      struct {float x; float y; float z;};
      struct {float r; float g; float b;};
      struct {float red; float grn; float blue;};
      };
#endif

   //constructors
   vector3d() {
      x=0.0f; y=0.0f; z=0.0f;};
   vector3d(sdecimal32 _x, sdecimal32 _y, sdecimal32 _z) {
      x =_x; y =_y; z =_z;};
   vector3d(const vector3d& _v) { 
      x = _v.x; y = _v.y; z = _v.z; };

   //methods
   //void set(const vector3d& _v) { x = _v.x; y = _v.y; z = _v.z; }
   void inline normalize() {
      #if CARMACK_HACK_INV_SQRT
         float inv_length = inv_sqrt_optimized(x*x+y*y+z*z);
         x = x*inv_length;
         y = y*inv_length;
         z = z*inv_length;
      #else
         //OPTIMIZE inverting squareroots!
         #if USING_FIXED_POINT
            fixed length = fixed_sqrt(x*x + y*y + z*z);
         #else
            float length = sqrt(x*x + y*y + z*z);
         #endif
         x = x/length;
         y = y/length;
         z = z/length;
      #endif
   };
   float length() {
      #if CARMACK_HACK_SQRT
         return sqrt_optimized(x*x + y*y + z*z);
      #else
         #if USING_FIXED_POINT
            return fix2float<16>(fixed_sqrt(x*x + y*y + z*z).intValue); 
         #else
            return sqrt(x*x + y*y + z*z);
         #endif
      #endif
   };
#if USING_FIXED_POINT
   fixed fixed_length() {
         return fixed_sqrt(x*x + y*y + z*z); 
   };
#endif

   //BIG BUG!!!!! dot product was returning a float, not a fixed
   float inline dot(const vector3d& v) { 
#if USING_FIXED_POINT
      return fix2float<16>((x*v.x + y*v.y + z*v.z).intValue);
#else
      return x*v.x + y*v.y + z*v.z;
#endif
   };

   void cross(const vector3d& v1, const vector3d& v2) { 
      x = v1.y*v2.z - v1.z*v2.y;
      y = v1.z*v2.x - v1.x*v2.z;
      z = v1.x*v2.y - v1.y*v2.x;};

   //operator over-loading
   void operator +=( vector3d _v) {x += _v.x; y+= _v.y; z+= _v.z;};
   void operator -=( vector3d _v) {x -= _v.x; y-= _v.y; z-= _v.z;};
   void operator *=( float f) {x *= f; y *= f; z *= f;};

   //OPTIMIZE set this to float?
   vector3d operator- () const {return vector3d(0-x, 0-y, 0-z);};
   //BUG do we want to only do this as pointers?
   //I need to be VERY careful of how memory is being handled by these stack vectors
   friend vector3d operator + (const vector3d& v1, const vector3d& v2) 
      { return vector3d(v1.x+v2.x, v1.y+v2.y, v1.z+v2.z); }
   friend vector3d operator - (const vector3d& v1, const vector3d& v2) 
      { return vector3d(v1.x-v2.x, v1.y-v2.y, v1.z-v2.z); }
   friend vector3d operator * (const vector3d& v,sdecimal32 f) 
      { return vector3d(v.x*f, v.y*f, v.z*f); }
   friend vector3d operator * (sdecimal32 f, const vector3d& v) 
      { return vector3d(v.x*f, v.y*f, v.z*f); }
friend vector3d operator * (const vector3d& v1, const vector3d& v2) 
      { return vector3d(v1.x*v2.x, v1.y*v2.y, v1.z*v2.z); }

#if CARMACK_HACK_INV_SQRT
    //amazing bit-level hack 
   //super fast, but sacrifices some accurancy (+/- ~0.001)
   float inv_sqrt_optimized( float x ) 
   { //http://www.mceniry.net/papers/Fast%20Inverse%20Square%20Root.pdf
      union {float f; unsigned long ul; } y; 
         y.f = x; 
         y.ul = ( 0xBE6EB50CUL - y.ul ) >> 1; 
         y.f = 0.5f * y.f * ( 3.0f - x * y.f * y.f ); 
         return y.f; 
         /*Adjusting for double-precision float-point numbers changes 
         * the Òfloatto ÒdoubleÓ, the Òunsigned longÓ to Òunsigned long longÓ, 
         * and the constant from above to the unsigned long long value 
         * of 0xBFCDD6A18F6A6F55ULL. */
   }
#endif

#if CARMACK_HACK_SQRT
   float sqrt_optimized(float number) {
      long i;
      float x, y;
      const float f = 1.5F;
         
      x = number * 0.5F;
      y  = number;
      i  = * ( long * ) &y;
      i  = 0x5f3759df - ( i >> 1 );
      y  = * ( float * ) &i;
      y  = y * ( 1.5F - ( x * y * y ) );
      //below line adds accurancy, but makes this twice as slow
      //y  = y * ( 1.5F - ( x * y * y ) )
      return number * y;
}
#endif
   virtual ~vector3d(){};
};

typedef vector3d Color;

} //end namespace

#endif /*VECTOR3D_H_*/
#endif /*VECTOR3D_FIXEDPOINT_H_*/
