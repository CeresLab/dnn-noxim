/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the power model
 */

#include <iostream>
#include "Power.h"
#include "Utils.h"
#include "systemc.h"

#define W2J(watt) ((watt)*GlobalParams::clock_period_ps*1.0e-12)

using namespace std;


Power::Power()
{
    total_power_s = 0.0;

    buffer_router_push_pwr_d = 0.0;
    buffer_router_pop_pwr_d = 0.0;
    buffer_router_front_pwr_d = 0.0;
    buffer_router_pwr_s = 0.0;
    
    routing_pwr_d = 0.0;
    routing_pwr_s = 0.0;

    selection_pwr_d = 0.0;
    selection_pwr_s = 0.0;

    crossbar_pwr_d = 0.0;
    crossbar_pwr_s = 0.0;

    link_r2r_pwr_d = 0.0;
    link_r2r_pwr_s = 0.0;
    default_tx_energy = 0.0;

    ni_pwr_d = 0.0;
    ni_pwr_s = 0.0;


    sleep_end_cycle = NOT_VALID;

    initPowerBreakdown();
}

void Power::configureRouter(int link_width,
	int buffer_depth,
	int buffer_item_size,
	string routing_function,
	string selection_function)
{
// (s)tatic, (d)ynamic power

    // Buffer 
    pair<int,int> key = pair<int,int>(buffer_depth, buffer_item_size);
    
    assert(GlobalParams::power_configuration.bufferPowerConfig.leakage.find(key) != GlobalParams::power_configuration.bufferPowerConfig.leakage.end());
    assert(GlobalParams::power_configuration.bufferPowerConfig.push.find(key) != GlobalParams::power_configuration.bufferPowerConfig.push.end());
    assert(GlobalParams::power_configuration.bufferPowerConfig.front.find(key) != GlobalParams::power_configuration.bufferPowerConfig.front.end());
    assert(GlobalParams::power_configuration.bufferPowerConfig.pop.find(key) != GlobalParams::power_configuration.bufferPowerConfig.pop.end());

    // Dynamic values are expressed in Joule
    // Static/Leakage values must be converted from Watt to Joule

    buffer_router_pwr_s = W2J(GlobalParams::power_configuration.bufferPowerConfig.leakage[key]);
    buffer_router_push_pwr_d = GlobalParams::power_configuration.bufferPowerConfig.push[key];
    buffer_router_front_pwr_d = GlobalParams::power_configuration.bufferPowerConfig.front[key];
    buffer_router_pop_pwr_d = GlobalParams::power_configuration.bufferPowerConfig.pop[key];

    // Routing 
    assert(GlobalParams::power_configuration.routerPowerConfig.routing_algorithm_pm.find(routing_function) != GlobalParams::power_configuration.routerPowerConfig.routing_algorithm_pm.end());

    routing_pwr_s = W2J(GlobalParams::power_configuration.routerPowerConfig.routing_algorithm_pm[routing_function].first);
    routing_pwr_d = GlobalParams::power_configuration.routerPowerConfig.routing_algorithm_pm[routing_function].second;

    // Selection 
    assert(GlobalParams::power_configuration.routerPowerConfig.selection_strategy_pm.find(selection_function) != GlobalParams::power_configuration.routerPowerConfig.selection_strategy_pm.end());

    selection_pwr_s = W2J(GlobalParams::power_configuration.routerPowerConfig.selection_strategy_pm[selection_function].first);
    selection_pwr_d = GlobalParams::power_configuration.routerPowerConfig.selection_strategy_pm[selection_function].second;

    // CrossBar
    // TODO future work: tuning of crossbar radix
    pair<int,int> xbar_k = pair<int,int>(5,GlobalParams::flit_size);
    assert(GlobalParams::power_configuration.routerPowerConfig.crossbar_pm.find(xbar_k) != GlobalParams::power_configuration.routerPowerConfig.crossbar_pm.end());
    crossbar_pwr_s = W2J(GlobalParams::power_configuration.routerPowerConfig.crossbar_pm[xbar_k].first);
    crossbar_pwr_d = GlobalParams::power_configuration.routerPowerConfig.crossbar_pm[xbar_k].second;
    
    // NetworkInterface
    ni_pwr_s = W2J(GlobalParams::power_configuration.routerPowerConfig.network_interface[GlobalParams::flit_size].first);
    ni_pwr_d = GlobalParams::power_configuration.routerPowerConfig.network_interface[GlobalParams::flit_size].second;

    // Link 
    // Router has both type of links
    double length_r2r = GlobalParams::r2r_link_length;
    
    assert(GlobalParams::power_configuration.linkBitLinePowerConfig.find(length_r2r)!=GlobalParams::power_configuration.linkBitLinePowerConfig.end());

    link_r2r_pwr_s= W2J(link_width * GlobalParams::power_configuration.linkBitLinePowerConfig[length_r2r].first);
    link_r2r_pwr_d= link_width * GlobalParams::power_configuration.linkBitLinePowerConfig[length_r2r].second;
}

// Router buffer
void Power::bufferRouterPush()
{
    power_dynamic.breakdown[BUFFER_PUSH_PWR_D].value += buffer_router_push_pwr_d;
}

void Power::bufferRouterPop()
{
    power_dynamic.breakdown[BUFFER_POP_PWR_D].value += buffer_router_pop_pwr_d;
}

void Power::bufferRouterFront()
{
    power_dynamic.breakdown[BUFFER_FRONT_PWR_D].value += buffer_router_front_pwr_d;
}

void Power::routing()
{
    power_dynamic.breakdown[ROUTING_PWR_D].value += routing_pwr_d;
}

void Power::selection()
{
    power_dynamic.breakdown[SELECTION_PWR_D].value +=selection_pwr_d ;
}

