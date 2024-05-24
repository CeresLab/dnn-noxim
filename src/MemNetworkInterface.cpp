/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the processing element
 */

#include "MemNetworkInterface.h"

int MemNetworkInterface::randInt(int min, int max)
{
    return min +
           (int)((double)(max - min + 1) * rand() / (RAND_MAX + 1.0));
}

void MemNetworkInterface::rxProcess()
{
    if (reset.read())
    {
        ack_rx.write(0);
        current_level_rx = 0;
        ofm_cnt = 0;
        receivedPackets = 0;
    }
    else
    {
        if (req_rx.read() == 1 - current_level_rx)
        {
            Flit flit_tmp = flit_rx.read();
            current_level_rx = 1 - current_level_rx; // Negate the old value for Alternating Bit Protocol (ABP)
            if (flit_tmp.data_type == INSTRUCTION && flit_tmp.flit_type != FLIT_TYPE_HEAD)
            {
                cout << " ------------------------------------------------------------------ " << endl;
                cout << " Memory: " << memory_id << endl;
                cout << " Src_type: " << flit_tmp.src_type << " Src_id: " << flit_tmp.src_id << endl;
                cout << " O_H = " << flit_tmp.ctrl_info.output_height << " O_W = " << flit_tmp.ctrl_info.output_width << endl;
                ofm_cnt = 0;
            }
            else if (flit_tmp.data_type == OUTPUT_DATA && flit_tmp.flit_type != FLIT_TYPE_HEAD)
            {
                cout << " OFM[" << ofm_cnt << "] " << flit_tmp.payload.data << endl;
                ofm_cnt++;
            }

            if (flit_tmp.flit_type == FLIT_TYPE_TAIL)
                receivedPackets++;
            stats.receivedFlit(sc_time_stamp().to_double() / GlobalParams::clock_period_ps, flit_tmp);
        }
        ack_rx.write(current_level_rx);
    }
}

// void MemNetworkInterface::rxProcess()
// {
//     if (reset.read())
//     {
//         ack_rx.write(0);
//         current_level_rx = 0;
//     }
//     else
//     {
//         if (req_rx.read() == 1 - current_level_rx)
//         {
//             // if (!pebuffer.IsFull())
//             // {
//             //     Flit flit_tmp = flit_rx.read();
//             //     // std::cout << " NI RX: => "
//             //     //           << "Flit: " << flit_tmp.payload.data << endl;
//             //     // std::cout << "Flit: " << flit_tmp.payload.data << endl;
//             //     pebuffer.Push(flit_tmp);
//             //     current_level_rx = 1 - current_level_rx; // Negate the old value for Alternating Bit Protocol (ABP)
//             // }
//         }
//         ack_rx.write(current_level_rx);
//     }
// }

void MemNetworkInterface::txProcess()
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

Flit MemNetworkInterface::nextFlit()
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
        makeF_state = INPUT_DATA;
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

bool MemNetworkInterface::canShot(Packet &packet)
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
        int remaining_traffic = traffic_table->getPacketinCommunication(memory_id, dstfromTable);
        // cout << " / Remaining Traffic in PE: " << remaining_traffic << endl;
        if (remaining_traffic > 0)
        {
            int vc = randInt(0, GlobalParams::n_virtual_channels - 1);
            packet.make(1, memory_id, transaction_dst_type, dstfromTable, vc, now, getRandomSize());
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
        if (ifm_data_cnt == 0 && w_data_cnt == 0 && remain_ifm_size == 0 && remain_w_size == 0)
        {
            remaining_traffic = transaction_table->getTransactionInfo(1, memory_id, 0, transaction_dst, transaction_opt,
                                                                      transaction_act, transaction_ctrl,
                                                                      ifmap_data, weight_data);
            ifm_data_cnt = ifmap_data.size();
            remain_ifm_size = ifmap_data.size();
            w_data_cnt = weight_data.size();
            remain_w_size = weight_data.size();
            // cout << "ifm cnt: " << ifm_data_cnt << " " << remain_ifm_size << endl;
        }

        //* Make the packet in different state and size
        if (remaining_traffic > 0)
        {
            int vc = randInt(0, GlobalParams::n_virtual_channels - 1);

            if (makeP_state == INSTRUCTION)
            {
                packet.make(1, memory_id, 0, transaction_dst, vc, now, 2);
                makeP_state = INPUT_DATA;
                // cout << "INSTRUCTION PP\n";
            }
            else if (makeP_state == INPUT_DATA)
            {
                if (ifm_data_cnt - 8 > 0)
                {
                    packet.make(1, memory_id, 0, transaction_dst, vc, now, 8 + 1);
                    ifm_data_cnt -= 8;
                }
                else
                {
                    packet.make(1, memory_id, 0, transaction_dst, vc, now, ifm_data_cnt + 1);
                    ifm_data_cnt = 0;
                    makeP_state = WEIGHT_DATA;
                }
                // cout << "INPUT_DATA PP\n";
            }
            else if (makeP_state == WEIGHT_DATA)
            {
                if (w_data_cnt - 8 > 0)
                {
                    packet.make(1, memory_id, 0, transaction_dst, vc, now, 8 + 1);
                    w_data_cnt -= 8;
                }
                else
                {
                    packet.make(1, memory_id, 0, transaction_dst, vc, now, w_data_cnt + 1);
                    w_data_cnt = 0;
                    makeP_state = INSTRUCTION; // TODO: Confirm WB state along with PE injected traffic
                }
                // cout << "WEIGHT_DATA PP\n";
            }
            shot = true;

            if (ifm_data_cnt == 0 && w_data_cnt == 0)
                remaining_traffic = -1;
        }
        else
        {
            shot = false;
        }
    }

    return shot;
}

