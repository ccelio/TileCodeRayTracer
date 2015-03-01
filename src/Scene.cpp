#include "Scene.h"
#include <stdlib.h>
#include "vector3d.h"
#include "SceneSphere.h"
#include "SceneInfinitePlane.h"
#include "SceneFinitePlane.h"
#include "Color_Values.h"
#include "ObjTexture.h"
#include "Texture_CheckerBoard.h"

namespace CelioRayTracer
{

Scene::Scene()
{
    objects = new SceneObject*[MAX_OBJECT_COUNT];
    //obj_pointer = 0;
    object_count=0;
    //TODO do i have to check if new worked?
}

//two mirrors
int Scene::initializeTwoMirrors(Camera *myCamera) {

    SceneObject* temp_ptr;
    ObjTexture* texture_ptr;
    Color _color;
    
    /*
    temp_ptr = new SceneSphere(vector3d(6.99, 6.99, 5.5), .15);
    temp_ptr->setIndex(object_count);
    temp_ptr->setAsLightSource();
    temp_ptr->setIntensity(.75);
    this->addObject(temp_ptr);
    */
    
    temp_ptr = new SceneSphere(vector3d(5, 10, 10), .15);
    temp_ptr->setAsLightSource();
    temp_ptr->setIntensity(.75f);
    this->addObject(temp_ptr);


    //origin ball
    temp_ptr = new SceneSphere(vector3d(), .10);
    temp_ptr->getMaterial()->setColor(COLOR_CYAN);
    this->addObject(temp_ptr);

    //THE SUN
    temp_ptr = new SceneSphere(vector3d(-40, 100, 40), 10);
    temp_ptr->getMaterial()->setColor(COLOR_YELLOW);
    temp_ptr->getMaterial()->setSpecularFactor(0.25);
    this->addObject(temp_ptr);
    
    temp_ptr = new SceneSphere(vector3d(), .05);
    temp_ptr->getMaterial()->setColor(COLOR_CYAN);
    this->addObject(temp_ptr);

    temp_ptr = new SceneSphere(vector3d(), .02);
    temp_ptr->getMaterial()->setColor(COLOR_CYAN);
    //objects.add(temp_ptr);
    this->addObject(temp_ptr);
    
    /************ GROUND PLANE ************/
    temp_ptr = new SceneInfinitePlane(vector3d(0,0,0),
                                        vector3d(0,0,1),
                                        vector3d(1,0,0));
    
    texture_ptr = new Texture_CheckerBoard(COLOR_WHITE, COLOR_BLACK);
    texture_ptr->setHeight(3.0f);
    texture_ptr->setWidth(3.0f);
    temp_ptr->getMaterial()->setTexture(texture_ptr);
    
    
    //temp_ptr->getMaterial()->setColor(COLOR_CYAN);
    temp_ptr->getMaterial()->setReflectiveFactor(.05);
    temp_ptr->getMaterial()->setDiffuseFactor(.5);
    //objects.add(temp_ptr);
    this->addObject(temp_ptr);


    /************ Mirror#1 PLANE ************/
    temp_ptr = new SceneFinitePlane(vector3d(-1.75,7,0),
                                        vector3d(0,-1,0),
                                        vector3d(1,0,0)
                                        , 5
                                        , 3.5f);
    //temp_ptr->getMaterial()->setTexture(new CheckerBoardTexture(3,3));
    temp_ptr->getMaterial()->setColor(COLOR_WHITE);
    temp_ptr->getMaterial()->setReflectiveFactor(1.0);
    temp_ptr->getMaterial()->setDiffuseFactor(.0);
    //objects.add(temp_ptr);
    this->addObject(temp_ptr);
    

    temp_ptr = new SceneFinitePlane(vector3d(-2,7,0),
                                        vector3d(0,-1,0),
                                        vector3d(1,0,0)
                                        , 5.25
                                        , 4.0);
    //temp_ptr->getMaterial()->setTexture(new CheckerBoardTexture(3,3));
    temp_ptr->getMaterial()->setColor(COLOR_BROWN);
    temp_ptr->getMaterial()->setDiffuseFactor(.5);
    //objects.add(temp_ptr);
    this->addObject(temp_ptr);

    /************ Mirror#2 PLANE ************/
    temp_ptr = new SceneFinitePlane(vector3d(1.75,-7,0),
                                        vector3d(0,1,0),
                                        vector3d(-1,0,0)
                                        , 5
                                        , 3.5);
    //temp_ptr->getMaterial()->setTexture(new CheckerBoardTexture(3,3));
    temp_ptr->getMaterial()->setColor(COLOR_WHITE);
    temp_ptr->getMaterial()->setDiffuseFactor(.0);
    temp_ptr->getMaterial()->setReflectiveFactor(1.0);
    //objects.add(temp_ptr);
    this->addObject(temp_ptr);

    temp_ptr = new SceneFinitePlane(vector3d(2,-7,0),
                                        vector3d(0,1,0),
                                        vector3d(-1,0,0)
                                        , 5.25
                                        , 4.0);
    //temp_ptr->getMaterial()->setTexture(new CheckerBoardTexture(3,3));
    temp_ptr->getMaterial()->setColor(COLOR_GREEN);
    temp_ptr->getMaterial()->setColor(COLOR_BROWN);
    temp_ptr->getMaterial()->setDiffuseFactor(.5);
    //objects.add(temp_ptr);
    this->addObject(temp_ptr);
    

    //3920 we need this number of objects
    float BASE_X = 14.50f;
    float BASE_Y = 15.0f;
    float BASE_Z = 16.5f;
    float i_start = 0;
    float j_start = 0;
    float offset = 0.5f;
    
    
    
    for(float k=0; k < BASE_Z; k+=offset) {
        i_start+=offset;
        j_start+=offset;
        for(float i=i_start; i < BASE_X-i_start; i+=offset) {
            for(float j=j_start; j < BASE_Y -j_start; j+=offset) {
                temp_ptr = new SceneSphere(vector3d(i+2.65, j+15, k), 0.33);
                temp_ptr->getMaterial()->setColor(COLOR_GREEN);
                this->addObject(temp_ptr);
            }
        }
    }
    
    
    //3920 we need this number of objects
     BASE_X = 5.f;
     BASE_Y = 5.0f;
     BASE_Z = 5.f;
     i_start = 0;
     j_start = 0;
     offset = 0.65f; //.65 3913
    
    
    
    for(float k=0; k < BASE_Z; k+=offset) {
        i_start+=offset;
        j_start+=offset;
        for(float i=i_start; i < BASE_X-i_start; i+=offset) {
            for(float j=j_start; j < BASE_Y -j_start; j+=offset) {
                temp_ptr = new SceneSphere(vector3d(i-6, j+10, k), .5);
                temp_ptr->getMaterial()->setColor(COLOR_RED);
                this->addObject(temp_ptr);
            }
        }
    }
    
    
    //3920 we need this number of objects
     BASE_X = 1.f;
     BASE_Y = 1.0f;
     BASE_Z = 1.f;
     i_start = 0;
     j_start = 0;
     offset = 0.33f; //.65 3913
    
    
    
    for(float k=0; k < BASE_Z; k+=offset) {
        i_start+=offset;
        j_start+=offset;
        for(float i=i_start; i < BASE_X-i_start; i+=offset) {
            for(float j=j_start; j < BASE_Y -j_start; j+=offset) {
                temp_ptr = new SceneSphere(vector3d(i, j+20, k), 0.33);
                temp_ptr->getMaterial()->setColor(COLOR_RED);
                this->addObject(temp_ptr);
            }
        }
    }
    
    printf("ObjectCount: %d\n", object_count);
    myCamera->setSceneTwoMirrors();
    
    scene_object_start_index = 0;
    scene_object_final_index = object_count;
    return 0;
}


int Scene::initialize()
{   
//  int box_count = 4;
//  int sphere_count = 6;
//  int plane_count = 1;
//  int real_obj_count = box_count*6 + sphere_count + plane_count;
    //TODO add object count to log_file
    
    //objects = (SceneObject**) malloc(object_count*sizeof(SceneObject*));
    //objects = new SceneObject*[object_count];
    //if(objects==NULL)
    //  return 1;
    
    SceneObject* temp_ptr;
    ObjMaterial* temp_omat_ptr;
    ObjTexture* texture_ptr;
    SceneFinitePlane** planes;
    //int start_index;
    Color _color;
    
    //BUG check for null pointers
    
//  printf("Greetings\n");
    /* LIGHTS */
    temp_ptr = new SceneSphere(vector3d(6.99, 6.99, 5.5), .15);
    temp_ptr->setIndex(object_count);
    temp_ptr->setAsLightSource();
    temp_ptr->setIntensity(.75);
    //objects[0] = temp_ptr;
    this->addObject(temp_ptr);

    /************ WALL PANELS ************/
    /*temp = new SceneFinitePlane(new Vector3d(0,0,4.8)
                                , new Vector3d(0,0,-1)
                                , new Vector3d(1,0,0)
                                , 1, 3);*/
    temp_ptr = new SceneSphere(vector3d(0,0,4.8), .15);
    temp_ptr->setIndex(object_count);
    //temp.getMaterial().setColor(COLOR_WHITE);
    temp_ptr->setAsLightSource();
    temp_ptr->setIntensity(1.0);
    //objects[1] = temp_ptr;
    this->addObject(temp_ptr);
    
    /* BALLS */
    temp_ptr = new SceneSphere(vector3d(0, 0, 2), 1);
    temp_ptr->setIndex(object_count);
    temp_ptr->getMaterial()->setColor(COLOR_RED);
    temp_ptr->getMaterial()->setReflectiveFactor(1.00f);
    this->addObject(temp_ptr);
    
    temp_ptr = new SceneSphere(vector3d(0, 0, 0), 0.01);
    temp_ptr->setIndex(object_count);
    this->addObject(temp_ptr);
    
    
    //this would mess up shading stuff
    for(int i=0; i < 2; i++) {
        temp_ptr = new SceneSphere(vector3d(-2.5 + ((i+0)*2.5), 3, 1), 1);
        temp_ptr->setIndex(object_count);
        temp_ptr->getMaterial()->setColor(COLOR_RED);
        if(i==1) {
            temp_ptr->getMaterial()->setColor(COLOR_WHITE);
            temp_ptr->getMaterial()->setReflectiveFactor(1.00f);
            temp_ptr->getMaterial()->setDiffuseFactor(0.00f);
        } else if (i==2) {
            temp_ptr->getMaterial()->setColor(COLOR_GREEN);
                
            
        } else {
            temp_ptr->getMaterial()->setSpecularFactor(.5);
        }
        //objects[i] = temp_ptr;
        this->addObject(temp_ptr);
    }
    

    //origin ball
    temp_ptr = new SceneSphere(vector3d(), .10);
    temp_ptr->setIndex(object_count);
    temp_ptr->getMaterial()->setColor(COLOR_CYAN);
    //objects[5] = temp_ptr;
    this->addObject(temp_ptr);

    
    /************ GROUND PLANE ************/
    temp_ptr = new SceneInfinitePlane(vector3d(0,0,0),
                                        vector3d(0,0,1),
                                        vector3d(1,0,0));
    temp_ptr->setIndex(object_count);
    temp_omat_ptr = temp_ptr->getMaterial();
    temp_omat_ptr->setColor(COLOR_GREEN);
    temp_omat_ptr->setReflectiveFactor(.5);
    temp_omat_ptr->setDiffuseFactor(.5);
    
    texture_ptr = new Texture_CheckerBoard(COLOR_WHITE, COLOR_BLACK);
    texture_ptr->setHeight(3.0f);
    texture_ptr->setWidth(3.0f);
    temp_omat_ptr->setTexture(texture_ptr);
    //objects[6] = temp_ptr;
    this->addObject(temp_ptr);

    /************ PEDESTAL ************/
    
    planes = this->makeSceneBox(vector3d(-.5f,-.5f,0)
                                , vector3d(1.f,1.f,1.f));
    
//  start_index = 7;
    _color = COLOR_BROWN;
    for(int i=0; i<6; i++) { //6 b/c a box has six sides
        temp_ptr = planes[i];
        temp_ptr->setIndex(object_count);
        temp_omat_ptr = temp_ptr->getMaterial();
        temp_omat_ptr->setColor(_color);
        temp_omat_ptr->setReflectiveFactor(0.00f);
        temp_omat_ptr->setDiffuseFactor(1.00f);
        //objects[start_index+i] = temp_ptr;
        this->addObject(temp_ptr);
        
    }
    
    planes = this->makeSceneBox(vector3d(-.70f,-.70f,0)
                                , vector3d(1.4f,1.4f,0.25f));
    
//  start_index = 13;
    _color = COLOR_BROWN;
    for(int i=0; i<6; i++) { //6 b/c a box has six sides
        temp_ptr = planes[i];
        temp_ptr->setIndex(object_count);
        temp_omat_ptr = temp_ptr->getMaterial();
        temp_omat_ptr->setColor(_color);
        temp_omat_ptr->setSpecularFactor(0.20f);
        //objects[start_index+i] = temp_ptr;
        this->addObject(temp_ptr);
        
    }

    /************ OUTSIDE WALLS ************/
    
    planes = this->makeSceneBox(vector3d(-7,-7,-1), vector3d(14,14,7));
    
//  start_index = 19; //TODO check objects for nulls before calling them?
    _color = COLOR_DARK_GREY;

    for(int i=0; i<6; i++) { //6 b/c a box has six sides
        temp_ptr = planes[i];
        temp_ptr->setIndex(object_count);
        temp_omat_ptr = temp_ptr->getMaterial();
        temp_omat_ptr->setColor(_color);
        temp_omat_ptr->setReflectiveFactor(0.00f);
        temp_omat_ptr->setDiffuseFactor(1.00f);
        temp_omat_ptr->setSpecularFactor(0.0f);
        //objects[start_index+i] = temp_ptr;
        this->addObject(temp_ptr);
    }

    //top part of the room
    planes = this->makeSceneBox(vector3d(-6,-6, 5), vector3d(12,12,1));
    
//  start_index = 25;
    _color = COLOR_LIGHT_GREY;

    for(int i=0; i<6; i++) { //6 b/c a box has six sides
        temp_ptr = planes[i];
        temp_ptr->setIndex(object_count);
        temp_omat_ptr = temp_ptr->getMaterial();
        temp_omat_ptr->setColor(_color);
        temp_omat_ptr->setReflectiveFactor(0.5f);
        temp_omat_ptr->setSpecularFactor(0.5f);
        //objects[start_index+i] = temp_ptr;
        this->addObject(temp_ptr);
    }
    
//  printf("Finished Creating Scene.\n");
    scene_object_start_index = 0;
    scene_object_final_index = object_count;

    return 0;
}

/* In order to "Build a Box", we make six Finite Planes
 * and place each finite plane into the objects array.
 */
SceneFinitePlane** Scene::makeSceneBox(vector3d _origin
                                        , vector3d _dims) {
    
    //TODO - calculate corners based on vectors, not straight up axis
    
    vector3d c0 = vector3d(_origin);
    vector3d c1 = vector3d(_origin); c1.x += _dims.x;
    vector3d c2 = vector3d(_origin); c2.y += _dims.y;
    vector3d c3 = vector3d(_origin); c3.z += _dims.z;
    vector3d c4 = vector3d(_origin); c4.x += _dims.x; c4.y += _dims.y;
    vector3d c5 = vector3d(_origin); c5.x += _dims.x; c5.z += _dims.z;
    vector3d c6 = vector3d(_origin); c6.y += _dims.y; c6.z += _dims.z;
    vector3d c7 = vector3d(_origin.x + _dims.x, _origin.y + _dims.y, _origin.z + _dims.z);
    
    SceneFinitePlane** planes;
    planes = new SceneFinitePlane*[6];
    planes[0] = new SceneFinitePlane(c0, c3, c2);
    planes[1] = new SceneFinitePlane(c0, c3, c1);
    planes[2] = new SceneFinitePlane(c0, c1, c2);
    planes[3] = new SceneFinitePlane(c7, c4, c6);
    planes[4] = new SceneFinitePlane(c7, c4, c5);
    planes[5] = new SceneFinitePlane(c7, c5, c6);
    
    return planes;
}


int Scene::initialize1()
{   
    int box_count = 0;
    int sphere_count = 1;
    int plane_count = 1;
    int real_obj_count = box_count*6 + sphere_count + plane_count;
    printf("Obj_Count: %d\n", real_obj_count);
    object_count = real_obj_count;
    //objects = (SceneObject**) malloc(object_count*sizeof(SceneObject*));
    objects = new SceneObject*[object_count];
    if(objects==NULL)
        return 1;
    
    SceneObject* temp_ptr;
    ObjMaterial* temp_omat_ptr;
    ObjTexture* texture_ptr;
    //SceneFinitePlane** planes;
    //int start_index;
    Color _color;
    
    //TODO Add in plane primitive, make sure camera, locs are working correctly.
    //BUG check for null pointers
    
    /* LIGHTS */
    temp_ptr = new SceneSphere(vector3d(6.99, 6.99, 5.5), .15);
    temp_ptr->setAsLightSource();
    temp_ptr->setIntensity(.75);
    //objects[0] = temp_ptr;
    this->addObject(temp_ptr);

    /************ GROUND PLANE ************/
    temp_ptr = new SceneInfinitePlane(vector3d(0,0,0),
                                        vector3d(0,0,1),
                                        vector3d(1,0,0));
    temp_omat_ptr = temp_ptr->getMaterial();
    temp_omat_ptr->setColor(COLOR_GREEN);
    temp_omat_ptr->setReflectiveFactor(.5);
    temp_omat_ptr->setDiffuseFactor(.5);
    
    texture_ptr = new Texture_CheckerBoard(COLOR_WHITE, COLOR_BLACK);
    texture_ptr->setHeight(3.0f);
    texture_ptr->setWidth(3.0f);
    temp_omat_ptr->setTexture(texture_ptr);
    //objects[1] = temp_ptr;
    this->addObject(temp_ptr);

    printf("Finished Creating Scene #1.\n");
    
    return 0;
}

void Scene::addObject(SceneObject* new_obj_ptr) {
    
    
    if(object_count+1  >= MAX_OBJECT_COUNT) {
        printf("***ERROR. Added too many objects to scene.\n");
    } else {
        objects[object_count] = new_obj_ptr;
        ++object_count;
    }
}
SceneObject* Scene::getObject(int i) {
    //buffer overflows are for cool people
    return objects[i];
}


void Scene::SetObjectIndices(int my_rank, int group_size) {
    
    int new_object_count = object_count / group_size;
    int start_index =  my_rank * new_object_count;

    // give overflow to the last guy. it sucks, and i need 
    // to be smart about the number of objects in the scene
    // being easily divided amongst all of the cores in the 
    // fiefdom.
    if(my_rank == group_size-1)  {
        new_object_count = object_count - start_index;
    }
    
    //this is used for the tiles (such as master cores)
    //that do not parse the object array, but still only
    //want to only search through a part of the object array
    //for collision detection
    scene_object_start_index = start_index;
    scene_object_final_index = start_index + new_object_count;
}

int Scene::ParseObjectArray(int my_rank, int group_size) {
    
    int new_object_count = object_count / group_size;
    int start_index =  my_rank * new_object_count;

    // give overflow to the last guy. it sucks, and i need 
    // to be smart about the number of objects in the scene
    // being easily divided amongst all of the cores in the 
    // fiefdom.
    if(my_rank == group_size-1)  {
        new_object_count = object_count - start_index;
    }
    
    scene_object_start_index = 0;
    scene_object_final_index = new_object_count;
    printf(" parsing object array! %d of %d objCount: %d, start_index: %d  new obj_count: %d\n", my_rank, group_size, object_count, start_index, new_object_count);

    SceneObject** new_objects = new SceneObject*[new_object_count];

    for(int i= 0; i < new_object_count; i++) {
        new_objects[i] = objects[i + start_index];
    }
    
    //delete the sceneObjects we will not be using....
    //TODO okay, this is a big mess in terms of memory leaking. 
    /*
    for(int i=0; i < start_index; i++) {
        delete objects[i];
    }
    for(int i=end_index; i < object_count; i++) {
        delete objects[i];
    } */
    
    objects = new_objects;
    object_count = new_object_count;

    return 0;
}

Scene::~Scene()
{
    //TODO free what I use (including the sceneobjects)
    delete objects;
}

}
