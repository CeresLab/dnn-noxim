/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the top-level of Noxim
 */

#ifndef _DATASTRUCS_H__
#define _DATASTRUCS_H__

#include <systemc.h>
#include "GlobalParams.h"

// Coord -- XY coordinates type of the Tile inside the Mesh
class Coord
{
public:
    int x; // X coordinate
    int y; // Y coordinate

    inline bool operator==(const Coord &coord) const
    {
        return (coord.x == x && coord.y == y);
    }
};

// FlitType -- Flit type enumeration
enum FlitType
{
    FLIT_TYPE_HEAD,
    FLIT_TYPE_BODY,
    FLIT_TYPE_TAIL
};

// DataType -- Flit type enumeration
enum DataType
{
    INSTRUCTION,
    INPUT_DATA,
    WEIGHT_DATA,
    OUTPUT_DATA
};

enum OperationType
{
    CONV2D,
    FC,
    POOLING
};

enum ActivationType
{
    RELU
};

struct ControlInfo
{
    int kernel_width;
    int kernel_height;
    int stride;
    int input_width;
    int input_height;
    int output_width;
    int output_height;
    int wb_dst;
};

// Payload -- Payload definition
struct Payload
{
    sc_uint<32> data; // Bus for the data to be exchanged
    inline bool operator==(const Payload &payload) const
    {
        return (payload.data == data);
    }
};

enum State
{
    IDLE,
    LOAD,
    BUSY,
    WB
};

// Packet -- Packet definition
struct Packet
{
    int src_type;
    int src_id;
    int dst_type;
    int dst_id;
    int vc_id;
    double timestamp; // SC timestamp at packet generation
    int size;
    int flit_left; // Number of remaining flits inside the packet
    bool use_low_voltage_path;

    // Constructors
    Packet() {}

    Packet(const int st, const int s, const int dt, const int d, const int vc, const double ts, const int sz)
    {
        make(st, s, dt, d, vc, ts, sz);
    }

    void make(const int st, const int s, const int dt, const int d, const int vc, const double ts, const int sz)
    {
        src_type = st;
        src_id = s;
        dst_type = dt;
        dst_id = d;
        vc_id = vc;
        timestamp = ts;
        size = sz;
        flit_left = sz;
        use_low_voltage_path = false;
    }
};

// RouteData -- data required to perform routing
struct RouteData
{
    int current_id;
    int src_type;
    int src_id;
    int dst_type;
    int dst_id;
    int dir_in; // direction from which the packet comes from
    int vc_id;
};

struct ChannelStatus
{
    int free_slots; // occupied buffer slots
    bool available; //
    inline bool operator==(const ChannelStatus &bs) const
    {
        return (free_slots == bs.free_slots && available == bs.available);
    };
};

// NoP_data -- NoP Data definition
struct NoP_data
{
    int sender_id;
    ChannelStatus channel_status_neighbor[DIRECTIONS];

    inline bool operator==(const NoP_data &nop_data) const
    {
        return (sender_id == nop_data.sender_id &&
                nop_data.channel_status_neighbor[0] ==
                    channel_status_neighbor[0] &&
                nop_data.channel_status_neighbor[1] ==
                    channel_status_neighbor[1] &&
                nop_data.channel_status_neighbor[2] ==
                    channel_status_neighbor[2] &&
                nop_data.channel_status_neighbor[3] ==
                    channel_status_neighbor[3]);
    };
};

struct TBufferFullStatus
{
    TBufferFullStatus()
    {
        for (int i = 0; i < MAX_VIRTUAL_CHANNELS; i++)
            mask[i] = false;
    };
    inline bool operator==(const TBufferFullStatus &bfs) const
    {
        for (int i = 0; i < MAX_VIRTUAL_CHANNELS; i++)
            if (mask[i] != bfs.mask[i])
                return false;
        return true;
    };

    bool mask[MAX_VIRTUAL_CHANNELS];
};

// Flit -- Flit definition
struct Flit
{
    int src_type;
    int src_id;
    int dst_type;
    int dst_id;
    int vc_id;          // Virtual Channel
    FlitType flit_type; // The flit type (FLIT_TYPE_HEAD, FLIT_TYPE_BODY, FLIT_TYPE_TAIL)
    int sequence_no;    // The sequence number of the flit inside the packet
    int sequence_length;
    Payload payload;  // Optional payload
    double timestamp; // Unix timestamp at packet generation
    int hop_no;       // Current number of hops from source to destination
    bool use_low_voltage_path;

    int data_type;
    ControlInfo ctrl_info;
    int op_type;
    int act_type;

    inline bool operator==(const Flit &flit) const
    {
        return (flit.src_type == src_type && flit.src_id == src_id && flit.dst_type == dst_type && flit.dst_id == dst_id && flit.flit_type == flit_type && flit.vc_id == vc_id && flit.sequence_no == sequence_no && flit.sequence_length == sequence_length && flit.payload == payload && flit.timestamp == timestamp && flit.hop_no == hop_no && flit.use_low_voltage_path == use_low_voltage_path);
    }
};

struct PayloadData
{
    Payload payload; // Optional payload
};

typedef struct
{
    string label;
    double value;
} PowerBreakdownEntry;

enum
{
    BUFFER_PUSH_PWR_D,
    BUFFER_POP_PWR_D,
    BUFFER_FRONT_PWR_D,
    ROUTING_PWR_D,
    SELECTION_PWR_D,
    CROSSBAR_PWR_D,
    LINK_R2R_PWR_D,
    NI_PWR_D,
    NO_BREAKDOWN_ENTRIES_D
};

enum
{
    TRANSCEIVER_RX_PWR_BIASING,
    TRANSCEIVER_TX_PWR_BIASING,
    BUFFER_ROUTER_PWR_S,
    ROUTING_PWR_S,
    SELECTION_PWR_S,
    CROSSBAR_PWR_S,
    NI_PWR_S,
    TRANSCEIVER_RX_PWR_S,
    TRANSCEIVER_TX_PWR_S,
    NO_BREAKDOWN_ENTRIES_S
};

typedef struct
{
    int size;
    PowerBreakdownEntry breakdown[NO_BREAKDOWN_ENTRIES_D + NO_BREAKDOWN_ENTRIES_S];
} PowerBreakdown;

#endif