// int MemNetworkInterface::findRandomDestination(int id, int hops)
// {
//     assert(GlobalParams::topology == TOPOLOGY_MESH);

//     int inc_y = rand() % 2 ? -1 : 1;
//     int inc_x = rand() % 2 ? -1 : 1;

//     Coord current = id2Coord(id);

//     for (int h = 0; h < hops; h++)
//     {

//         if (current.x == 0)
//             if (inc_x < 0)
//                 inc_x = 0;

//         if (current.x == GlobalParams::mesh_dim_x - 1)
//             if (inc_x > 0)
//                 inc_x = 0;

//         if (current.y == 0)
//             if (inc_y < 0)
//                 inc_y = 0;

//         if (current.y == GlobalParams::mesh_dim_y - 1)
//             if (inc_y > 0)
//                 inc_y = 0;

//         if (rand() % 2)
//             current.x += inc_x;
//         else
//             current.y += inc_y;
//     }
//     return coord2Id(current);
// }

// int roulette()
// {
//     int slices = GlobalParams::mesh_dim_x + GlobalParams::mesh_dim_y - 2;

//     double r = rand() / (double)RAND_MAX;

//     for (int i = 1; i <= slices; i++)
//     {
//         if (r < (1 - 1 / double(2 << i)))
//         {
//             return i;
//         }
//     }
//     assert(false);
//     return 1;
// }

// Packet MemNetworkInterface::trafficULocal()
// {
//     Packet p;
//     p.src_id = local_id;

//     int target_hops = roulette();

//     p.dst_id = findRandomDestination(local_id, target_hops);

//     p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
//     p.size = p.flit_left = getRandomSize();
//     p.vc_id = randInt(0, GlobalParams::n_virtual_channels - 1);

//     return p;
// }

// Packet MemNetworkInterface::trafficRandom()
// {
//     Packet p;
//     p.src_id = local_id;
//     double rnd = rand() / (double)RAND_MAX;
//     double range_start = 0.0;
//     int max_id;

//     if (GlobalParams::topology == TOPOLOGY_MESH)
//         max_id = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) - 1; // Mesh
//     else                                                                    // other delta topologies
//         max_id = GlobalParams::n_delta_tiles - 1;

//     // Random destination distribution
//     do
//     {
//         p.dst_id = randInt(0, max_id);

//         // check for hotspot destination
//         for (size_t i = 0; i < GlobalParams::hotspots.size(); i++)
//         {

//             if (rnd >= range_start && rnd < range_start + GlobalParams::hotspots[i].second)
//             {
//                 if (local_id != GlobalParams::hotspots[i].first)
//                 {
//                     p.dst_id = GlobalParams::hotspots[i].first;
//                 }
//                 break;
//             }
//             else
//                 range_start += GlobalParams::hotspots[i].second; // try next
//         }
// #ifdef DEADLOCK_AVOIDANCE
//         assert((GlobalParams::topology == TOPOLOGY_MESH));
//         if (p.dst_id % 2 != 0)
//         {
//             p.dst_id = (p.dst_id + 1) % 256;
//         }
// #endif

//     } while (p.dst_id == p.src_id);

//     p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
//     p.size = p.flit_left = getRandomSize();
//     p.vc_id = randInt(0, GlobalParams::n_virtual_channels - 1);

//     return p;
// }
// // TODO: for testing only
// Packet MemNetworkInterface::trafficTest()
// {
//     Packet p;
//     p.src_id = local_id;
//     p.dst_id = 10;

//     p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
//     p.size = p.flit_left = getRandomSize();
//     p.vc_id = randInt(0, GlobalParams::n_virtual_channels - 1);

//     return p;
// }

