/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file represents the top-level testbench
 */

#ifndef __NOXIMNOC_H__
#define __NOXIMNOC_H__

#include <systemc.h>
#include "Tile.h"
#include "GlobalRoutingTable.h"
#include "GlobalTrafficTable.h"
using namespace std;

template <typename T>
struct sc_signal_NSWE
{
    sc_signal<T> east;
    sc_signal<T> west;
    sc_signal<T> south;
    sc_signal<T> north;
};

SC_MODULE(NoC)
{
    public: bool SwitchOnly; //true if the tile are switch only 
    // I/O Ports
    sc_in_clk clock;		// The input clock for the NoC
    sc_in < bool > reset;	// The reset signal for the NoC

    // Signals mesh and switch bloc in delta topologies
    sc_signal_NSWE<bool> **req;
    sc_signal_NSWE<bool> **ack;
    sc_signal_NSWE<TBufferFullStatus> **buffer_full_status;
    sc_signal_NSWE<Flit> **flit;
    sc_signal_NSWE<int> **free_slots;

    // NoP
    sc_signal_NSWE<NoP_data> **nop_data;

    // Matrix of tiles
    Tile ***t;
    Tile ** core;

    // Global tables
    GlobalRoutingTable grtable;
    GlobalTrafficTable gttable;


    // Constructor

    SC_CTOR(NoC) 
    {


	if (GlobalParams::topology == TOPOLOGY_MESH)
	    // Build the Mesh
	    buildMesh();
	else {
	    cerr << "ERROR: Topology " << GlobalParams::topology << " is not yet supported." << endl;
	    exit(0);
    }
	// out of yaml configuration (experimental features)
	//GlobalParams::channel_selection = CHSEL_FIRST_FREE;

	if (GlobalParams::ascii_monitor)
	{
	    SC_METHOD(asciiMonitor);
	    sensitive << clock.pos();
	}

    }

    // Support methods
    Tile *searchNode(const int id) const;

  private:

    void buildMesh();
    void buildCommon();
    void asciiMonitor();
};

#endif
