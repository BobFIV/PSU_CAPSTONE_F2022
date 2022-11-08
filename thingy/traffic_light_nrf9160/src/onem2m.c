#include <stdlib.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <cJSON.h>

#include "onem2m.h"
#include "deployment_settings.h"
#include "modules/http_module.h"

LOG_MODULE_REGISTER(oneM2M, LOG_LEVEL_INF);

// Global Variables
#define ACPI_LENGTH 40
char acpi[ACPI_LENGTH];

#define aei_LENGTH 50
char aeurl[aei_LENGTH];

#define flexident_LENGTH 100
char flexident[flexident_LENGTH]; 

#define PCH_LENGTH 100
char pchurl[PCH_LENGTH];

void init_oneM2M() {
    // Call this at startup
    memset(acpi, 0, ACPI_LENGTH);
    memset(aeurl, 0, aei_LENGTH);
    memset(flexident, 0, flexident_LENGTH);
}

void createACP() {
    /// @brief Attempts to create an ACP on the CSE
    LOG_INF("Creating ACP");
    
    //create headers needed for the creation of ACP
    const char* headers[] = {
        "Content-Type: application/json;ty=1\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    //Create the payload to send to the ACME server
    char payload[] = "{\
        \"m2m:acp\": { \
            \"rn\": \"" M2M_ORIGINATOR "-ACP\",\
            \"pv\": {\
                \"acr\": [\
                    {\"acor\": [\"Cdashboard\"], \"acop\": 63},\
                    {\"acor\": [\"" M2M_ORIGINATOR "\"], \"acop\": 63}\
                ]\
            },\
            \"pvs\": {\
                \"acr\": [\
                    {\"acor\": [\"Cdashboard\"], \"acop\": 63},\
                    {\"acor\": [\"" M2M_ORIGINATOR "\"], \"acop\": 63}\
                ]\
            }\
        }\
    }";
    
    // make post request
    take_http_sem();
    int response_length = post_request(ENDPOINT_HOSTNAME, "/id-in", payload, strlen(payload), headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to create ACP!");
        give_http_sem();
        return;
    }

    cJSON* j = parse_json_response();
    if (j != NULL) {
        const cJSON* acp = cJSON_GetObjectItemCaseSensitive(j, "m2m:acp");
        if (cJSON_IsObject(acp))
        {
            const cJSON* ri = cJSON_GetObjectItemCaseSensitive(acp, "ri");
            if (cJSON_IsString(ri) && (ri->valuestring != NULL))
            {
                // Copy the acpi from the JSON into the location pointed to
                strncpy(acpi, ri->valuestring, ACPI_LENGTH);
                LOG_INF("Created ACP, acpi=%s", acpi);
            }
            else {
                LOG_ERR("Failed to find \"ri\" JSON field!");
            }
        }
        else {
            LOG_ERR("Failed to find \"m2m:acp\" JSON field!");
        }
        
    }
    free_json_response(j);
    give_http_sem();
}

bool discoverACP() {
    LOG_INF("Checking to see if ACP is already created");

    //create headers for get request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};
    
    //create URL 
    take_http_sem();
    char URL[51];
    memset(URL, 0, 51);
    sprintf(URL,"id-in?fu=1&drt=2&ty=1&rn=%s-ACP", M2M_ORIGINATOR);
    int response_length = get_request(ENDPOINT_HOSTNAME, URL, headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to check if ACP is already created");
        give_http_sem();
        return NULL;
    }

    cJSON* j = parse_json_response();
    if (j != NULL) {
        //the AE was found. take the information we need from it
        const cJSON* ae = cJSON_GetObjectItemCaseSensitive(j, "m2m:uril");
        if (cJSON_GetArraySize(ae) != 0)
        {
            const cJSON* ri = cJSON_GetArrayItem(ae, 0);
            if (cJSON_IsString(ri) && (ri->valuestring != NULL))
            {
                // Copy the acpi from the JSON into the location pointed to
                strncpy(acpi, &ri->valuestring[7], ACPI_LENGTH);
                LOG_INF("Found acpi, acpi=%s", acpi);
            }
            else {
                LOG_ERR("Failed to find \"acpi\" JSON field!");
            }
        }
        else {
            LOG_INF("There is no matching ACP found");
            free_json_response(j);
            give_http_sem();
            return false;
        }
        
    }
    free_json_response(j);
    give_http_sem();
    return true;
}

char* createAE() {
    LOG_INF("Creating AE");
    //create headers needed for the creation of AE
    const char* headers[] = {
        "Content-Type: application/json;ty=2\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    char payload[400];
    memset(payload, 0, 400);
    //Create the payload to send to the ACME server
    sprintf(payload, "{\
        \"m2m:ae\": {\
            \"acpi\": [\
                \"%s\"\
            ],\
            \"api\": \"NtrafficAPI\",\
            \"rn\": \"intersection" DEVICE_LETTER "\",\
            \"srv\": [\
                \"3\"\
            ],\
            \"rr\": false\
        }\
    }", acpi);
    
    // make post request
    take_http_sem();
    int response_length = post_request(ENDPOINT_HOSTNAME, "/id-in", payload, strlen(payload), headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to create AE!");
        give_http_sem();
        return NULL;
    }

    cJSON* j = parse_json_response();
    if (j != NULL) {
        const cJSON* ae = cJSON_GetObjectItemCaseSensitive(j, "m2m:ae");
        if (cJSON_IsObject(ae))
        {
            const cJSON* aei = cJSON_GetObjectItemCaseSensitive(ae, "aei");
            if (cJSON_IsString(aei) && (aei->valuestring != NULL))
            {
                // Copy the acpi from the JSON into the location pointed to
                strncpy(aeurl, aei->valuestring, aei_LENGTH);
                LOG_INF("Created AE, aei=%s", aeurl);
            }
            else {
                LOG_ERR("Failed to find \"aei\" JSON field!");
            }
        }
        else {
            LOG_ERR("Failed to find \"m2m:acp\" JSON field!");
        }
        
    }
    free_json_response(j);
    give_http_sem();
    LOG_INF("Created AE");
    return NULL;
}

bool discoverAE() {
    LOG_INF("Checking to see if AE is already created");

    //create headers for get request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    take_http_sem();
    //create URL 
    char URL[51];
    memset(URL, 0, 51);
    sprintf(URL,"id-in?fu=1&drt=2&ty=2&rn=intersection%s", DEVICE_LETTER);
    int response_length = get_request(ENDPOINT_HOSTNAME, URL, headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to check if AE is already created");
        give_http_sem();
        return NULL;
    }

    cJSON* j = parse_json_response();
    if (j != NULL) {
        //the AE was found. take the information we need from it
        const cJSON* ae = cJSON_GetObjectItemCaseSensitive(j, "m2m:uril");
        if (cJSON_GetArraySize(ae) != 0)
        {
            const cJSON* aei = cJSON_GetArrayItem(ae, 0);
            if (cJSON_IsString(aei) && (aei->valuestring != NULL))
            {
                // Copy the acpi from the JSON into the location pointed to
                strncpy(aeurl, &aei->valuestring[7], aei_LENGTH);
                LOG_INF("Found AE, ae=%s", aeurl);
            }
            else {
                LOG_ERR("Failed to find \"aei\" JSON field!");
            }
        }
        else {
            LOG_INF("There is no matching AE found");
            free_json_response(j);
            give_http_sem();
            return false;
        }
    }
    free_json_response(j);
    give_http_sem();
    return true;
}

int deleteAE(const char* resourceName) {
    return 0;
}

char* createFlexContainer() {
    LOG_INF("Creating Flex Container");
    
    //create headers needed for the creation of AE
    const char* headers[] = {
        "Content-Type: application/json;ty=28\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};


    char payload[400];
    memset(payload, 0, 400);
    //Create the payload to send to the ACME server
    sprintf(payload, "{\
        \"traffic:trfint\": {\
            \"acpi\": [\
                \"%s\"\
            ],\
            \"cnd\": \"edu.psu.cse.traffic.trafficLightIntersection\",\
            \"rn\": \"intersection\",\
            \"l1s\": \"red\",\
            \"l2s\": \"red\",\
            \"bts\": \"disconnected\"\
        }\
    }", acpi);

    // make post request
    take_http_sem();
    char flexURL[51];
    memset(flexURL, 0, 51);
    sprintf(flexURL,"/%s", aeurl);
    int response_length = post_request(ENDPOINT_HOSTNAME, flexURL, payload, strlen(payload), headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to create Flex Container!");
        give_http_sem();
        return NULL;
    }

    cJSON* j = parse_json_response();
    if (j != NULL) {
        const cJSON* flex = cJSON_GetObjectItemCaseSensitive(j, "traffic:trfint");
        if (cJSON_IsObject(flex))
        {
            const cJSON* ri = cJSON_GetObjectItemCaseSensitive(flex, "ri");
            if (cJSON_IsString(ri) && (ri->valuestring != NULL))
            {
                // Copy the acpi from the JSON into the location pointed to
                strncpy(flexident, ri->valuestring, flexident_LENGTH);
                LOG_INF("Created Flex Container, Flex Contationer Identifier=%s", flexident);
            }
            else {
                LOG_ERR("Failed to find \"ri\" JSON field!");
            }
        }
        else {
            LOG_ERR("Failed to find \"traffic:trfint\" JSON field!");
        }
        
    }
    free_json_response(j);
    give_http_sem();
    LOG_INF("Created Flex Container");
    return NULL;
}

bool discoverFlexContainer() {
    LOG_INF("Checking to see if flex container is already created");

    //create headers for get request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};
    
    //create URL 
    take_http_sem();
    char URL[51];
    memset(URL, 0, 51);
    sprintf(URL,"id-in?fu=1&drt=2&ty=28&pi=%s", aeurl);
    int response_length = get_request(ENDPOINT_HOSTNAME, URL, headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to check if flex container is already created");
        give_http_sem();
        return NULL;
    }

    cJSON* j = parse_json_response();
    if (j != NULL) {
        //the AE was found. take the information we need from it
        const cJSON* ae = cJSON_GetObjectItemCaseSensitive(j, "m2m:uril");
        if (cJSON_GetArraySize(ae) != 0)
        {
            const cJSON* aei = cJSON_GetArrayItem(ae, 0);
            if (cJSON_IsString(aei) && (aei->valuestring != NULL))
            {
                // Copy the acpi from the JSON into the location pointed to
                strncpy(flexident, &aei->valuestring[7], flexident_LENGTH);
                LOG_INF("Found flex container identity, acpi=%s", flexident);
            }
            else {
                LOG_ERR("Failed to find \"ri\" JSON field!");
            }
        }
        else {
            LOG_INF("There is no matching flex container found");
            free_json_response(j);
            give_http_sem();
            return false;
        }
    }
    free_json_response(j);
    give_http_sem();
    return true;
}

void retrieveFlexContainer() {
    LOG_INF("getting contents of the flex container");

    //need to create headers for the get request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};
    
    //create a url that targest the flex container
    take_http_sem();
    char URL[51];
    memset(URL, 0, 51);
    sprintf(URL,"/%s", flexident);
    int response_length = get_request(ENDPOINT_HOSTNAME, URL, headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to check if flex container is already created");
        give_http_sem();
        return NULL;
    }

    //parse the response
    cJSON* j = parse_json_response();
    if (j != NULL) {
        const cJSON* flex = cJSON_GetObjectItemCaseSensitive(j, "traffic:trfint");
        if (cJSON_IsObject(flex))
        {
            //have the data from the flex container. parse the data out
            //parse light one status
            const cJSON* lOne = cJSON_GetObjectItemCaseSensitive(flex, "l1s");
            if (cJSON_IsString(lOne) && (lOne->valuestring != NULL))
            {
                //Do something with the data
                LOG_INF("Got light 1s status: %s", lOne->valuestring);

                
            }
            else {
                LOG_ERR("Failed to find \"l1s\" JSON field!");
            }


            //parse light two status
            const cJSON* lTwo = cJSON_GetObjectItemCaseSensitive(flex, "l2s");
            if (cJSON_IsString(lTwo) && (lTwo->valuestring != NULL))
            {
                //Do something with the data
                LOG_INF("Got light 2s status: %s", lTwo->valuestring);

                
            }
            else {
                LOG_ERR("Failed to find \"l2s\" JSON field!");
            }
        }
        else {
            LOG_ERR("Failed to find \"traffic:trfint\" JSON field! In Get function");
        }
    }
    free_json_response(j);
    give_http_sem();
    return;
}

bool updateFlexContainer(const char* l1s, const char* l2s, const char* bts) {
    LOG_INF("Updating Flex Container");

    //need to create headers for the get request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    //create payload
    char payload[400];
    memset(payload, 0, 400);
    sprintf(payload, "{\
        \"traffic:trfint\": {\
            \"l1s\": \"%s\",\
            \"l2s\": \"%s\",\
            \"bts\": \"%s\"\
        }\
    }", l1s, l2s, bts);

    // make post request
    take_http_sem();
    char URL[51];
    memset(URL, 0, 51);
    sprintf(URL,"/%s", flexident);
    int response_length = put_request(ENDPOINT_HOSTNAME, URL, payload, strlen(payload), headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to update Flex Container!");
        give_http_sem();
        return false;
    }

    give_http_sem();
    return true;
}

void createPCH() {
    /// @brief Attempts to create an PCH on the CSE
    LOG_INF("Creating PCH");
    
    //create headers needed for the creation of ACP
    const char* headers[] = {
        "Content-Type: application/json;ty=15\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    //create payload
    char payload[400];
    memset(payload, 0, 400);
    sprintf(payload, "{\
        \"m2m:pch\": {\
        }\
    }");

    take_http_sem();
    char URL[51];
    memset(URL, 0, 51);
    sprintf(URL,"/%s", aeurl);
    int response_length = post_request(ENDPOINT_HOSTNAME, URL, payload, strlen(payload), headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to check if PCH is already created");
        give_http_sem();
        return NULL;
    }
    
    cJSON* j = parse_json_response();
    if (j != NULL) {
        const cJSON* acp = cJSON_GetObjectItemCaseSensitive(j, "m2m:pch");
        if (cJSON_IsObject(acp))
        {
            const cJSON* ri = cJSON_GetObjectItemCaseSensitive(acp, "ri");
            if (cJSON_IsString(ri) && (ri->valuestring != NULL))
            {
                // Copy the acpi from the JSON into the location pointed to
                strncpy(pchurl, ri->valuestring, PCH_LENGTH);
                LOG_INF("Created PCH, pchurl=%s", pchurl);
            }
            else {
                LOG_ERR("Failed to find \"ri\" JSON field!");
            }
        }
        else {
            LOG_ERR("Failed to find \"m2m:pch\" JSON field!");
        }
        
    }
    free_json_response(j);

    give_http_sem();
    return;
}

bool discoverPCH() {
    LOG_INF("Checking to see if PCH is already created");

    //create headers for get request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};
    
    //create URL 
    take_http_sem();
    char URL[51];
    memset(URL, 0, 51);
    sprintf(URL,"id-in?fu=1&drt=2&ty=15&pi=%s", aeurl);
    int response_length = get_request(ENDPOINT_HOSTNAME, URL, headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to check if PCH is already created");
        give_http_sem();
        return NULL;
    }

    cJSON* j = parse_json_response();
    if (j != NULL) {
        //the AE was found. take the information we need from it
        const cJSON* ae = cJSON_GetObjectItemCaseSensitive(j, "m2m:uril");
        if (cJSON_GetArraySize(ae) != 0)
        {
            const cJSON* aei = cJSON_GetArrayItem(ae, 0);
            if (cJSON_IsString(aei) && (aei->valuestring != NULL))
            {
                // Copy the acpi from the JSON into the location pointed to
                strncpy(pchurl, &aei->valuestring[7], PCH_LENGTH);
                LOG_INF("Found PCH identity, pchurl=%s", pchurl);
            }
            else {
                LOG_ERR("Failed to find \"ri\" JSON field!");
            }
        }
        else {
            LOG_INF("There is no matching PCH found");
            free_json_response(j);
            give_http_sem();
            return false;
        }
        
    }
    free_json_response(j);
    give_http_sem();
    return true;
}

void createSUB() {
    /// @brief Attempts to create an SUB on the CSE
    LOG_INF("Creating SUB");
    
    //create headers needed for the creation of ACP
    const char* headers[] = {
        "Content-Type: application/json;ty=23\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    //create payload
    char payload[400];
    memset(payload, 0, 400);
    sprintf(payload, "{\
        \"m2m:sub\": {\
            \"acpi\": [\
                \"%s\"\
            ],\
            \"nu\": [\
                \"%s\"\
            ],\
            \"rn\": \"%sSUB\",\
            \"nct\": 1,\
            \"enc\": {\
                \"net\": [\
                    1\
                ]\
            }\
        }\
    }", acpi, M2M_ORIGINATOR, M2M_ORIGINATOR);

    take_http_sem();
    char URL[51];
    memset(URL, 0, 51);
    sprintf(URL,"/%s", flexident);
    int response_length = post_request(ENDPOINT_HOSTNAME, URL, payload, strlen(payload), headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to check if SUB is already created");
        give_http_sem();
        return NULL;
    }

    give_http_sem();
    LOG_INF("Created SUB");
    return;
}

bool discoverSUB() {
    LOG_INF("Checking to see if SUB is already created");

    //create headers for get request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};
    
    //create URL 
    take_http_sem();
    char URL[51];
    memset(URL, 0, 51);
    sprintf(URL,"id-in?fu=1&drt=2&ty=23&rn=%sSUB", M2M_ORIGINATOR);
    int response_length = get_request(ENDPOINT_HOSTNAME, URL, headers);
    if (response_length <= 0) {
        LOG_ERR("Failed to check if SUB is already created");
        give_http_sem();
        return NULL;
    }

    cJSON* j = parse_json_response();
    if (j != NULL) {
        //the AE was found. take the information we need from it
        const cJSON* ae = cJSON_GetObjectItemCaseSensitive(j, "m2m:uril");
        if (cJSON_GetArraySize(ae) != 0)
        {
            LOG_INF("There is SUB");
            free_json_response(j);
            give_http_sem();
            return true;
        }
        
    }
    free_json_response(j);
    give_http_sem();
    LOG_INF("There is NO SUB");
    return false;
}