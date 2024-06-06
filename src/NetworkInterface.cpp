/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the processing element
 */

#include "NetworkInterface.h"

int NetworkInterface::randInt(int min, int max)
{
    return min +
           (int)((double)(max - min + 1) * rand() / (RAND_MAX + 1.0));
}

void NetworkInterface::rxProcess()
{
    if (reset.read())
    {
        ack_rx.write(0);
        current_level_rx = 0;
    }
    else
    {
        if (req_rx.read() == 1 - current_level_rx)
        {
            if (!pebuffer.IsFull())
            {
                Flit flit_tmp = flit_rx.read();
                // std::cout << " NI RX: => "
                //           << "Flit: " << flit_tmp.payload.data << endl;
                // std::cout << "Flit: " << flit_tmp.payload.data << endl;
                pebuffer.Push(flit_tmp);
                current_level_rx = 1 - current_level_rx; // Negate the old value for Alternating Bit Protocol (ABP)

                //* For latency information
                if (last_received_flit_time < sc_time_stamp().to_double() / GlobalParams::clock_period_ps)
                    last_received_flit_time = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
            }
        }
        ack_rx.write(current_level_rx);
    }
}

void NetworkInterface::txProcess()
{
    if (reset.read())
    {
        req_tx.write(0);
        current_level_tx = 0;
        transmittedAtPreviousCycle = false;
        ifmap_data.clear();
        weight_data.clear();
        ifm_data_cnt = remain_ifm_size = 0;
        w_data_cnt = remain_w_size = 0;
        ofm_data_cnt = remain_ofm_size = 0;
        makeP_state = INSTRUCTION;
        makeF_state = INSTRUCTION;
        remaining_traffic = 0;
    }
    else
    {
        Packet packet;
        if (canShot(packet))
        {
            packet_queue.push(packet);
            transmittedAtPreviousCycle = true;
        }
        else
            transmittedAtPreviousCycle = false;

        if (ack_tx.read() == current_level_tx)
        {
            if (!packet_queue.empty())
            {
                Flit flit = nextFlit();                  // Generate a new flit
                flit_tx->write(flit);                    // Send the generated flit
                current_level_tx = 1 - current_level_tx; // Negate the old value for Alternating Bit Protocol (ABP)
                req_tx.write(current_level_tx);

                if (flit.flit_type == FLIT_TYPE_HEAD)
                {
                    cout << "\nHEAD: (" << flit.src_id << "->" << flit.dst_id << ")\n";
                }
                else if (flit.flit_type == FLIT_TYPE_BODY)
                {
                    cout << "BODY: <" << flit.data_type << "> @"
                         << flit.payload.data << "@\n";
                }
                else
                {
                    cout << "TILE: <" << flit.data_type << "> @"
                         << flit.payload.data << "@\n";
                }
            }
        }
        // if (ack_tx.read() == current_level_tx)
        // {
        //     if (!pebuffer.IsEmpty())
        //     {
        //         Flit flit = pebuffer.Front();            // Generate a new flit
        //         flit_tx->write(flit);                    // Send the generated flit
        //         current_level_tx = 1 - current_level_tx; // Negate the old value for Alternating Bit Protocol (ABP)
        //         req_tx.write(current_level_tx);
        //     }
        // }
    }
}

void NetworkInterface::rxPeProcess()
{
    if (reset.read())
    {
        ack_rx_pe.write(0);
        current_level_rx_pe = 0;
    }
    else
    {
        if (req_rx_pe.read() == 1 - current_level_rx_pe)
        {
            if (!pebuffer.IsFull())
            {
                Flit flit_tmp = flit_rx_pe.read();
                // std::cout << "Flit: " << flit_tmp.payload.data << endl;
                pebuffer.Push(flit_tmp);
                current_level_rx_pe = 1 - current_level_rx_pe; // Negate the old value for Alternating Bit Protocol (ABP)
            }
        }
        ack_rx_pe.write(current_level_rx_pe);
    }
}

void NetworkInterface::txPeProcess()
{
    if (reset.read())
    {
        req_tx_pe.write(0);
        current_level_tx_pe = 0;
        transmittedAtPreviousCycle = false;
    }
    else
    {
        if (ack_tx_pe.read() == current_level_tx_pe)
        {
            if (!pebuffer.IsEmpty())
            {
                if (current_level_tx_pe == ack_tx_pe.read())
                {
                    Flit flit_tmp = pebuffer.Front(); // Generate a new flit
                    // std::cout << " NI TXPE: => "
                    //           << "Flit: " << flit_tmp.payload.data << endl;
                    flit_tx_pe.write(flit_tmp);                    // Send the generated flit
                    current_level_tx_pe = 1 - current_level_tx_pe; // Negate the old value for Alternating Bit Protocol (ABP)
                    req_tx_pe.write(current_level_tx_pe);
                    pebuffer.Pop();
                }
            }
        }
    }
}

