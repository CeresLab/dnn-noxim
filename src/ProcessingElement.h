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

    int input_buffer_addr;
    int weight_buffer_addr;
    State state;
    float input_buffer[8 * 8];
    float weight_buffer[8 * 8];
    int i;

    // Functions
    void rxProcess(); // The receiving process
    void txProcess(); // The transmitting process
    Flit nextFlit();

    // Constructor
    SC_CTOR(ProcessingElement)
    {
        SC_METHOD(rxProcess);
        sensitive << reset;
        sensitive << clock.pos();

        SC_METHOD(txProcess);
        sensitive << reset;
        sensitive << clock.pos();
    }
};

#endif
