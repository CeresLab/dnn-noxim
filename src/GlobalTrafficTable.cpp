/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the global traffic table
 */

#include "GlobalTrafficTable.h"

GlobalTrafficTable::GlobalTrafficTable()
{
}

bool GlobalTrafficTable::load(const char *fname)
{
	// Open file
	ifstream fin(fname, ios::in);
	if (!fin)
		return false;

	// Initialize variables
	traffic_table.clear();

	// Cycle reading file
	while (!fin.eof())
	{
		char line[512];
		fin.getline(line, sizeof(line) - 1);

		if (line[0] != '\0')
		{
			if (line[0] != '%')
			{
				int src, dst; // Mandatory
				double pir, por;
				int t_on, t_off, t_period;
				int count;

				int params =
					sscanf(line, "%d %d %d %lf %lf %d %d %d", &src, &dst, &count, &pir,
						   &por, &t_on, &t_off, &t_period);
				if (params >= 2)
				{
					// Create a communication from the parameters read on the line
					Communication communication;

					// Mandatory fields
					communication.src = src;
					communication.dst = dst;

					if (params >= 3)
						communication.count = count;

					// Custom PIR
					if (params >= 4 && pir >= 0 && pir <= 1)
						communication.pir = pir;
					else
						communication.pir =
							GlobalParams::packet_injection_rate;

					// Custom POR
					if (params >= 5 && por >= 0 && por <= 1)
						communication.por = por;
					else
						communication.por = communication.pir; // GlobalParams::probability_of_retransmission;

					// Custom Ton
					if (params >= 6 && t_on >= 0)
						communication.t_on = t_on;
					else
						communication.t_on = 0;

					// Custom Toff
					if (params >= 7 && t_off >= 0)
					{
						assert(t_off > t_on);
						communication.t_off = t_off;
					}
					else
						communication.t_off =
							GlobalParams::reset_time +
							GlobalParams::simulation_time;

					// Custom Tperiod
					if (params >= 8 && t_period > 0)
					{
						assert(t_period > t_off);
						communication.t_period = t_period;
					}
					else
						communication.t_period =
							GlobalParams::reset_time +
							GlobalParams::simulation_time;

					// Add this communication to the vector of communications
					traffic_table.push_back(communication);
				}
			}
		}
	}

	return true;
}

double GlobalTrafficTable::getCumulativePirPor(const int src_id,
											   const int ccycle,
											   const bool pir_not_por,
											   vector<pair<int, double>> &dst_prob)
{
	double cpirnpor = 0.0;
	dst_prob.clear();
	for (unsigned int i = 0; i < traffic_table.size(); i++)
	{
		Communication comm = traffic_table[i];
		if (comm.src == src_id)
		{
			int r_ccycle = ccycle % comm.t_period;
			if (r_ccycle > comm.t_on && r_ccycle < comm.t_off)
			{
				cpirnpor += pir_not_por ? comm.pir : comm.por;
				pair<int, double> dp(comm.dst, cpirnpor);
				dst_prob.push_back(dp);
			}
		}
	}

	return cpirnpor;
}

// Get each communication packet counts - AddDate: 2023/04/02
int GlobalTrafficTable::getPacketinCommunication(const int src_id, int &dst_id)
{
	for (unsigned int i = 0; i < traffic_table.size(); i++)
	{
		// cout << "Taffic [ " << i+1 << " ]: Src=" << traffic_table[ i ].src << " Dst=" << traffic_table[ i ].dst << " Before--Count=" << traffic_table[ i ].count;
		if (traffic_table[i].src == src_id && traffic_table[i].count > 0)
		{
			traffic_table[i].count = traffic_table[i].count - 1;
			// cout << "->After--Count=" << traffic_table[ i ].count << endl;
			dst_id = traffic_table[i].dst;
			return traffic_table[i].count + 1;
		}
	}
	return -1;
}

int GlobalTrafficTable::occurrencesAsSource(const int src_id)
{
	int count = 0;

	for (unsigned int i = 0; i < traffic_table.size(); i++)
		if (traffic_table[i].src == src_id)
			count++;

	return count;
}

