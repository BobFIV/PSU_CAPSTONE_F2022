#ifndef TRAFFIC_LIGHT_NRF9160_ONEM2M_H_
#define TRAFFIC_LIGHT_NRF9160_ONEM2M_H_

//
// These are just function stubs based off the code here: https://github.com/BobFIV/PSU_CAPSTONE_F2021/blob/main/sensor_oneM2M/src/main.c
// Feel free to change these however you see fit. I am placing them here just to show the structure of the code/files.
//

void init_oneM2M();

void createACP();
char* createAE();
char* retrieveAE(char* resourceName);
int deleteAE(char* resourceName);
char* createContainer(char* resourceName, char* parentID, int mni, char* acpi);
char* retrieveContainer(char* resourceName, char* parentID);
char* createFlexContainer();
char* retrieveFlexContainer(char* resourceName, char* parentID);
int createCIN(char* parentID, char* content, char* label);
void retrieveCIN(char* parentID, char* CNTName);


#endif // TRAFFIC_LIGHT_NRF9160_ONEM2M_H_