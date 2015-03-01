#ifndef RAYTRACER_H_
#define RAYTRACER_H_

#include "rt_project_parameters.h"
#include <time.h>
#include "Scene.h"
#include "SceneObject.h"
#include "PixelQueue.h"
#include "Camera.h"


#if HARDWARE_IS_TILE
extern "C" {
    #include <ilib.h>
}
#endif


typedef CelioRayTracer::SceneObject::CollisionObject CollisionObject;
typedef CelioRayTracer::Ray Ray;
typedef CelioRayTracer::Color Color;

#if HARDWARE_IS_TILE
    CelioRayTracer::vector3d** pixels;                                      
    //shared (locked) resource that stores all of the work to be done                       
    CelioRayTracer::PixelQueue* workQueue;
    CelioRayTracer::PixelQueue* bossWorkQueue;
    
    typedef struct {
        int* queue_size;
        ilibMutex mutex;
    } LockedInt;
    
    LockedInt* boss_work_size;
    
    typedef struct {
        bool* boolVal;
        ilibMutex mutex;
    } LockedBool;
    
    LockedBool* fiefdom_finished; //scheme #3, boss tells workers finished!

#else
    CelioRayTracer::vector3d pixels[SCREEN_HORIZONTAL_RESOLUTION][SCREEN_VERTICAL_RESOLUTION];
#endif

/* Personal Core Global Variables */

int my_global_rank;
CelioRayTracer::Scene my_scene = CelioRayTracer::Scene();
CelioRayTracer::Camera my_camera = CelioRayTracer::Camera();
CelioRayTracer::vector3d NULL_COLOR = CelioRayTracer::vector3d(0.75f, 0.75f, 0.75f); //COLOR_WHITE;

/* Hierachical Stuff */

#if HARDWARE_IS_TILE
typedef struct {
    CelioRayTracer::vector3d** sharedmem_pixels_ptr;
    CelioRayTracer::PixelQueue* sharedmem_workqueue_ptr;
} InitStruct;
#endif

//make sure this equals CORE_NUM!
#if IS_FOR_SIMULATION           
    #define FIEFDOM_SIZE 3
    #define NUMBER_OF_FIEFDOMS 1
#else
    #define FIEFDOM_SIZE 56 //3
    #define NUMBER_OF_FIEFDOMS 1 //18
#endif
#define BOSS_RANK 0
#define FIEFDOM_MASTER_RANK 0 //local rank, used for fine-grain collision parrallization
//#define GLOBAL_MASTER_RANK 0
int my_fiefdom_index; //this is a more useful way to keep track of all of the fiefdoms


#if HARDWARE_IS_TILE
ilibGroup* fiefdoms; 
ilibGroup my_fiefdom; //use this to push into ilib functions
#endif

int** fiefdom_ids; //for each fiefdom [i], a list of all ids in it 
int my_local_rank;

/*for parallel collision code
 * each tile only is responsible
 * for detecting collision for a 
 * fraction of the total scene objects
 */
//use this only for master tiles
//int scene_obj_start_index;
//int scene_obj_end_index;

CollisionObject* collision_array[FIEFDOM_SIZE];

/*
 * command = 1 -> parallelize collision detection
 * command = 2 -> compute inShade
 * command = 1337 -> finished with program
 */
#define FINISHED_COMMAND 1337
#define COLLISION_COMMAND 1415
#define INSHADE_COMMAND 123
#define NUM_PACKETS MAX_RECURSION_LEVEL+1 //2 before
typedef struct {
    int command;
    bool isNull;
    Ray ray;
    sdecimal32 distToLight; //only used for inshade calculations
} RayPacket;

typedef struct {
    bool isNull;
    CollisionObject collisionObject;
} CollisionObjectPacket;

//Part_strat==5 parameters
#define STATIC_PARTION_X_WIDTH 7
#define STATIC_PARTION_Z_WIDTH 8

//Part_strat==3 paramters
#define MIN_BOSS_WORK_COUNT 5
#define BATCH_GRAB_WORK_COUNT 4
#define BOSS_PRELOAD_COUNT 5 //40
//TODO make 'getRay' faster!




/* FILE I/O */

FILE *log_file;             /* created a logfile for playback from Java app */
#define INIT_FILE_NAME "init_file.txt"
#define LOG_FILE_NAME "raytracer_screen.txt"
#define log_file_mode "w"       /* w only writes, file need not exist */

