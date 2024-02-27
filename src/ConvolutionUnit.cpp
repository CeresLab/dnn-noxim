/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the processing element
 */

#include "ConvolutionUnit.h"

void ConvolutionUnit::rxProcess()
{
    if (reset.read())
    {
        ack_rx_pe.write(0);
        current_level_rx_pe = 0;
        input_buffer_addr = 0;
        state = IDLE;
    }
    else
    {
        if (req_rx_pe.read() == 1 - current_level_rx_pe)
        {
            Flit flit_tmp = flit_rx_pe.read();
            std::cout << "Local id : " << local_id << " CU: => "
                      << "Flit: " << flit_tmp.payload.data << endl;
            if (state == IDLE)
            {
            }
            else if (state == LOAD)
            {
                if (flit_rx_pe.read().payload.data_type == INSTRUCTION)
                {
                    int kernel_size = 0;
                    int operation = 0;
                    input_buffer_addr = 0;
                    weight_buffer_addr = 0;
                }
                else if (flit_rx_pe.read().payload.data_type == INPUT_DATA)
                {
                    input_buffer[input_buffer_addr] = flit_rx_pe.read().payload.data;
                    input_buffer_addr++;
                }
                else if (flit_rx_pe.read().payload.data_type == WEIGHT_DATA)
                {
                    weight_buffer[weight_buffer_addr] = flit_rx_pe.read().payload.data;
                    weight_buffer_addr++;
                }
                if (1)
                {
                    Flit flit_tmp = flit_rx_pe.read();
                    current_level_rx_pe = 1 - current_level_rx_pe; // Negate the old value for Alternating Bit Protocol (ABP)
                }
            }
            else if (state == BUSY)
            {
            }
            current_level_rx_pe = 1 - current_level_rx_pe; // Negate the old value for Alternating Bit Protocol (ABP)
        }

        ack_rx_pe.write(current_level_rx_pe);
    }
}

void ConvolutionUnit::txProcess()
{
    if (reset.read())
    {
        req_tx_pe.write(0);
        current_level_tx_pe = 0;
        transmittedAtPreviousCycle = false;
        i = 0;
    }
    else
    {
        Packet packet;

        if (i == 0 && local_id == 4)
        {
            packet.src_id = local_id;
            packet.dst_id = 15;
            packet.vc_id = 0;
            packet.size = 8;
            packet.flit_left = 8;
            packet.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
            packet_queue.push(packet);
            transmittedAtPreviousCycle = true;
            i++;
        }
        else
            transmittedAtPreviousCycle = false;

        if (ack_tx_pe.read() == current_level_tx_pe)
        {
            if (!packet_queue.empty())
            {
                Flit flit = nextFlit();
                flit_tx_pe->write(flit);                       // Send the generated flit
                current_level_tx_pe = 1 - current_level_tx_pe; // Negate the old value for Alternating Bit Protocol (ABP)
                req_tx_pe.write(current_level_tx_pe);
            }
        }
    }
}

Flit ConvolutionUnit::nextFlit()
{
    Flit flit;
    Packet packet = packet_queue.front();

    flit.src_id = packet.src_id;
    flit.dst_id = packet.dst_id;
    flit.vc_id = packet.vc_id;
    flit.timestamp = packet.timestamp;
    flit.sequence_no = packet.size - packet.flit_left;
    flit.sequence_length = packet.size;
    flit.hop_no = 0;
    flit.payload.data = 5; // test

    if (packet.size == packet.flit_left)
        flit.flit_type = FLIT_TYPE_HEAD;
    else if (packet.flit_left == 1)
        flit.flit_type = FLIT_TYPE_TAIL;
    else
        flit.flit_type = FLIT_TYPE_BODY;

    packet_queue.front().flit_left--;
    if (packet_queue.front().flit_left == 0)
        packet_queue.pop();

    return flit;
}
