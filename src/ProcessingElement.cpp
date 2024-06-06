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
    // computeProcess();
    rxProcess();
}

void ProcessingElement::rxProcess()
{
    if (reset.read())
    {
        ack_rx_pe.write(0);
        current_level_rx_pe = 0;

        I_H = 0;
        I_W = 0;
        K_H = 0;
        K_W = 0;
        S = 0;
        OP = -1;
        O_H = 0;
        O_W = 0;
        WB_DST = -1;
        input_buffer_addr = 0;
        weight_buffer_addr = 0;
        psum_buffer_addr = 0;
        output_buffer_addr = 0;
        tranmitted_flit = 0;
        state = LOAD;
        compute_state = 0;
        cycle_cnt = 0;
    }
    else
    {

        // if (state == IDLE)
        // {
        // }
        // else
        if (state == LOAD)
        {
            if (req_rx_pe.read() == 1 - current_level_rx_pe)
            {
                Flit flit_tmp = flit_rx_pe.read();
                if (flit_tmp.data_type == INSTRUCTION && flit_tmp.flit_type != FLIT_TYPE_HEAD)
                {
                    I_H = flit_tmp.ctrl_info.input_height;
                    I_W = flit_tmp.ctrl_info.input_width;
                    K_H = flit_tmp.ctrl_info.kernel_height;
                    K_W = flit_tmp.ctrl_info.kernel_width;
                    S = flit_tmp.ctrl_info.stride;
                    OP = flit_tmp.op_type;
                    ACT = flit_tmp.act_type;
                    O_H = (I_H - K_H) / S + 1;
                    O_W = (I_W - K_W) / S + 1;
                    WB_DST = flit_tmp.ctrl_info.wb_dst;
                    input_buffer_addr = 0;
                    weight_buffer_addr = 0;
                    psum_buffer_addr = 0;
                    output_buffer_addr = 0;
                    tranmitted_flit = 0;
                    cout << "Instruction" << endl;
                }
                else if (flit_tmp.data_type == INPUT_DATA && flit_tmp.flit_type != FLIT_TYPE_HEAD)
                {
                    input_buffer[input_buffer_addr] = flit_tmp.payload.data;
                    input_buffer_addr++;
                    cout << "INPUT_DATA" << endl;
                }
                else if (flit_tmp.data_type == WEIGHT_DATA && flit_tmp.flit_type != FLIT_TYPE_HEAD)
                {
                    weight_buffer[weight_buffer_addr] = flit_tmp.payload.data;
                    weight_buffer_addr++;
                    cout << "WEIGHT_DATA" << endl;
                }
                else if (flit_tmp.data_type == OUTPUT_DATA && flit_tmp.flit_type != FLIT_TYPE_HEAD)
                {
                    cout << flit_tmp.payload.data << " ";
                }

                if (OP == CONV2D || OP == FC)
                {
                    if ((I_H * I_W == input_buffer_addr) && (K_W * K_H == weight_buffer_addr) && input_buffer_addr != 0 && weight_buffer_addr != 0)
                        state = BUSY;
                }
                else if (OP == POOLING)
                {
                    if ((I_H * I_W == input_buffer_addr) && input_buffer_addr != 0)
                        state = BUSY;
                }

                // std::cout << " PE: => "
                //           << "Flit: " << flit_tmp.payload.data
                //           << " Local id : " << local_id << endl;
                // TODO: Update ABP current level at LOAD state only
                current_level_rx_pe = 1 - current_level_rx_pe; // Negate the old value for Alternating Bit Protocol (ABP)
                ack_rx_pe.write(current_level_rx_pe);
            }
        }
        else if (state == BUSY)
        {
            computeProcess();
        }
        else if (state == WB)
        {
            writeBack();
            state = LOAD;

            I_H = 0;
            I_W = 0;
            K_H = 0;
            K_W = 0;
            S = 0;
            OP = -1;
            O_H = 0;
            O_W = 0;
            WB_DST = -1;
            input_buffer_addr = 0;
            weight_buffer_addr = 0;
            psum_buffer_addr = 0;
            output_buffer_addr = 0;
        }
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
        makeP_state = 0;
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
        }
    }
}