Flit NetworkInterface::nextFlit()
{
    Flit flit;
    Packet packet = packet_queue.front();

    flit.src_type = packet.src_type;
    flit.src_id = packet.src_id;
    flit.dst_type = packet.dst_type;
    flit.dst_id = packet.dst_id;
    flit.vc_id = packet.vc_id;
    flit.timestamp = packet.timestamp;
    flit.sequence_no = packet.size - packet.flit_left;
    flit.sequence_length = packet.size;
    flit.hop_no = 0;

    if (makeF_state == INSTRUCTION && packet.size != packet.flit_left)
    {
        flit.ctrl_info = transaction_ctrl;
        flit.op_type = transaction_opt;
        flit.act_type = transaction_act;
        flit.payload.data = 0;
        flit.data_type = INSTRUCTION;
        if (ofmap_data.size() == 0)
            makeF_state = INPUT_DATA;
        else
            makeF_state = OUTPUT_DATA;
    }
    else if (makeF_state == INPUT_DATA && packet.size != packet.flit_left)
    {
        flit.payload.data = ifmap_data[ifmap_data.size() - remain_ifm_size];
        remain_ifm_size -= 1;
        flit.data_type = INPUT_DATA;
        if (remain_ifm_size == 0)
            makeF_state = WEIGHT_DATA;
    }
    else if (makeF_state == WEIGHT_DATA && packet.size != packet.flit_left)
    {
        flit.payload.data = weight_data[weight_data.size() - remain_w_size];
        remain_w_size -= 1;
        flit.data_type = WEIGHT_DATA;
        if (remain_w_size == 0)
            makeF_state = INSTRUCTION;
    }
    else if (makeF_state == OUTPUT_DATA && packet.size != packet.flit_left)
    {
        flit.payload.data = ofmap_data[ofmap_data.size() - remain_ofm_size];
        remain_ofm_size -= 1;
        flit.data_type = OUTPUT_DATA;
        if (remain_ofm_size == 0)
            makeF_state = INSTRUCTION; // TODO: Confirm WB state along with PE injected traffic
    }

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

bool NetworkInterface::canShot(Packet &packet)
{
    // assert(false);
    if (never_transmit)
        return false;

    // if(local_id!=16) return false;
    /* DEADLOCK TEST
    double current_time = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

    if (current_time >= 4100)
    {
        //if (current_time==3500)
                //cout << name() << " IN CODA " << packet_queue.size() << endl;
        return false;
    }
    */

    // #ifdef DEADLOCK_AVOIDANCE
    //     if (local_id % 2 == 0)
    //         return false;
    // #endif

    bool shot = false;
    // double threshold;

    double now = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

    // if (GlobalParams::traffic_distribution != TRAFFIC_TABLE_BASED)
    // {
    //     if (!transmittedAtPreviousCycle)
    //         threshold = GlobalParams::packet_injection_rate;
    //     else
    //         threshold = GlobalParams::probability_of_retransmission;

    //     shot = (((double)rand()) / RAND_MAX < threshold);
    //     if (shot)
    //     {
    // if (GlobalParams::traffic_distribution == TRAFFIC_RANDOM)
    //     packet = trafficRandom();
    // else if (GlobalParams::traffic_distribution == TRAFFIC_TRANSPOSE1)
    //     packet = trafficTranspose1();
    // else if (GlobalParams::traffic_distribution == TRAFFIC_TRANSPOSE2)
    //     packet = trafficTranspose2();
    // else if (GlobalParams::traffic_distribution == TRAFFIC_BIT_REVERSAL)
    //     packet = trafficBitReversal();
    // else if (GlobalParams::traffic_distribution == TRAFFIC_SHUFFLE)
    //     packet = trafficShuffle();
    // else if (GlobalParams::traffic_distribution == TRAFFIC_BUTTERFLY)
    //     packet = trafficButterfly();
    // else if (GlobalParams::traffic_distribution == TRAFFIC_ULOCAL)
    //     packet = trafficULocal();
    // else
    // {
    //     cout << "Invalid traffic distribution: " << GlobalParams::traffic_distribution << endl;
    //     exit(-1);
    // }
    //     }
    // }

    if (GlobalParams::traffic_distribution == TRAFFIC_TABLE_BASED)
    {
        if (never_transmit)
            return false;

        // bool use_pir = (transmittedAtPreviousCycle == false);
        // vector < pair < int, double > > dst_prob;
        // double threshold =
        //     traffic_table->getCumulativePirPor(local_id, (int) now, use_pir, dst_prob);

        // double prob = (double) rand() / RAND_MAX;
        // shot = (prob < threshold);
        // if (shot) {
        //     for (unsigned int i = 0; i < dst_prob.size(); i++) {
        //         if (prob < dst_prob[i].second) {
        //             int vc = randInt(0,GlobalParams::n_virtual_channels-1);
        //             packet.make(local_id, dst_prob[i].first, vc, now, getRandomSize());
        //             // cout << "Make Success >>> Src: " << local_id << " Dst: " << dst_prob[i].first << endl;
        //             break;
        //         }
        //     }
        // }

        int dstfromTable;
        int remaining_traffic = traffic_table->getPacketinCommunication(local_id, dstfromTable);
        // cout << " / Remaining Traffic in PE: " << remaining_traffic << endl;
        if (remaining_traffic > 0)
        {
            int vc = randInt(0, GlobalParams::n_virtual_channels - 1);
            packet.make(0, local_id, transaction_dst_type, dstfromTable, vc, now, getRandomSize());
            shot = true;
        }
        else
            shot = false;
    }

    if (GlobalParams::traffic_distribution == TRANSACTION_BASED)
    {
        if (never_transmit)
            return false;

        //* Get the next transaction while finishing previous traffic
        // if (ifm_data_cnt == 0 && w_data_cnt == 0 && remain_ifm_size == 0 && remain_w_size == 0)
        // {
        //     remaining_traffic = transaction_table->getTransactionInfo(0, local_id, transaction_dst_type, transaction_dst, transaction_opt,
        //                                                               transaction_act, transaction_ctrl,
        //                                                               ifmap_data, weight_data);
        //     ifm_data_cnt = ifmap_data.size();
        //     remain_ifm_size = ifmap_data.size();
        //     w_data_cnt = weight_data.size();
        //     remain_w_size = weight_data.size();
        //     // cout << "ifm cnt: " << ifm_data_cnt << " " << remain_ifm_size << endl;
        // }

        //* Get the write back transaction while finishing computation
        if (ofm_data_cnt == 0 && remain_ofm_size == 0)
        {
            remaining_traffic = transaction_table->getPETransactionInfo(0, local_id, transaction_dst_type, transaction_dst, transaction_opt,
                                                                        transaction_act, transaction_ctrl,
                                                                        ifmap_data, weight_data, ofmap_data);
            // ifm_data_cnt = ifmap_data.size();
            // remain_ifm_size = ifmap_data.size();
            // w_data_cnt = weight_data.size();
            // remain_w_size = weight_data.size();
            ofm_data_cnt = ofmap_data.size();
            remain_ofm_size = ofmap_data.size();
            // cout << "ifm cnt: " << ifm_data_cnt << " " << remain_ifm_size << endl;
            // cout << "WRITE BACK" << endl;
        }

        //* Make the packet in different state and size
        if (remaining_traffic > 0)
        {
            int vc = randInt(0, GlobalParams::n_virtual_channels - 1);
            if (makeP_state == INSTRUCTION)
            {
                packet.make(0, local_id, 1, transaction_dst, vc, now, 2);
                if (ofmap_data.size() == 0)
                    makeP_state = INPUT_DATA;
                else
                    makeP_state = OUTPUT_DATA;

                // cout << "INSTRUCTION PP\n";
            }
            else if (makeP_state == INPUT_DATA)
            {
                if (ifm_data_cnt - 8 > 0)
                {
                    packet.make(0, local_id, 1, transaction_dst, vc, now, 8 + 1);
                    ifm_data_cnt -= 8;
                }
                else
                {
                    packet.make(0, local_id, 1, transaction_dst, vc, now, ifm_data_cnt + 1);
                    ifm_data_cnt = 0;
                    makeP_state = WEIGHT_DATA;
                }
                // cout << "INPUT_DATA PP\n";
            }
            else if (makeP_state == WEIGHT_DATA)
            {
                if (w_data_cnt - 8 > 0)
                {
                    packet.make(0, local_id, 1, transaction_dst, vc, now, 8 + 1);
                    w_data_cnt -= 8;
                }
                else
                {
                    packet.make(0, local_id, 1, transaction_dst, vc, now, w_data_cnt + 1);
                    w_data_cnt = 0;
                    makeP_state = INSTRUCTION;
                }
                // cout << "WEIGHT_DATA PP\n";
            }
            else if (makeP_state == OUTPUT_DATA)
            {
                if (ofm_data_cnt - 8 > 0)
                {
                    packet.make(0, local_id, 1, transaction_dst, vc, now, 8 + 1);
                    ofm_data_cnt -= 8;
                }
                else
                {
                    packet.make(0, local_id, 1, transaction_dst, vc, now, ofm_data_cnt + 1);
                    ofm_data_cnt = 0;
                    makeP_state = INSTRUCTION; // TODO: Confirm WB state along with PE injected traffic
                }
                // cout << "WEIGHT_DATA PP\n";
            }
            shot = true;

            if (ifm_data_cnt == 0 && w_data_cnt == 0 && ofm_data_cnt == 0)
                remaining_traffic = -1;
        }
        else
        {
            shot = false;
        }
    }

    return shot;
}

int NetworkInterface::getRandomSize()
{
    return randInt(GlobalParams::min_packet_size,
                   GlobalParams::max_packet_size);
}

unsigned int NetworkInterface::getQueueSize() const
{
    return packet_queue.size();
}
