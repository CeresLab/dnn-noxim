#ifndef __NOXIMROUTING_HAMILTON_H__
#define __NOXIMROUTING_HAMILTON_H__

#include "RoutingAlgorithm.h"
#include "RoutingAlgorithms.h"
#include "../Router.h"
#define name() "Routing_HAMILTON"

using namespace std;

class Routing_HAMILTON : RoutingAlgorithm {
	public:
		vector<int> route(Router * router, const RouteData & routeData);

		static Routing_HAMILTON * getInstance();

	private:
		Routing_HAMILTON(){};
		~Routing_HAMILTON(){};

		static Routing_HAMILTON * routing_HAMILTON;
		static RoutingAlgorithmsRegister routingAlgorithmsRegister;
};

#endif
