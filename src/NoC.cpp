/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the Network-on-Chip
 */

#include "NoC.h"

using namespace std;

void NoC::buildCommon()
{
	// Check for routing table availability
	if (GlobalParams::routing_algorithm == ROUTING_TABLE_BASED)
		assert(grtable.load(GlobalParams::routing_table_filename.c_str()));

	// Check for traffic table availability
	if (GlobalParams::traffic_distribution == TRAFFIC_TABLE_BASED)
		assert(gttable.load(GlobalParams::traffic_table_filename.c_str()));

	//* Check for transaction table availability
	if (GlobalParams::traffic_distribution == TRANSACTION_BASED)
		assert(gttable.loadTransaction(GlobalParams::traffic_table_filename.c_str()));
}

void NoC::buildMesh()
{
	buildCommon();

	// Initialize signals
	int dimX = GlobalParams::mesh_dim_x + 1;
	int dimY = GlobalParams::mesh_dim_y + 1;

	req = new sc_signal_NSWE<bool> *[dimX];
	ack = new sc_signal_NSWE<bool> *[dimX];
	buffer_full_status = new sc_signal_NSWE<TBufferFullStatus> *[dimX];
	flit = new sc_signal_NSWE<Flit> *[dimX];

	free_slots = new sc_signal_NSWE<int> *[dimX];
	nop_data = new sc_signal_NSWE<NoP_data> *[dimX];

	for (int i = 0; i < dimX; i++)
	{
		req[i] = new sc_signal_NSWE<bool>[dimY];
		ack[i] = new sc_signal_NSWE<bool>[dimY];
		buffer_full_status[i] = new sc_signal_NSWE<TBufferFullStatus>[dimY];
		flit[i] = new sc_signal_NSWE<Flit>[dimY];

		free_slots[i] = new sc_signal_NSWE<int>[dimY];
		nop_data[i] = new sc_signal_NSWE<NoP_data>[dimY];
	}

	t = new Tile **[GlobalParams::mesh_dim_x];
	for (int i = 0; i < GlobalParams::mesh_dim_x; i++)
	{
		t[i] = new Tile *[GlobalParams::mesh_dim_y];
	}

	mt = new MemTile *[GlobalParams::mesh_dim_y];

	// Create the mesh as a matrix of tiles
	for (int j = 0; j < GlobalParams::mesh_dim_y; j++)
	{
		for (int i = 0; i < GlobalParams::mesh_dim_x; i++)
		{
			// Create the single Tile with a proper name
			char tile_name[64];
			Coord tile_coord;
			tile_coord.x = i;
			tile_coord.y = j;
			int tile_id = coord2Id(tile_coord);
			cout << "PE ID: " << tile_id << endl;
			sprintf(tile_name, "Tile[%02d][%02d]_(#%d)", i, j, tile_id);
			t[i][j] = new Tile(tile_name, tile_id);

			// Tell to the router its coordinates
			t[i][j]->r->configure(tile_id,
								  GlobalParams::stats_warm_up_time,
								  GlobalParams::buffer_depth,
								  grtable);
			t[i][j]->r->power.configureRouter(GlobalParams::flit_size,
											  GlobalParams::buffer_depth,
											  GlobalParams::flit_size,
											  string(GlobalParams::routing_algorithm),
											  "default");

			// Tell to the PE its coordinates
			t[i][j]->ni->local_id = tile_id;
			t[i][j]->pe->local_id = tile_id;

			// Check for traffic table availability
			if (GlobalParams::traffic_distribution == TRAFFIC_TABLE_BASED)
			{
				t[i][j]->ni->traffic_table = &gttable; // Needed to choose destination
				t[i][j]->ni->never_transmit = (gttable.occurrencesAsSource(t[i][j]->ni->local_id) == 0);
			}
			//* Check for transaction table availability
			else if (GlobalParams::traffic_distribution == TRANSACTION_BASED)
			{
				t[i][j]->ni->transaction_table = &gttable; // Needed to choose destination
				t[i][j]->ni->never_transmit = 0;
			}
			else
				t[i][j]->ni->never_transmit = false;

			// Map clock and reset
			t[i][j]->clock(clock);
			t[i][j]->reset(reset);

			// Map Rx signals
			t[i][j]->req_rx[DIRECTION_NORTH](req[i][j].south);
			t[i][j]->flit_rx[DIRECTION_NORTH](flit[i][j].south);
			t[i][j]->ack_rx[DIRECTION_NORTH](ack[i][j].north);
			t[i][j]->buffer_full_status_rx[DIRECTION_NORTH](buffer_full_status[i][j].north);

			t[i][j]->req_rx[DIRECTION_EAST](req[i + 1][j].west);
			t[i][j]->flit_rx[DIRECTION_EAST](flit[i + 1][j].west);
			t[i][j]->ack_rx[DIRECTION_EAST](ack[i + 1][j].east);
			t[i][j]->buffer_full_status_rx[DIRECTION_EAST](buffer_full_status[i + 1][j].east);

			t[i][j]->req_rx[DIRECTION_SOUTH](req[i][j + 1].north);
			t[i][j]->flit_rx[DIRECTION_SOUTH](flit[i][j + 1].north);
			t[i][j]->ack_rx[DIRECTION_SOUTH](ack[i][j + 1].south);
			t[i][j]->buffer_full_status_rx[DIRECTION_SOUTH](buffer_full_status[i][j + 1].south);

			t[i][j]->req_rx[DIRECTION_WEST](req[i][j].east);
			t[i][j]->flit_rx[DIRECTION_WEST](flit[i][j].east);
			t[i][j]->ack_rx[DIRECTION_WEST](ack[i][j].west);
			t[i][j]->buffer_full_status_rx[DIRECTION_WEST](buffer_full_status[i][j].west);

			// Map Tx signals
			t[i][j]->req_tx[DIRECTION_NORTH](req[i][j].north);
			t[i][j]->flit_tx[DIRECTION_NORTH](flit[i][j].north);
			t[i][j]->ack_tx[DIRECTION_NORTH](ack[i][j].south);
			t[i][j]->buffer_full_status_tx[DIRECTION_NORTH](buffer_full_status[i][j].south);

			t[i][j]->req_tx[DIRECTION_EAST](req[i + 1][j].east);
			t[i][j]->flit_tx[DIRECTION_EAST](flit[i + 1][j].east);
			t[i][j]->ack_tx[DIRECTION_EAST](ack[i + 1][j].west);
			t[i][j]->buffer_full_status_tx[DIRECTION_EAST](buffer_full_status[i + 1][j].west);

			t[i][j]->req_tx[DIRECTION_SOUTH](req[i][j + 1].south);
			t[i][j]->flit_tx[DIRECTION_SOUTH](flit[i][j + 1].south);
			t[i][j]->ack_tx[DIRECTION_SOUTH](ack[i][j + 1].north);
			t[i][j]->buffer_full_status_tx[DIRECTION_SOUTH](buffer_full_status[i][j + 1].north);

			t[i][j]->req_tx[DIRECTION_WEST](req[i][j].west);
			t[i][j]->flit_tx[DIRECTION_WEST](flit[i][j].west);
			t[i][j]->ack_tx[DIRECTION_WEST](ack[i][j].east);
			t[i][j]->buffer_full_status_tx[DIRECTION_WEST](buffer_full_status[i][j].east);

			// Map buffer level signals (analogy with req_tx/rx port mapping)
			t[i][j]->free_slots[DIRECTION_NORTH](free_slots[i][j].north);
			t[i][j]->free_slots[DIRECTION_EAST](free_slots[i + 1][j].east);
			t[i][j]->free_slots[DIRECTION_SOUTH](free_slots[i][j + 1].south);
			t[i][j]->free_slots[DIRECTION_WEST](free_slots[i][j].west);

			t[i][j]->free_slots_neighbor[DIRECTION_NORTH](free_slots[i][j].south);
			t[i][j]->free_slots_neighbor[DIRECTION_EAST](free_slots[i + 1][j].west);
			t[i][j]->free_slots_neighbor[DIRECTION_SOUTH](free_slots[i][j + 1].north);
			t[i][j]->free_slots_neighbor[DIRECTION_WEST](free_slots[i][j].east);

			// NoP
			t[i][j]->NoP_data_out[DIRECTION_NORTH](nop_data[i][j].north);
			t[i][j]->NoP_data_out[DIRECTION_EAST](nop_data[i + 1][j].east);
			t[i][j]->NoP_data_out[DIRECTION_SOUTH](nop_data[i][j + 1].south);
			t[i][j]->NoP_data_out[DIRECTION_WEST](nop_data[i][j].west);

			t[i][j]->NoP_data_in[DIRECTION_NORTH](nop_data[i][j].south);
			t[i][j]->NoP_data_in[DIRECTION_EAST](nop_data[i + 1][j].west);
			t[i][j]->NoP_data_in[DIRECTION_SOUTH](nop_data[i][j + 1].north);
			t[i][j]->NoP_data_in[DIRECTION_WEST](nop_data[i][j].east);
		}
	}

	//
	for (int j = 0; j < GlobalParams::mesh_dim_y; j++)
	{
		// Create the single Tile with a proper name
		char mem_tile_name[64];
		// Coord tile_coord;
		// tile_coord.y = j;
		// int tile_id = coord2Id(tile_coord);
		cout << "Memory ID: " << j << endl;
		sprintf(mem_tile_name, "MemTile[%02d][%02d]_(#%d)", GlobalParams::mesh_dim_x - 1, j, j);
		mt[j] = new MemTile(mem_tile_name, j);

		// Tell to the router its coordinates

		// Tell to the PE its coordinates
		mt[j]->memni->memory_id = j;

		// Check for traffic table availability
		if (GlobalParams::traffic_distribution == TRAFFIC_TABLE_BASED)
		{
			mt[j]->memni->traffic_table = &gttable; // Needed to choose destination
			mt[j]->memni->never_transmit = (gttable.occurrencesAsSource(mt[j]->memni->memory_id) == 0);
		}
		//* Check for transaction table availability
		else if (GlobalParams::traffic_distribution == TRANSACTION_BASED)
		{
			mt[j]->memni->transaction_table = &gttable; // Needed to choose destination
			mt[j]->memni->never_transmit = 0;
		}
		else
			mt[j]->memni->never_transmit = false;

		// Map clock and reset
		mt[j]->clock(clock);
		mt[j]->reset(reset);

		// Map Rx signals
		mt[j]->req_rx(req[GlobalParams::mesh_dim_x][j].east);
		mt[j]->flit_rx(flit[GlobalParams::mesh_dim_x][j].east);
		mt[j]->ack_rx(ack[GlobalParams::mesh_dim_x][j].west);
		mt[j]->buffer_full_status_rx(buffer_full_status[GlobalParams::mesh_dim_x][j].west);

		// Map Tx signals

		mt[j]->req_tx(req[GlobalParams::mesh_dim_x][j].west);
		mt[j]->flit_tx(flit[GlobalParams::mesh_dim_x][j].west);
		mt[j]->ack_tx(ack[GlobalParams::mesh_dim_x][j].east);
		mt[j]->buffer_full_status_tx(buffer_full_status[GlobalParams::mesh_dim_x][j].east);

		// Map buffer level signals (analogy with req_tx/rx port mapping)
		// mt[j]->free_slots(free_slots[GlobalParams::mesh_dim_x][j].west);

		mt[j]->free_slots_neighbor(free_slots[GlobalParams::mesh_dim_x][j].east);

		// NoP
		// mt[j]->NoP_data_out(nop_data[GlobalParams::mesh_dim_x][j].west);

		// mt[j]->NoP_data_in(nop_data[GlobalParams::mesh_dim_x + 1][j].west);
	}

	// dummy NoP_data structure
	NoP_data tmp_NoP;

	tmp_NoP.sender_id = NOT_VALID;

	for (int i = 0; i < DIRECTIONS; i++)
	{
		tmp_NoP.channel_status_neighbor[i].free_slots = NOT_VALID;
		tmp_NoP.channel_status_neighbor[i].available = false;
	}

	// Clear signals for borderline nodes

	for (int i = 0; i <= GlobalParams::mesh_dim_x; i++)
	{
		req[i][0].south = 0;
		ack[i][0].north = 0;
		req[i][GlobalParams::mesh_dim_y].north = 0;
		ack[i][GlobalParams::mesh_dim_y].south = 0;

		free_slots[i][0].south.write(NOT_VALID);
		free_slots[i][GlobalParams::mesh_dim_y].north.write(NOT_VALID);

		nop_data[i][0].south.write(tmp_NoP);
		nop_data[i][GlobalParams::mesh_dim_y].north.write(tmp_NoP);
	}

	//* Open signals for borderline nodes of X dimension to connect with Memory Tile
	for (int j = 0; j <= GlobalParams::mesh_dim_y; j++)
	{
		req[0][j].east = 0;
		ack[0][j].west = 0;
		// req[GlobalParams::mesh_dim_x][j].west = 0;
		// ack[GlobalParams::mesh_dim_x][j].east = 0;

		free_slots[0][j].east.write(NOT_VALID);
		// free_slots[GlobalParams::mesh_dim_x][j].west.write(NOT_VALID);

		nop_data[0][j].east.write(tmp_NoP);
		// nop_data[GlobalParams::mesh_dim_x][j].west.write(tmp_NoP);
	}
}

