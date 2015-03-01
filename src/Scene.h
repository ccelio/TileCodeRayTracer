#ifndef SCENE_H_
#define SCENE_H_

#include "SceneObject.h"
#include "SceneFinitePlane.h"
#include "Camera.h"

#define MAX_OBJECT_COUNT 4000
namespace CelioRayTracer
{
//  I want to mention that things that are deallocated with 'free' don't get
//  "destroyed" in one sense of the word - their destructors don't get
//  called (unless you do it explicitly

class Scene
{
public:
    Scene();
    int initialize(void);
    int initialize1(void);
    int initializeTwoMirrors(Camera *myCamera);
    int getObjectCount() {return object_count;};
    void addObject(SceneObject* new_obj_ptr);
    SceneObject* getObject(int i);
    SceneFinitePlane** makeSceneBox(vector3d _origin, vector3d _dims);

    //for hierarchical scheme we give each core a subset
    //of the objects array for data-parallizing collision function
    int ParseObjectArray(int my_rank, int group_size);

    void SetObjectIndices(int my_rank, int group_size);
    int inline getSceneObjectStartIndex() { return scene_object_start_index;}
    int inline getSceneObjectFinalIndex() { return scene_object_final_index;}
    
    virtual ~Scene();
//  SceneObject **objects;
private:
    SceneObject **objects; //TODO make this stack based?
    int obj_pointer;
    int object_count;
    int scene_object_start_index;
    int scene_object_final_index;
};

}

#endif /*SCENE_H_*/