bool GlobalTrafficTable::loadTransaction(const char *fname)
{
	ifstream fin;
	fin.open(fname);
	if (!fin)
		return false;

	transaction_table.clear();

	/*
	 * Fetch transactions (8 lines per each)
	 *	'#': source and destination, STATE = 0
	 *	'?': operation type and activation type, STATE = 1
	 *	'!': control inforamtions, STATE = 2
	 *	'%': input feature map and weight data, STATE = 3
	 */
	int fetch_state = 0;
	while (!fin.eof())
	{
		string line;
		Transaction trans;
		for (int i = 0; i < 8; i++)
		{
			getline(fin, line);

			if (line[0] != '>' && fetch_state == 0)
			{
				int src_type, src_id, dst_id, dst_type;
				int params =
					sscanf(line.c_str(), "%d %d %d %d", &src_type, &src_id, &dst_type, &dst_id);

				assert(params == 4);

				trans.src_type = src_type;
				trans.src = src_id;
				trans.dst_type = dst_type;
				trans.dst = dst_id;
			}
			else if (line[0] != '>' && fetch_state == 1)
			{
				int opt, actt;
				int params =
					sscanf(line.c_str(), "%d %d", &opt, &actt);

				assert(params == 2);

				trans.operation_type = opt;
				trans.activation_type = actt;
			}
			else if (line[0] != '>' && fetch_state == 2)
			{
				int kw, kh, stride, ifw, ifh, wb_dst;
				int params =
					sscanf(line.c_str(), "%d %d %d %d %d %d", &kw, &kh, &stride,
						   &ifw, &ifh, &wb_dst);

				assert(params == 6);

				trans.ctrl_info.kernel_width = kw;
				trans.ctrl_info.kernel_height = kh;
				trans.ctrl_info.stride = stride;
				trans.ctrl_info.input_width = ifw;
				trans.ctrl_info.input_height = ifh;
				trans.ctrl_info.wb_dst = wb_dst;
			}
			else if (line[0] != '>' && fetch_state == 3)
			{
				string tmp;
				stringstream ss(line);
				int ifm_data_cnt = trans.ctrl_info.input_width * trans.ctrl_info.input_height;
				int w_data_cnt = trans.ctrl_info.kernel_width * trans.ctrl_info.kernel_height;
				int format = 0;

				while (getline(ss, tmp, ' '))
				{
					if (ifm_data_cnt > format)
						trans.ifmap.push_back(stoi(tmp));
					else
						trans.weight.push_back(stoi(tmp));
					format++;
				}

				if (trans.operation_type == CONV2D || trans.operation_type == FC)
					assert(format == (ifm_data_cnt + w_data_cnt));
				else if (trans.operation_type == POOLING)
					assert(format == ifm_data_cnt);
			}
			else
			{
				if (line[1] == '#')
					fetch_state = 0;
				if (line[1] == '?')
					fetch_state = 1;
				if (line[1] == '!')
					fetch_state = 2;
				if (line[1] == '%')
					fetch_state = 3;
			}
		}
		trans.consumed_flag = 0;
		transaction_table.push_back(trans);
	}

	for (unsigned int i = 0; i < transaction_table[0].ifmap.size(); i++)
	{
		cout << transaction_table[0].ifmap[i] << " ";
	}
	cout << endl;
	return true;
}

int GlobalTrafficTable::getTransactionInfo(const int src_type, const int src_id, int dst_type, int &dst_id, int &op, int &actt,
										   ControlInfo &ctrl, vector<int> &ifm, vector<int> &w)
{
	for (unsigned int i = 0; i < transaction_table.size(); i++)
	{
		if (transaction_table[i].src == src_id && transaction_table[i].src_type == src_type && !transaction_table[i].consumed_flag)
		{
			dst_type = transaction_table[i].dst_type;
			dst_id = transaction_table[i].dst;
			op = transaction_table[i].operation_type;
			actt = transaction_table[i].activation_type;
			ctrl = transaction_table[i].ctrl_info;
			ifm = transaction_table[i].ifmap;
			w = transaction_table[i].weight;

			transaction_table[i].consumed_flag = 1;
			return 1;
		}
		else
			continue;
	}

	ifm.clear();
	w.clear();
	return -1;
}