void ProcessingElement::computeProcess()
{
    if (state == BUSY)
    {
        cout << " BUSY" << endl;
        if (OP == CONV2D)
        {
            if (compute_state == 0)
            {
                Convolution2D(cycle_cnt);
                cout << "cycle_cnt: " << cycle_cnt << endl;
                compute_state = 1;
            }
            else if (compute_state == 1)
            {
                if (cycle_cnt == 0)
                {
                    compute_state = 0;
                    if (WB_DST < 0)
                    {
                        state = LOAD;
                        cout << "PE: " << local_id << " Convolution2D done!" << endl;
                        cout << "TO Load state !!!!" << endl;
                        I_H = 0;
                        I_W = 0;
                        K_H = 0;
                        K_W = 0;
                        S = 0;
                        OP = -1;
                        O_H = 0;
                        O_W = 0;
                        WB_DST = -1;
                        input_buffer_addr = 0;
                        weight_buffer_addr = 0;
                        psum_buffer_addr = 0;
                        output_buffer_addr = 0;
                        tranmitted_flit = 0;
                    }
                    else
                    {
                        state = WB;
                        cout << "PE: " << local_id << " Convolution2D done!" << endl;
                        cout << "TO Write back state !!!!" << endl;
                    }
                }
                else
                {
                    cycle_cnt--;
                    LOG << " computing iteration: " << cycle_cnt << endl;
                }
            }
        }
        else if (OP == FC)
        {
            FullyConnected();
            if (WB_DST < 0)
            {
                state = LOAD;
                cout << "TO Load state !!!!" << endl;

                I_H = 0;
                I_W = 0;
                K_H = 0;
                K_W = 0;
                S = 0;
                OP = -1;
                O_H = 0;
                O_W = 0;
                WB_DST = -1;
                input_buffer_addr = 0;
                weight_buffer_addr = 0;
                psum_buffer_addr = 0;
                output_buffer_addr = 0;
                tranmitted_flit = 0;
            }
            else
            {
                state = WB;
                cout << "TO Write back state !!!!" << endl;
            }
        }
        else if (OP == POOLING)
        {
            Pooling();
            if (WB_DST < 0)
            {
                state = LOAD;
                cout << "TO Load state !!!!" << endl;

                I_H = 0;
                I_W = 0;
                K_H = 0;
                K_W = 0;
                S = 0;
                OP = -1;
                O_H = 0;
                O_W = 0;
                WB_DST = -1;
                input_buffer_addr = 0;
                weight_buffer_addr = 0;
                psum_buffer_addr = 0;
                output_buffer_addr = 0;
                tranmitted_flit = 0;
            }
            else
            {
                state = WB;
                cout << "TO Write back state !!!!" << endl;
            }
        }
    }
}

void ProcessingElement::writeBack()
{
    Transaction wb_trans; /*Add: For pe write back data to memory - 2024/05/14*/

    wb_trans.dst = (WB_DST * GlobalParams::mesh_dim_x) + (GlobalParams::mesh_dim_x - 1);
    wb_trans.dst_type = 1;
    wb_trans.src = local_id;
    wb_trans.src_type = 0;
    wb_trans.ctrl_info.output_height = O_H;
    wb_trans.ctrl_info.output_width = O_W;
    wb_trans.consumed_flag = 0;

    for (int i = 0; i < O_H; i++)
    {
        for (int j = 0; j < O_W; j++)
        {
            wb_trans.ofmap.push_back(O_M[i][j]);
            cout << "OM: " << wb_trans.ofmap[i * O_W + j] << " ";
        }
        cout << endl;
    }
    cout << endl;

    transaction_table->loadPETransaction(local_id, wb_trans);
}

void ProcessingElement::Convolution2D(int &cycle_cnt)
{
    cout << "============================" << endl;
    cout << "|      Convolution2D       |" << endl;
    cout << "============================" << endl;
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
                    cycle_cnt++;
                }
            }
            //*====================================
            //*Activation fuction
            if (ACT == RELU)
            {
                O_M[i][j] = ReLU(O_M[i][j]);
                cout << "OM: " << O_M[i][j] << " ";
            }
            else
                cout << "OM: " << O_M[i][j] << " ";
            //*====================================
        }
        cout << endl;
    }
    cout << "============================" << endl;
}

void ProcessingElement::FullyConnected()
{
    cout << "============================" << endl;
    cout << "|      Fully-Connected     |" << endl;
    cout << "============================" << endl;
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
    //*====================================
    //*Activation fuction
    if (ACT == RELU)
    {
        O_M[0][0] = ReLU(O_M[0][0]);
        cout << "OM: " << O_M[0][0] << endl;
    }
    else
        cout << "OM: " << O_M[0][0] << endl;
    //*====================================

    cout << "============================" << endl;
}

void ProcessingElement::Pooling()
{
    cout << "============================" << endl;
    cout << "|          Pooling         |" << endl;
    cout << "============================" << endl;
    int I_M[I_H][I_W];

    for (int i = 0; i < I_H; i++)
        for (int j = 0; j < I_W; j++)
            I_M[i][j] = input_buffer[i * I_H + j];

    for (int i = 0; i < O_H; i++)
    {
        for (int j = 0; j < O_W; j++)
        {
            int MAX = -1;
            for (int m = 0; m < K_H; m++)
            {
                for (int n = 0; n < K_W; n++)
                {
                    if (I_M[i * S + m][j * S + n] > MAX)
                        MAX = I_M[i * S + m][j * S + n];
                }
            }
            O_M[i][j] = MAX;
            cout << "OM: " << O_M[i][j] << " ";
        }
        cout << endl;
    }
    cout << "============================" << endl;
}

int ProcessingElement::ReLU(int a)
{
    int b;
    b = (a > 0) ? a : 0;
    return b;
}

void ProcessingElement::computingCycle()
{
}