#define USE_INIT_FILE FALSE /* FALSE -> use init_field() to randomly generate a field
                            *  TRUE-> use read_init_file() to play a pre-generated field from a file
                            */

/* benchmarking variables */
time_t start_time; //use the clock()
time_t stop_time;
double run_time_ns;
double run_time_us;
double run_time_s;

#if HARDWARE_IS_TILE
//these stats use the get_cycle_count() telera API call.
uint64_t    start_cycle_count;  //get_cycle_count (void);
uint64_t    end_cycle_count;    //get_cycle_count (void);
#define CLOCK_SPEED     700000000. /*0.70GHz clock speed used to calculate passage of time */
#endif

/***********************/
/* Function ProtoTypes */
/***********************/

/* Broadcast the pointers to the malloc_shared pixels array */
int broadcast_shared_pointers(void);
int broadcast_distributed_workqueue_pointer(void);

/* Run the main Ray Tracing Loop */
int raytrace_main(void);

int spawn_processes(void);
int serf_distributed_main(void);
int boss_setup_workqueue(void);
int distribute_procs_to_fiefdoms(void); 

/* Calculate the object's perceived color, based on the object color 
 * and the collision's position relative to the light source */
CelioRayTracer::Color* cosineShade(CelioRayTracer::Color* final_color
                                    , CelioRayTracer::Color* object_color
                                    , CelioRayTracer::SceneObject *light_object_ptr
                                    , CollisionObject *collision_ptr);

/* deduce whether the collision location is blocked from
 * the light source */

bool inShadeCollisionDetection(Ray* light_ray_ptr, float dist_to_light); 
//bool inShadeCollisionDetection(Ray* light_ray_ptr, float dist_to_light); 

bool inShade(CelioRayTracer::SceneObject *light_ptr
        , CollisionObject *collision_ptr);

/* Given a Ray, find the collision and return the CollisionObject */
CollisionObject* getCollision(Ray *temp_ray_ptr);

/* Calculate the color value for a given ray */
CelioRayTracer::Color calculatePixel(Ray *temp_ray_ptr, int recursive_level);

/* Log Printing Functions */
int printPixelsToLog(void);
int init_log(void);
int addToLogStr(char *tag, char *value_str);
int addToLogInt(char *tag, int value);
int addToLogDouble(char *tag, double value);



#if HARDWARE_IS_TILE

/* Direct Message Port Stuff */

/* Buffer Channel Support */
RayPacket* bchan_serf_wait_for_ray(void);
int bchan_raytrace_serf_collision_parallel(void);

int bchan_send_ray_to_serfs(int _command, Ray *temp_ray_ptr, sdecimal32 dist_to_light);
int inline bchan_send_ray_to_serfs(int _command, Ray *temp_ray_ptr) {
    bchan_send_ray_to_serfs(_command, temp_ray_ptr, 0);
    return 0;
}

void master_receive_collision_info(void);
//CollisionObject** master_receive_collision_info(void);
bool reduce_inshade_info(bool* inshade_array);

int bchan_set_up_parallel_collision_communications(void);
int bchan_set_up_master_send_ports(void);
int bchan_set_up_serf_send_port(void);
int bchan_set_up_master_receive_ports(void** buffers, size_t buffer_size);
int bchan_set_up_serf_receive_port(void* buffer, size_t buffer_size);

#define CHAN_TAG_FROM_SERF 1600
#define CHAN_TAG_FROM_MASTER 1000

/* Personal Port for Serf Tile */
ilibBufChanSendPort port_send_to_master;
ilibBufChanReceivePort port_receive_from_master;

/* ports for Master Tile */
ilibBufChanSendPort *port_send_to_serfs;
ilibBufChanReceivePort *port_receive_from_serfs;


/* Raw Channel Support */
/* Only for Partioning Strategy #7 */
int rchan_set_up_parallel_collision_communications(void);
int rchan_set_up_serf_data_channel(void);
int rchan_set_up_master_command_channel(void);


int rchan_send_ray_to_serfs(int _command, Ray *temp_ray_ptr, sdecimal32 dist_to_light);
int inline rchan_send_ray_to_serfs(int _command, Ray *temp_ray_ptr) {
    rchan_send_ray_to_serfs(_command, temp_ray_ptr, 0);
    return 0;
}
#define CHANNEL_MASTER_COMMAND_TAG 1600
#define CHANNEL_SERF_DATA_TAG 1000

