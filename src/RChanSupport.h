#ifndef RCHANSUPPORT_H_
#define RCHANSUPPORT_H_

#if HARDWARE_IS_TILE

extern "C" {
    #include <ilib.h>
}
//#include "RayTracer.h"

extern ilibGroup my_fiefdom;
extern int my_local_rank;

/* FUNCTION PROTOTYPES */

int rchan_set_up_master_command_channel(void);
int rchan_set_up_serf_data_channel(void);

//int setUpReceiveChannels();
//int rchan_set_up_parrallel_collision_communications(void);
//int rchan_set_up_master_send_ports(void);
//int rchan_set_up_serf_send_port(void);
//int rchan_set_up_master_receive_ports(void** buffers, size_t buffer_size);
//int rchan_set_up_serf_receive_port(void* buffer, size_t buffer_size);

#define CHANNEL_MASTER_COMMAND_TAG 1600
#define CHANNEL_SERF_DATA_TAG 1000

//SERF PORTS 
ILIB_RAW_SEND_PORT(port_serf_send, 0);
ILIB_RAW_RECEIVE_PORT(port_serf_receive, 2);

//MASTER PORTS
ILIB_RAW_SEND_PORT(port_master_send1, 0);
ILIB_RAW_SEND_PORT(port_master_send2, 1);
ILIB_RAW_SEND_PORT(port_master_send3, 2);
ILIB_RAW_RECEIVE_PORT(port_sink, 2);

#endif
#endif