Tile *NoC::searchNode(const int id) const
{
	if (GlobalParams::topology == TOPOLOGY_MESH)
	{
		for (int i = 0; i < GlobalParams::mesh_dim_x; i++)
			for (int j = 0; j < GlobalParams::mesh_dim_y; j++)
				if (t[i][j]->r->local_id == id)
					return t[i][j];
	}
	else // in delta topologies id equals to the vector index
		return core[id];
	return NULL;
}

void NoC::asciiMonitor()
{
	// cout << sc_time_stamp().to_double()/GlobalParams::clock_period_ps << endl;
	system("clear");
	//
	// asciishow proof-of-concept #1 free slots

	if (GlobalParams::topology != TOPOLOGY_MESH)
	{
		cout << "Delta topologies are not supported for asciimonitor option!";
		assert(false);
	}
	for (int j = 0; j < GlobalParams::mesh_dim_y; j++)
	{
		for (int s = 0; s < 3; s++)
		{
			for (int i = 0; i < GlobalParams::mesh_dim_x; i++)
			{
				if (s == 0)
					std::printf("|  %d  ", t[i][j]->r->buffer[s][0].getCurrentFreeSlots());
				else if (s == 1)
					std::printf("|%d   %d", t[i][j]->r->buffer[s][0].getCurrentFreeSlots(), t[i][j]->r->buffer[3][0].getCurrentFreeSlots());
				else
					std::printf("|__%d__", t[i][j]->r->buffer[2][0].getCurrentFreeSlots());
			}
			cout << endl;
		}
	}
}
