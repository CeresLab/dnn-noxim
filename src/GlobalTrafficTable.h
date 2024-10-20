/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the definition of the global traffic table
 */

#ifndef __NOXIMGLOBALTRAFFIC_TABLE_H__
#define __NOXIMGLOBALTRAFFIC_TABLE_H__

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <bits/stdc++.h>
#include "DataStructs.h"

using namespace std;

// Structure used to store information into the table
struct Communication
{
	int src;	  // ID of the source node (PE)
	int dst;	  // ID of the destination node (PE)
	double pir;	  // Packet Injection Rate for the link
	double por;	  // Probability Of Retransmission for the link
	int t_on;	  // Time (in cycles) at which activity begins
	int t_off;	  // Time (in cycles) at which activity ends
	int t_period; // Period after which activity starts again

	int count; // Packet count - AddDate: 2023/04/02
};

struct Transaction
{
	int src_type;
	int src;
	int dst_type;
	int dst;

	int operation_type;
	// int data_type;
	int activation_type;

	ControlInfo ctrl_info;

	vector<int> ifmap;
	vector<int> weight;
	vector<int> ofmap;
	int consumed_flag;
};

class GlobalTrafficTable
{

public:
	GlobalTrafficTable();
	// Load traffic table from file. Returns true if ok, false otherwise
	bool load(const char *fname);

	// Returns the cumulative pir por along with a vector of pairs. The
	// first component of the pair is the destination. The second
	// component is the cumulative shotting probability.
	double getCumulativePirPor(const int src_id,
							   const int ccycle,
							   const bool pir_not_por,
							   vector<pair<int, double>> &dst_prob);

	// Returns the number of occurrences of soruce src_id in the traffic
	// table
	int occurrencesAsSource(const int src_id);

	int getPacketinCommunication(const int src_id, int &dst_id);

	bool loadTransaction(const char *fname);
	int getTransactionInfo(const int src_type, const int src_id, int dst_type, int &dst_id, int &op, int &actt,
						   ControlInfo &ctrl, vector<int> &ifm, vector<int> &w);

	void loadPETransaction(const int src_id, Transaction &pe_trans);
	int getPETransactionInfo(const int src_type, const int src_id, int dst_type, int &dst_id, int &op, int &actt,
							 ControlInfo &ctrl, vector<int> &ifm, vector<int> &w, vector<int> &ofm);

	void initPETransaction();

private:
	vector<Communication> traffic_table;
	vector<Transaction> transaction_table;
	vector<vector<Transaction>> pe_transaction_table;
};

#endif