void Power::crossBar()
{
    power_dynamic.breakdown[CROSSBAR_PWR_D].value +=crossbar_pwr_d;
}

void Power::r2rLink()
{
    power_dynamic.breakdown[LINK_R2R_PWR_D].value +=link_r2r_pwr_d;
}

void Power::networkInterface()
{
    power_dynamic.breakdown[NI_PWR_D].value +=ni_pwr_d;
}


double Power::getDynamicPower()
{
    double power = 0.0;
    for (int i = 0; i<power_dynamic.size; i++)
    {
	power+= power_dynamic.breakdown[i].value;
    }

    return power;
}

double Power::getStaticPower()
{
    double power = 0.0;
    for (int i = 0; i<power_static.size; i++)
	power+= power_static.breakdown[i].value;

    return power;
}

void Power::biasingRx()
{
    power_static.breakdown[TRANSCEIVER_RX_PWR_BIASING].value += transceiver_rx_pwr_biasing;
}

void Power::biasingTx()
{
    power_static.breakdown[TRANSCEIVER_TX_PWR_BIASING].value += transceiver_tx_pwr_biasing;

}

// Note: In the following 3 functions buffer_pwr_s 
// is assumed as loaded with the proper values from configuration file:
// - Router: takes the value of input buffers leakage
// - Hub: takes the leakage value of buffer_from_tile/to_tile
void Power::leakageBufferRouter()
{
    power_static.breakdown[BUFFER_ROUTER_PWR_S].value +=buffer_router_pwr_s;
}

void Power::leakageLinkRouter2Router()
{
    //power_static.breakdown[LINK_R2R_PWR_S].value +=link_r2r_pwr_s;
}

void Power::leakageRouter()
{
    // note: leakage contributions depending on instance number are 
    // accounted in specific separate leakage functions
    power_static.breakdown[ROUTING_PWR_S].value +=routing_pwr_s;
    power_static.breakdown[SELECTION_PWR_S].value +=selection_pwr_s;
    power_static.breakdown[CROSSBAR_PWR_S].value +=crossbar_pwr_s;
    power_static.breakdown[NI_PWR_S].value +=ni_pwr_s;
}



void Power::leakageTransceiverRx()
{

    power_static.breakdown[TRANSCEIVER_RX_PWR_S].value +=transceiver_rx_pwr_s;
}

void Power::leakageTransceiverTx()
{

    power_static.breakdown[TRANSCEIVER_TX_PWR_S].value +=transceiver_tx_pwr_s;
}

void Power::printBreakDown(std::ostream & out)
{
    assert(false);
    //printMap("power_dynamic",power_dynamic,cout);
    //printMap("power_static",power_static,cout);
}


void Power::rxSleep(int cycles)
{

    int sleep_start_cycle = (int)(sc_time_stamp().to_double()/GlobalParams::clock_period_ps);
    sleep_end_cycle = sleep_start_cycle + cycles;
}


bool Power::isSleeping()
{
    assert(GlobalParams::use_powermanager);
    int now = (int)(sc_time_stamp().to_double()/GlobalParams::clock_period_ps);

    return (now<sleep_end_cycle);

}


void Power::initPowerBreakdownEntry(PowerBreakdownEntry* pbe,string label)
{
    pbe->label = label;
    pbe->value = 0.0;
}



void Power::initPowerBreakdown()
{
    power_dynamic.size = NO_BREAKDOWN_ENTRIES_D;
    power_static.size = NO_BREAKDOWN_ENTRIES_S;

    initPowerBreakdownEntry(&power_dynamic.breakdown[BUFFER_PUSH_PWR_D], "buffer_push_pwr_d");
    initPowerBreakdownEntry(&power_dynamic.breakdown[BUFFER_POP_PWR_D],"buffer_pop_pwr_d");
    initPowerBreakdownEntry(&power_dynamic.breakdown[BUFFER_FRONT_PWR_D],"buffer_front_pwr_d");
    initPowerBreakdownEntry(&power_dynamic.breakdown[ROUTING_PWR_D],"routing_pwr_d");
    initPowerBreakdownEntry(&power_dynamic.breakdown[SELECTION_PWR_D],"selection_pwr_d");
    initPowerBreakdownEntry(&power_dynamic.breakdown[CROSSBAR_PWR_D],"crossbar_pwr_d");
    initPowerBreakdownEntry(&power_dynamic.breakdown[LINK_R2R_PWR_D],"link_r2r_pwr_d");
    initPowerBreakdownEntry(&power_dynamic.breakdown[NI_PWR_D],"ni_pwr_d");

    initPowerBreakdownEntry(&power_static.breakdown[TRANSCEIVER_RX_PWR_BIASING],"transceiver_rx_pwr_biasing");
    initPowerBreakdownEntry(&power_static.breakdown[TRANSCEIVER_TX_PWR_BIASING],"transceiver_tx_pwr_biasing");
    initPowerBreakdownEntry(&power_static.breakdown[BUFFER_ROUTER_PWR_S],"buffer_router_pwr_s");
    initPowerBreakdownEntry(&power_static.breakdown[ROUTING_PWR_S],"routing_pwr_s");
    initPowerBreakdownEntry(&power_static.breakdown[SELECTION_PWR_S],"selection_pwr_s");
    initPowerBreakdownEntry(&power_static.breakdown[CROSSBAR_PWR_S],"crossbar_pwr_s");
    initPowerBreakdownEntry(&power_static.breakdown[NI_PWR_S],"ni_pwr_s");
    initPowerBreakdownEntry(&power_static.breakdown[TRANSCEIVER_RX_PWR_S],"transceiver_rx_pwr_s");
    initPowerBreakdownEntry(&power_static.breakdown[TRANSCEIVER_TX_PWR_S],"transceiver_tx_pwr_s");
}

    