// Packet MemNetworkInterface::trafficTranspose1()
// {
//     assert(GlobalParams::topology == TOPOLOGY_MESH);
//     Packet p;
//     p.src_id = local_id;
//     Coord src, dst;

//     // Transpose 1 destination distribution
//     src.x = id2Coord(p.src_id).x;
//     src.y = id2Coord(p.src_id).y;
//     dst.x = GlobalParams::mesh_dim_x - 1 - src.y;
//     dst.y = GlobalParams::mesh_dim_y - 1 - src.x;
//     fixRanges(src, dst);
//     p.dst_id = coord2Id(dst);

//     p.vc_id = randInt(0, GlobalParams::n_virtual_channels - 1);
//     p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
//     p.size = p.flit_left = getRandomSize();

//     return p;
// }

// Packet MemNetworkInterface::trafficTranspose2()
// {
//     assert(GlobalParams::topology == TOPOLOGY_MESH);
//     Packet p;
//     p.src_id = local_id;
//     Coord src, dst;

//     // Transpose 2 destination distribution
//     src.x = id2Coord(p.src_id).x;
//     src.y = id2Coord(p.src_id).y;
//     dst.x = src.y;
//     dst.y = src.x;
//     fixRanges(src, dst);
//     p.dst_id = coord2Id(dst);

//     p.vc_id = randInt(0, GlobalParams::n_virtual_channels - 1);
//     p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
//     p.size = p.flit_left = getRandomSize();

//     return p;
// }

// void MemNetworkInterface::setBit(int &x, int w, int v)
// {
//     int mask = 1 << w;

//     if (v == 1)
//         x = x | mask;
//     else if (v == 0)
//         x = x & ~mask;
//     else
//         assert(false);
// }

// int MemNetworkInterface::getBit(int x, int w)
// {
//     return (x >> w) & 1;
// }

// inline double MemNetworkInterface::log2ceil(double x)
// {
//     return ceil(log(x) / log(2.0));
// }

// Packet MemNetworkInterface::trafficBitReversal()
// {

//     int nbits =
//         (int)
//             log2ceil((double)(GlobalParams::mesh_dim_x *
//                               GlobalParams::mesh_dim_y));
//     int dnode = 0;
//     for (int i = 0; i < nbits; i++)
//         setBit(dnode, i, getBit(local_id, nbits - i - 1));

//     Packet p;
//     p.src_id = local_id;
//     p.dst_id = dnode;

//     p.vc_id = randInt(0, GlobalParams::n_virtual_channels - 1);
//     p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
//     p.size = p.flit_left = getRandomSize();

//     return p;
// }

// Packet MemNetworkInterface::trafficShuffle()
// {

//     int nbits =
//         (int)
//             log2ceil((double)(GlobalParams::mesh_dim_x *
//                               GlobalParams::mesh_dim_y));
//     int dnode = 0;
//     for (int i = 0; i < nbits - 1; i++)
//         setBit(dnode, i + 1, getBit(local_id, i));
//     setBit(dnode, 0, getBit(local_id, nbits - 1));

//     Packet p;
//     p.src_id = local_id;
//     p.dst_id = dnode;

//     p.vc_id = randInt(0, GlobalParams::n_virtual_channels - 1);
//     p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
//     p.size = p.flit_left = getRandomSize();

//     return p;
// }

// Packet MemNetworkInterface::trafficButterfly()
// {

//     int nbits = (int)log2ceil((double)(GlobalParams::mesh_dim_x *
//                                        GlobalParams::mesh_dim_y));
//     int dnode = 0;
//     for (int i = 1; i < nbits - 1; i++)
//         setBit(dnode, i, getBit(local_id, i));
//     setBit(dnode, 0, getBit(local_id, nbits - 1));
//     setBit(dnode, nbits - 1, getBit(local_id, 0));

//     Packet p;
//     p.src_id = local_id;
//     p.dst_id = dnode;

//     p.vc_id = randInt(0, GlobalParams::n_virtual_channels - 1);
//     p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
//     p.size = p.flit_left = getRandomSize();

//     return p;
// }

// void MemNetworkInterface::fixRanges(const Coord src,
//                                   Coord &dst)
// {
//     // Fix ranges
//     if (dst.x < 0)
//         dst.x = 0;
//     if (dst.y < 0)
//         dst.y = 0;
//     if (dst.x >= GlobalParams::mesh_dim_x)
//         dst.x = GlobalParams::mesh_dim_x - 1;
//     if (dst.y >= GlobalParams::mesh_dim_y)
//         dst.y = GlobalParams::mesh_dim_y - 1;
// }

int MemNetworkInterface::getRandomSize()
{
    return randInt(GlobalParams::min_packet_size,
                   GlobalParams::max_packet_size);
}

unsigned int MemNetworkInterface::getQueueSize() const
{
    return packet_queue.size();
}