//SERF PORTS 
ILIB_RAW_SEND_PORT(port_serf_send, 0);
ILIB_RAW_RECEIVE_PORT(port_serf_receive, 2);

//MASTER PORTS
ILIB_RAW_RECEIVE_PORT(port_sink, 2);

ilibRawSendPort *master_send_ports;

ILIB_RAW_SEND_PORT(port_master_send1, 0);
ILIB_RAW_SEND_PORT(port_master_send2, 1);
ILIB_RAW_SEND_PORT(port_master_send3, 2);
ILIB_RAW_SEND_PORT(port_master_send4, 0);
ILIB_RAW_SEND_PORT(port_master_send5, 0);
ILIB_RAW_SEND_PORT(port_master_send6, 0);
ILIB_RAW_SEND_PORT(port_master_send7, 0);
ILIB_RAW_SEND_PORT(port_master_send8, 0);
ILIB_RAW_SEND_PORT(port_master_send9, 0);
ILIB_RAW_SEND_PORT(port_master_send10, 0);
ILIB_RAW_SEND_PORT(port_master_send11, 0);
ILIB_RAW_SEND_PORT(port_master_send12, 0);
ILIB_RAW_SEND_PORT(port_master_send13, 0);
ILIB_RAW_SEND_PORT(port_master_send14, 0);
ILIB_RAW_SEND_PORT(port_master_send15, 0);
ILIB_RAW_SEND_PORT(port_master_send16, 0);
ILIB_RAW_SEND_PORT(port_master_send17, 0);
ILIB_RAW_SEND_PORT(port_master_send18, 0);
ILIB_RAW_SEND_PORT(port_master_send19, 0);
ILIB_RAW_SEND_PORT(port_master_send20, 0);
ILIB_RAW_SEND_PORT(port_master_send21, 0);
ILIB_RAW_SEND_PORT(port_master_send22, 0);
ILIB_RAW_SEND_PORT(port_master_send23, 0);
ILIB_RAW_SEND_PORT(port_master_send24, 0);
ILIB_RAW_SEND_PORT(port_master_send25, 0);
ILIB_RAW_SEND_PORT(port_master_send26, 0);
ILIB_RAW_SEND_PORT(port_master_send27, 0);
ILIB_RAW_SEND_PORT(port_master_send28, 0);
ILIB_RAW_SEND_PORT(port_master_send29, 0);
ILIB_RAW_SEND_PORT(port_master_send30, 0);
ILIB_RAW_SEND_PORT(port_master_send31, 0);
ILIB_RAW_SEND_PORT(port_master_send32, 0);
ILIB_RAW_SEND_PORT(port_master_send33, 0);
ILIB_RAW_SEND_PORT(port_master_send34, 0);
ILIB_RAW_SEND_PORT(port_master_send35, 0);
ILIB_RAW_SEND_PORT(port_master_send36, 0);
ILIB_RAW_SEND_PORT(port_master_send37, 0);
ILIB_RAW_SEND_PORT(port_master_send38, 0);
ILIB_RAW_SEND_PORT(port_master_send39, 0);
ILIB_RAW_SEND_PORT(port_master_send40, 0);
ILIB_RAW_SEND_PORT(port_master_send41, 0);
ILIB_RAW_SEND_PORT(port_master_send42, 0);
ILIB_RAW_SEND_PORT(port_master_send43, 0);
ILIB_RAW_SEND_PORT(port_master_send44, 0);
ILIB_RAW_SEND_PORT(port_master_send45, 0);
ILIB_RAW_SEND_PORT(port_master_send46, 0);
ILIB_RAW_SEND_PORT(port_master_send47, 0);
ILIB_RAW_SEND_PORT(port_master_send48, 0);
ILIB_RAW_SEND_PORT(port_master_send49, 0);
ILIB_RAW_SEND_PORT(port_master_send50, 0);
ILIB_RAW_SEND_PORT(port_master_send51, 0);
ILIB_RAW_SEND_PORT(port_master_send52, 0);
ILIB_RAW_SEND_PORT(port_master_send53, 0);
ILIB_RAW_SEND_PORT(port_master_send54, 0);
ILIB_RAW_SEND_PORT(port_master_send55, 0);
ILIB_RAW_SEND_PORT(port_master_send56, 0);
ILIB_RAW_SEND_PORT(port_master_send57, 0);
ILIB_RAW_SEND_PORT(port_master_send58, 0);

#endif //tile hardware
#endif /*RAYTRACER_H_*/
