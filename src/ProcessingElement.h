/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the processing element
 */

#ifndef __NOXIMPROCESSINGELEMENT_H__
#define __NOXIMPROCESSINGELEMENT_H__

#include <queue>
#include <systemc.h>

#include "DataStructs.h"
#include "Buffer.h"
#include "GlobalTrafficTable.h"
#include "Utils.h"
#include "GlobalParams.h"

using namespace std;

SC_MODULE(ProcessingElement)
{

    // I/O Ports
    sc_in_clk clock;   // The input clock for the PE
    sc_in<bool> reset; // The reset signal for the PE

    sc_in<Flit> flit_rx_pe; // The input channel
    sc_in<bool> req_rx_pe;  // The request associated with the input channel
    sc_out<bool> ack_rx_pe; // The outgoing ack signal associated with the input channel
    sc_out<TBufferFullStatus> buffer_full_status_rx_pe;

    sc_out<Flit> flit_tx_pe; // The output channel
    sc_out<bool> req_tx_pe;  // The request associated with the output channel
    sc_in<bool> ack_tx_pe;   // The outgoing ack signal associated with the output channel
    sc_in<TBufferFullStatus> buffer_full_status_tx_pe;

    // sc_in<int> free_slots_neighbor;

    // Registers
    int local_id;                    // Unique identification number
    bool current_level_rx_pe;        // Current level for Alternating Bit Protocol (ABP)
    bool current_level_tx_pe;        // Current level for Alternating Bit Protocol (ABP)
    PeBufferBank pebuffer;           // buffer[direction][virtual_channel]
    queue<Packet> packet_queue;      // Local queue of packets
    bool transmittedAtPreviousCycle; // Used for distributions with memory

    /*===================================================================================*/
    int input_buffer_addr;
    int weight_buffer_addr;
    int psum_buffer_addr;
    int output_buffer_addr;
    int tranmitted_flit;
    int I_W;
    int I_H;
    int K_W;
    int K_H;
    int S;
    int OP;
    int ACT;
    int O_H;
    int O_W;
    int O_M[OUTPUT_BUFFER][OUTPUT_BUFFER];
    int WB_DST;
    State state;
    sc_uint<32> input_buffer[INPUT_BUFFER * INPUT_BUFFER];
    sc_uint<32> weight_buffer[WEIGHT_BUFFER * WEIGHT_BUFFER];
    sc_uint<32> psum_buffer[8 * 8];
    sc_uint<32> output_buffer[OUTPUT_BUFFER * OUTPUT_BUFFER];
    int i;
    int makeP_state;
    int compute_state;
    int cycle_cnt;

    /*===================================================================================*/

    // Functions
    void process();
    void rxProcess();      // The receiving process
    void txProcess();      // The transmitting process
    void computeProcess(); // The transmitting process
    void writeBack();
    void computingCycle();
    /*===================================================================================*/
    void Convolution2D(int &cycle_cnt);
    void FullyConnected();
    void Pooling();
    void DepthWise();
    void PointWise();
    int ReLU(int a);
    /*===================================================================================*/

    GlobalTrafficTable *transaction_table; //* Reference to the Global transaction Table
    Flit nextFlit();

    // Constructor
    SC_CTOR(ProcessingElement)
    {
        // SC_METHOD(rxProcess);
        // sensitive << reset;
        // sensitive << clock.pos();

        // SC_METHOD(txProcess);
        // sensitive << reset;
        // sensitive << clock.pos();

        SC_METHOD(process);
        sensitive << reset;
        sensitive << clock.pos();
    }
};

#endif
