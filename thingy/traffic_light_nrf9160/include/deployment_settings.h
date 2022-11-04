#ifndef TRAFFIC_LIGHT_NRF9160_DEPLOYMENT_SETTINGS_H_
#define TRAFFIC_LIGHT_NRF9160_DEPLOYMENT_SETTINGS_H_
/*
    This file contains constants/settings that may change between each Thingy:91 device.
    These are things such as: ID's, hostnames, serial numbers.
    Do not place any functions in here, only #define statements.
    This file should be changed before programming a new Thingy:91 with the Traffic Light AE so that each AE has a different identifier.
*/

// Location and port of the oneM2M CSE
#define ENDPOINT_HOSTNAME "34.238.135.110"
#define ENDPOINT_PORT 8080

// Originator name
#define M2M_ORIGINATOR "Cthingy91A"

#endif