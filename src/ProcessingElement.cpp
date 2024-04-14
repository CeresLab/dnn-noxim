/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the processing element
 */

#include "ProcessingElement.h"

void ProcessingElement::process()
{
    txProcess();
    computeProcess();
    rxProcess();
}

void ProcessingElement::rxProcess()
{
    if (reset.read())
    {
        ack_rx_pe.write(0);
        current_level_rx_pe = 0;
        input_buffer_addr = 0;
        psum_buffer_addr = 0;
        output_buffer_addr = 0;
        state = LOAD;
    }
    else
    {
        if (req_rx_pe.read() == 1 - current_level_rx_pe)
        {
            Flit flit_tmp = flit_rx_pe.read();
            std::cout << " PE: => "
                      << "Flit: " << flit_tmp.payload.data
                      << " Local id : " << local_id << endl;
            // if (state == IDLE)
            // {
            // }
            // else
            if (state == LOAD)
            {
                if (flit_tmp.data_type == INSTRUCTION)
                {
                    I_H = flit_tmp.ctrl_info.input_height;
                    I_W = flit_tmp.ctrl_info.input_width;
                    K_H = flit_tmp.ctrl_info.kernel_height;
                    K_W = flit_tmp.ctrl_info.kernel_width;
                    S = flit_tmp.ctrl_info.stride;
                    OP = flit_tmp.op_type;
                    O_H = (I_H - K_H) / S + 1;
                    O_W = (I_W - K_W) / S + 1;
                    WB_DST = flit_tmp.ctrl_info.wb_dst;
                    input_buffer_addr = 0;
                    weight_buffer_addr = 0;
                    psum_buffer_addr = 0;
                    output_buffer_addr = 0;
                    flit_num = 0;
                    cout << "Instruction" << endl;
                }
                else if (flit_tmp.data_type == INPUT_DATA)
                {
                    input_buffer[input_buffer_addr] = flit_tmp.payload.data;
                    input_buffer_addr++;
                }
                else if (flit_tmp.data_type == WEIGHT_DATA)
                {
                    weight_buffer[weight_buffer_addr] = flit_tmp.payload.data;
                    weight_buffer_addr++;
                }
                else if (flit_tmp.data_type == OUTPUT_DATA)
                {
                    cout << flit_tmp.payload.data << " ";
                }

                if ((I_H * I_W == input_buffer_addr) && (K_W * K_H == weight_buffer_addr))
                    state = BUSY;
            }
            else if (state == BUSY)
            {
                if (OP == CONV2D)
                {
                    Convolution2D();
                    if (WB_DST < 0)
                        state = LOAD;
                    else
                        state = WB;
                }
                else if (OP == FC)
                {
                    FullyConnected();
                    if (WB_DST < 0)
                        state = LOAD;
                    else
                        state = WB;
                }
                else if (OP == POOLING)
                {
                    FullyConnected();
                    if (WB_DST < 0)
                        state = LOAD;
                    else
                        state = WB;
                }
            }

            current_level_rx_pe = 1 - current_level_rx_pe; // Negate the old value for Alternating Bit Protocol (ABP)
        }

        ack_rx_pe.write(current_level_rx_pe);
    }
}

void ProcessingElement::txProcess()
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
        // Packet packet;

        // if (i == 0 && local_id == 4)
        // {
        //     packet.src_id = local_id;
        //     packet.dst_id = 15;
        //     packet.vc_id = 0;
        //     packet.size = 8;
        //     packet.flit_left = 8;
        //     packet.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
        //     packet_queue.push(packet);
        //     transmittedAtPreviousCycle = true;
        //     i++;
        // }
        // else
        //     transmittedAtPreviousCycle = false;

        if (ack_tx_pe.read() == current_level_tx_pe)
        {
            if (state == WB)
            {
                flit_num = O_W * O_H;
                if (flit_num > 0)
                {
                    // Packet packet;
                    // double now = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
                    // int packet_size;
                    // if (flit_num / 8 != 0)
                    //     packet_size = 8;
                    // else
                    //     packet_size = flit_num;
                    // packet.make(local_id, WB_DST, 0, now, packet_size);

                    // for (int i = 0; i < packet_size; i++)
                    //     packet.data.push_back(O_M[flit_num / O_H][flit_num % O_W]);
                    Flit flit;
                    flit.src_id = local_id;
                    flit.dst_id = WB_DST;
                    flit.payload.data = O_M[flit_num / O_H][flit_num % O_W]; // test
                    flit_tx_pe->write(flit);                                 // Send the generated flit
                    current_level_tx_pe = 1 - current_level_tx_pe;           // Negate the old value for Alternating Bit Protocol (ABP)
                    req_tx_pe.write(current_level_tx_pe);
                    flit_num++;
                }
                else
                    state == LOAD;
            }
        }
    }
}

void ProcessingElement::computeProcess()
{
    if (state == BUSY)
    {
        if (OP == CONV2D)
        {
            Convolution2D();
            if (WB_DST < 0)
                state = WB;
            else
                state = LOAD;
        }
        else if (OP == FC)
        {
            FullyConnected();
            if (WB_DST < 0)
                state = WB;
            else
                state = LOAD;
        }
        else if (OP == POOLING)
        {
            FullyConnected();
            if (WB_DST < 0)
                state = WB;
            else
                state = LOAD;
        }
    }
}

Flit ProcessingElement::nextFlit()
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

void ProcessingElement::Convolution2D()
{
    int I_M[I_H][I_W];
    int W_M[K_H][K_W];

    for (int i = 0; i < I_H; i++)
        for (int j = 0; j < I_W; j++)
            I_M[i][j] = input_buffer[i * I_H + j];
    for (int i = 0; i < K_H; i++)
        for (int j = 0; j < K_W; j++)
            W_M[i][j] = weight_buffer[i * K_H + j];

    for (int i = 0; i < O_H; i++)
    {
        for (int j = 0; j < O_W; j++)
        {
            for (int m = 0; m < K_H; m++)
            {
                for (int n = 0; n < K_W; n++)
                {
                    O_M[i][j] += I_M[i * S + m][j * S + n] * W_M[m][n];
                }
            }
        }
    }
}

void ProcessingElement::FullyConnected()
{
    int I_M[I_H * I_W];
    int W_M[K_H * K_W];

    for (int i = 0; i < I_H * I_W; i++)
        I_M[i] = input_buffer[i];

    for (int j = 0; j < K_H * K_W; j++)
        W_M[j] = weight_buffer[j];

    for (int k = 0; k < K_H * K_W; k++)
    {
        O_M[0][0] += I_M[k] * W_M[k];
    }
}

void ProcessingElement::Pooling()
{
}

// void ProcessingElement::DepthWise()
// {
// }
// void ProcessingElement::PointWise()
// {
// }

void ProcessingElement::ReLU()
{
}

void ProcessingElement::Sigmoid()
{
}