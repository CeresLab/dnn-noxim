/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the tile
 */

#ifndef __NOXIMMEMTILE_H__
#define __NOXIMMEMTILE_H__

#include <systemc.h>
#include "MemNetworkInterface.h"

using namespace std;

SC_MODULE(MemTile)
{
	SC_HAS_PROCESS(MemTile);

	// I/O Ports
	sc_in_clk clock;   // The input clock for the tile
	sc_in<bool> reset; // The reset signal for the tile

	int memory_id; // Unique ID

	sc_in<Flit> flit_rx; // The input channels
	sc_in<bool> req_rx;	 // The requests associated with the input channels
	sc_out<bool> ack_rx; // The outgoing ack signals associated with the input channels
	sc_out<TBufferFullStatus> buffer_full_status_rx;

	sc_out<Flit> flit_tx; // The output channels
	sc_out<bool> req_tx;  // The requests associated with the output channels
	sc_in<bool> ack_tx;	  // The outgoing ack signals associated with the output channels
	sc_in<TBufferFullStatus> buffer_full_status_tx;

	// NoP related I/O and signals
	// sc_out<int> free_slots;
	sc_in<int> free_slots_neighbor;
	// sc_out<NoP_data> NoP_data_out;
	// sc_in<NoP_data> NoP_data_in;

	// sc_signal<int> free_slots_local;
	// sc_signal<int> free_slots_neighbor_local;

	// Signals required for Router-PE connection
	// sc_signal<Flit> flit_rx_local;
	// sc_signal<bool> req_rx_local;
	// sc_signal<bool> ack_rx_local;
	// sc_signal<TBufferFullStatus> buffer_full_status_rx_local;

	// sc_signal<Flit> flit_tx_local;
	// sc_signal<bool> req_tx_local;
	// sc_signal<bool> ack_tx_local;
	// sc_signal<TBufferFullStatus> buffer_full_status_tx_local;

	// sc_signal<Flit> flit_rx_pe;
	// sc_signal<bool> req_rx_pe;
	// sc_signal<bool> ack_rx_pe;
	// sc_signal<TBufferFullStatus> buffer_full_status_rx_pe;

	// sc_signal<Flit> flit_tx_pe;
	// sc_signal<bool> req_tx_pe;
	// sc_signal<bool> ack_tx_pe;
	// sc_signal<TBufferFullStatus> buffer_full_status_tx_pe;

	// Instances
	MemNetworkInterface *memni; // Processing Element instance
	// Constructor

	MemTile(sc_module_name nm, int id) : sc_module(nm)
	{
		memory_id = id;

		// Memory network interface pin assignments
		memni = new MemNetworkInterface("MemNetworkInterface");
		memni->clock(clock);
		memni->reset(reset);

		memni->flit_rx(flit_rx);
		memni->req_rx(req_rx);
		memni->ack_rx(ack_rx);
		memni->buffer_full_status_rx(buffer_full_status_rx);

		memni->flit_tx(flit_tx);
		memni->req_tx(req_tx);
		memni->ack_tx(ack_tx);
		memni->buffer_full_status_tx(buffer_full_status_tx);

		// memni->free_slots(free_slots);
		memni->free_slots_neighbor(free_slots_neighbor);

		// local
		// memni->flit_rx_pe(flit_tx_local);
		// memni->req_rx_pe(req_tx_local);
		// memni->ack_rx_pe(ack_tx_local);
		// memni->buffer_full_status_rx_pe(buffer_full_status_tx_local);

		// memni->flit_tx_pe(flit_rx_local);
		// memni->req_tx_pe(req_rx_local);
		// memni->ack_tx_pe(ack_rx_local);
		// memni->buffer_full_status_tx_pe(buffer_full_status_rx_local);

		// NoP
		// memni->free_slots_neighbor(free_slots_neighbor_local);
	}
};

#endif
