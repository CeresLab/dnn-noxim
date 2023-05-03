#include "Routing_HAMILTON.h"

RoutingAlgorithmsRegister Routing_HAMILTON::routingAlgorithmsRegister("HAMILTON", getInstance());

Routing_HAMILTON *Routing_HAMILTON::routing_HAMILTON = 0;

Routing_HAMILTON *Routing_HAMILTON::getInstance()
{
    if (routing_HAMILTON == 0) routing_HAMILTON = new Routing_HAMILTON();
    return routing_HAMILTON;
}

vector<int>Routing_HAMILTON::route(Router *router, const RouteData &routeData)
{
    Coord current = id2Coord(routeData.current_id);
    Coord destination = id2Coord(routeData.dst_id);
    vector <int> directions;

    if (routeData.dst_id > routeData.current_id)
    {
        if (routeData.current_id == 0)
            directions.push_back(DIRECTION_EAST);
        else if (current.y % 2 == 0)
        {
            if (current.x == 0)
                directions.push_back(DIRECTION_EAST);
            else if (current.x == GlobalParams::mesh_dim_x - 1)
                directions.push_back(DIRECTION_SOUTH);
            else
                directions.push_back(DIRECTION_EAST);
        }
        else
        {
            if (current.x == GlobalParams::mesh_dim_x - 1)
                directions.push_back(DIRECTION_SOUTH);
            else if (current.x == 0)
                directions.push_back(DIRECTION_WEST);
            else
                directions.push_back(DIRECTION_WEST);
        }
    }   
    else
    {
        if (routeData.current_id == GlobalParams::mesh_dim_x - 1)
        {
            if (GlobalParams::mesh_dim_x % 2 == 0)
                directions.push_back(DIRECTION_EAST);
            else
                directions.push_back(DIRECTION_WEST);
        }
        else if (current.y % 2 == 0)
        {
            if (current.x == 0)
                directions.push_back(DIRECTION_NORTH);
            else if (current.x == GlobalParams::mesh_dim_x - 1)
                directions.push_back(DIRECTION_WEST);
            else
                directions.push_back(DIRECTION_WEST);
        }
        else
        {
            if (current.x == GlobalParams::mesh_dim_x - 1)
                directions.push_back(DIRECTION_EAST);
            else if (current.x == 0)
                directions.push_back(DIRECTION_NORTH);
            else
                directions.push_back(DIRECTION_EAST);
        }
    }

    return directions;
}
