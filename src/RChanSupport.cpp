#if HARDWARE_IS_TILE 

#include "RChanSupport.h"

int rchan_set_up_master_command_channel() {

/*  for(int i=0; i < FIEFDOM_SIZE-1; i++) {
        //i+1 is the receiver rank
        
        if (ilib_bufchan_connect(my_fiefdom
                                , MASTER_RANK
                                , CHAN_TAG_FROM_MASTER+i+1
                                , i+1
                                , CHAN_TAG_FROM_MASTER+i+1) < 0) {
            ilib_die("Failed to define channel.");
        } */

    if(my_local_rank==FIEFDOM_MASTER_RANK) {
        // Open our send ports.
        if (ilib_rawchan_open_sender(CHANNEL_MASTER_COMMAND_TAG+1
            , port_master_send1) < 0)
            ilib_die("Failed to open SendPort.");
        if (ilib_rawchan_open_sender(CHANNEL_MASTER_COMMAND_TAG+2
            , port_master_send2) < 0)
            ilib_die("Failed to open SendPort.");
        if (ilib_rawchan_open_sender(CHANNEL_MASTER_COMMAND_TAG+3
            , port_master_send3) < 0)
            ilib_die("Failed to open SendPort.");
    
    } else {
        //is a serf tile
        if(ilib_rawchan_open_receiver(CHANNEL_MASTER_COMMAND_TAG+my_local_rank, port_serf_receive) < 0)
            ilib_die("Failed to open ReceivePort.");
    }
        


    return 0;
}



/* Set up the Serf Data Channel
 * consists of Master Sink
 * and 3 serf senders 
 * Note: this is only run by
 * the master tile */
int rchan_set_up_serf_data_channel() 
{
    ilibSink sink;

    if(my_local_rank==FIEFDOM_MASTER_RANK) {
        if (ilib_rawchan_start_sink(my_fiefdom  
                                , FIEFDOM_MASTER_RANK
                                , CHANNEL_SERF_DATA_TAG
                                , &sink) != ILIB_SUCCESS)
            ilib_die("Failed to start sink.");

        //add the serfs as senders for the sink
        for(int i=0; i < FIEFDOM_SIZE; i++) {
            if (ilib_rawchan_add_sink_sender(my_fiefdom
                                    , i+1
                                    , CHANNEL_SERF_DATA_TAG+i+1
                                    , &sink) != ILIB_SUCCESS)
            ilib_die("Failed to add sink sender.");
        }
   
        if (ilib_rawchan_finish_sink(&sink) != ILIB_SUCCESS)
            ilib_die("Failed to finish sink.");

        /* Connect master as receiver to sink */
        if (ilib_rawchan_open_receiver(CHANNEL_SERF_DATA_TAG, port_sink) < 0)
            ilib_die("Failed to open ReceivePort.");
    
    } else {
        //else is a serf

        /* open serf's sender port  to sink */
        if (ilib_rawchan_open_sender(CHANNEL_SERF_DATA_TAG + my_local_rank, port_serf_send) < 0)
            ilib_die("Failed to open SendPort.");


    }
    return 0;
}

#endif //is tile is hardware
