#ifndef TRAFFIC_LIGHT_NRF9160_ONEM2M_H_
#define TRAFFIC_LIGHT_NRF9160_ONEM2M_H_

void init_oneM2M();

// Access Control Policy (ACP)
void createACP();
bool discoverACP();
bool deleteACP();

// Application Entity (AE)
char* createAE();
bool discoverAE();
bool deleteAE();

// Flex Container
char* createFlexContainer();
bool discoverFlexContainer();
bool deleteFLEX();
void retrieveFlexContainer();
bool updateFlexContainer(const char* l1s, const char* l2s, const char* bts);

// Polling Channel (PCH)
void createPCH();
bool discoverPCH();
bool deletePCH();
void onem2m_performPoll();

// Subscriptions (SUB)
void createSUB();
bool discoverSUB();
bool deleteSUB();

#endif // TRAFFIC_LIGHT_NRF9160_ONEM2M_H_