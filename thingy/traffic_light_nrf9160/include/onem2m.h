#ifndef TRAFFIC_LIGHT_NRF9160_ONEM2M_H_
#define TRAFFIC_LIGHT_NRF9160_ONEM2M_H_

void init_oneM2M();

// Access Control Policy (ACP)
void createACP();
bool discoverACP();

// Application Entity (AE)
char* createAE();
bool discoverAE();
int deleteAE(const char* resourceName);

// Flex Container
char* createFlexContainer();
bool discoverFlexContainer();
void retrieveFlexContainer();
bool updateFlexContainer(const char* l1s, const char* l2s, const char* bts);

// Polling Channel (PCH)
void createPCH();
bool discoverPCH();
void performPoll();

// Subscriptions (SUB)
void createSUB();
bool discoverSUB();

#endif // TRAFFIC_LIGHT_NRF9160_ONEM2M_H_