#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <cJSON.h>

#include "onem2m.h"
#include "onem2m_payloads.h"
#include "deployment_settings.h"
#include "modules/http_module.h"
#include "events/ae_event.h"

LOG_MODULE_REGISTER(oneM2M, LOG_LEVEL_INF);

// Global Variables
#define ACPI_LENGTH 40
char acpi[ACPI_LENGTH];

#define aei_LENGTH 50
char aeurl[aei_LENGTH];

#define flexident_LENGTH 50
char flexident[flexident_LENGTH]; 

#define PCH_LENGTH 50
char pchurl[PCH_LENGTH];

#define SUB_LENGTH 50
char suburl[PCH_LENGTH];

#define RQI_LENGTH 50
char rqi_value[RQI_LENGTH];
const char* rqi_header = rqi_value;

#define MAX_ONEM2M_REQUEST_PAYLOAD_SIZE 2048
static char onem2m_request_payload[MAX_ONEM2M_REQUEST_PAYLOAD_SIZE];

#define MAX_ONEM2M_URL_SIZE 200
static char onem2m_url_buffer[MAX_ONEM2M_URL_SIZE];

void init_oneM2M() {
    // Call this at startup
    memset(acpi, 0, ACPI_LENGTH);
    memset(aeurl, 0, aei_LENGTH);
    memset(flexident, 0, flexident_LENGTH);
}

void clear_onem2m_request_payload() {
    memset(onem2m_request_payload, 0, MAX_ONEM2M_REQUEST_PAYLOAD_SIZE);
}

void clear_onem2m_url_buffer() {
    memset(onem2m_url_buffer, 0, MAX_ONEM2M_URL_SIZE);
}

void updateLightStatesFromJSON(const cJSON* flex) {
    struct ae_event* v = new_ae_event();
    v->cmd = AE_EVENT_LIGHT_CMD;
    
    //have the data from the flex container. parse the data out
    //parse light one status
    const cJSON* lOne = cJSON_GetObjectItemCaseSensitive(flex, "l1s");
    if (cJSON_IsString(lOne) && (lOne->valuestring != NULL))
    {
        //Do something with the data
        LOG_INF("Got light 1s status: %s", lOne->valuestring);
        v->new_light1_state = string_to_light_state(lOne->valuestring, strlen(lOne->valuestring));
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
        v->new_light2_state = string_to_light_state(lTwo->valuestring, strlen(lTwo->valuestring));
    }
    else {
        LOG_ERR("Failed to find \"l2s\" JSON field!");
    }
    APP_EVENT_SUBMIT(v);
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
    
    // make post request
    take_http_sem();
    int response_code = post_request(ENDPOINT_HOSTNAME, "/id-in", acp_create_payload, strlen(acp_create_payload), headers);
    if (response_code <= 0) {
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
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"id-in?fu=1&drt=2&ty=1&rn=%s-ACP", M2M_ORIGINATOR);
    int response_code = get_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
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

bool deleteACP() {
    LOG_INF("Delete ACP");

    //create headers for delete request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    take_http_sem();
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"/%s", acpi);
    int response_code = delete_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to delete ACP");
        give_http_sem();
        return false;
    }

    give_http_sem();
    LOG_INF("ACP Deleted");
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

    //Create the payload to send to the ACME server
    clear_onem2m_request_payload();
    sprintf(onem2m_request_payload, ae_create_payload, acpi);
    
    // make post request
    take_http_sem();
    int response_code = post_request(ENDPOINT_HOSTNAME, "/id-in", onem2m_request_payload, strlen(onem2m_request_payload), headers);
    if (response_code <= 0) {
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
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"id-in?fu=1&drt=2&ty=2&rn=intersection%s", DEVICE_LETTER);
    int response_code = get_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
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

bool deleteAE() {
    LOG_INF("Delete AE");

    //create headers for delete request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    take_http_sem();
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"/%s", aeurl);
    int response_code = delete_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to delete AE");
        give_http_sem();
        return false;
    }

    give_http_sem();
    LOG_INF("AE Deleted");
    return true;
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

    //Create the payload to send to the ACME server
    clear_onem2m_request_payload();
    sprintf(onem2m_request_payload, flex_container_create_payload, acpi);

    // make post request
    take_http_sem();
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"/%s", aeurl);
    int response_code = post_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, onem2m_request_payload, strlen(onem2m_request_payload), headers);
    if (response_code <= 0) {
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
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"id-in?fu=1&drt=2&ty=28&pi=%s", aeurl);
    int response_code = get_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
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
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"/%s", flexident);
    int response_code = get_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to check if flex container is already created");
        give_http_sem();
        return;
    }

    //parse the response
    cJSON* j = parse_json_response();
    if (j != NULL) {
        const cJSON* flex = cJSON_GetObjectItemCaseSensitive(j, "traffic:trfint");
        if (cJSON_IsObject(flex))
        {
            updateLightStatesFromJSON(flex);
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
        "X-M2M-RTU: 1\r\n", // RUI = 1 means nonBlockingSync
        NULL};

    //create payload
    clear_onem2m_request_payload();
    sprintf(onem2m_request_payload, "{\
        \"traffic:trfint\": {\
            \"l1s\": \"%s\",\
            \"l2s\": \"%s\",\
            \"bts\": \"%s\"\
        }\
    }", l1s, l2s, bts);

    // make post request
    take_http_sem();
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"/%s?rt=1", flexident);
    int response_code = put_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, onem2m_request_payload, strlen(onem2m_request_payload), headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to update Flex Container!");
        give_http_sem();
        return false;
    }

    give_http_sem();
    return true;
}

bool deleteFLEX() {
    LOG_INF("Delete FLEX");

    //create headers for delete request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    take_http_sem();
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"/%s", flexident);
    int response_code = delete_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to delete FLEX");
        give_http_sem();
        return false;
    }

    give_http_sem();
    LOG_INF("FLEX Deleted");
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
    char* create_pch_payload = "{\"m2m:pch\": {}}";

    take_http_sem();
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"/%s", aeurl);
    int response_code = post_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, create_pch_payload, strlen(create_pch_payload), headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to check if PCH is already created");
        give_http_sem();
        return;
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
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"id-in?fu=1&drt=2&ty=15&pi=%s", aeurl);
    int response_code = get_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
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

bool deletePCH() {
    LOG_INF("Delete PCH");

    //create headers for delete request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    take_http_sem();
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"/%s", pchurl);
    int response_code = delete_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to delete PCH");
        give_http_sem();
        return false;
    }

    give_http_sem();
    LOG_INF("PCH Deleted");
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
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"/%s", flexident);
    int response_code = post_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, payload, strlen(payload), headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to check if SUB is already created");
        give_http_sem();
        return;
    }

    //get sub url form creation  response
    cJSON* j = parse_json_response();
    if (j != NULL) {
        const cJSON* acp = cJSON_GetObjectItemCaseSensitive(j, "m2m:sub");
        if (cJSON_IsObject(acp))
        {
            const cJSON* ri = cJSON_GetObjectItemCaseSensitive(acp, "ri");
            if (cJSON_IsString(ri) && (ri->valuestring != NULL))
            {
                // Copy the acpi from the JSON into the location pointed to
                strncpy(suburl, ri->valuestring, SUB_LENGTH);
                LOG_INF("Created SUB, suburl=%s", suburl);
            }
            else {
                LOG_ERR("Failed to find \"ri\" JSON field!");
            }
        }
        else {
            LOG_ERR("Failed to find \"m2m:sub\" JSON field!");
        }
        
    }


    free_json_response(j);
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
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"id-in?fu=1&drt=2&ty=23&rn=%sSUB", M2M_ORIGINATOR);
    int response_code = get_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to check if SUB is already created");
        give_http_sem();
        return false;
    }

     cJSON* j = parse_json_response();
    if (j != NULL) {
        //the SUB was found. take the information we need from it
        const cJSON* ae = cJSON_GetObjectItemCaseSensitive(j, "m2m:uril");
        if (cJSON_GetArraySize(ae) != 0)
        {
            const cJSON* aei = cJSON_GetArrayItem(ae, 0);
            if (cJSON_IsString(aei) && (aei->valuestring != NULL))
            {
                // Copy the acpi from the JSON into the location pointed to
                strncpy(suburl, &aei->valuestring[7], SUB_LENGTH);
                LOG_INF("Found SUB identity, suburl=%s", suburl);
            }
            else {
                LOG_ERR("Failed to find \"ri\" JSON field!");
            }
        }
        else {
            LOG_INF("There is no matching SUB found");
            free_json_response(j);
            give_http_sem();
            return false;
        }
        
    }

    free_json_response(j);
    give_http_sem();
    return true;
}

bool deleteSUB() {
    LOG_INF("Delete SUB");

    //create headers for delete request
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: o4d3qpiix6p\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};

    take_http_sem();
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer,"/%s", suburl);
    int response_code = delete_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to delete SUB");
        give_http_sem();
        return false;
    }

    give_http_sem();
    LOG_INF("SUB Deleted");
    return true;
}

void onem2m_performPoll() {
    const char* headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        "X-M2M-RI: a34sds2efw4rg6r\r\n",
        "X-M2M-RVI: 3\r\n",
        NULL};
    
    //create URL 
    take_http_sem();
    clear_onem2m_url_buffer();
    sprintf(onem2m_url_buffer, "%s/pcu", pchurl);
    int response_code = get_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to poll PCH!");
        give_http_sem();
        return;
    }

    if (response_code == 504) {
        // Response timed out, nothing to update
        give_http_sem();
        return;
    }

    cJSON* j = parse_json_response();
    if (j != NULL) {
        const cJSON* rqp = cJSON_GetObjectItemCaseSensitive(j, "m2m:rqp");
        if (!cJSON_IsObject(rqp)) {
            LOG_INF("Failed to get m2m:rqp from PCH response!");
            free_json_response(j);
            give_http_sem();
            return;
        }

        const cJSON* rqi = cJSON_GetObjectItemCaseSensitive(rqp, "rqi");
        if (cJSON_IsString(rqi) && (rqi->valuestring != NULL)) {
            // Copy in the request id (rqi)
            strncpy(rqi_value, rqi->valuestring, RQI_LENGTH);
            LOG_INF("Got rqi: %s", rqi_value);
        } 
        else {
            LOG_INF("Failed to get rqi from PCH response!");
            free_json_response(j);
            give_http_sem();
            return;
        }

        //pc.m2m:sgn.nev.rep.traffic:trfint
        //
        const cJSON* pc = cJSON_GetObjectItemCaseSensitive(rqp, "pc");
        if (cJSON_IsObject(pc)) {
            const cJSON* sgn = cJSON_GetObjectItemCaseSensitive(pc, "m2m:sgn");
            if (cJSON_IsObject(sgn)) {
                const cJSON* nev = cJSON_GetObjectItemCaseSensitive(sgn, "nev");
                if (cJSON_IsObject(nev)) {
                    const cJSON* rep = cJSON_GetObjectItemCaseSensitive(nev, "rep");
                    if (cJSON_IsObject(rep)) {
                        const cJSON* flex = cJSON_GetObjectItemCaseSensitive(rep, "traffic:trfint");
                        if (cJSON_IsObject(flex)) {
                                updateLightStatesFromJSON(flex);   
                        }
                        else {
                            LOG_ERR("Failed to get m2m:rqp.pc.m2m:sgn.nev.rep.traffic:trfint from JSON!");
                        } 
                    }
                    else {
                        LOG_ERR("Failed to get m2m:rqp.pc.m2m:sgn.nev.rep from JSON!");
                    }  
                }
                else {
                    LOG_ERR("Failed to get m2m:rqp.pc.m2m:sgn.nev from JSON!");
                }   
            }
            else {
                LOG_ERR("Failed to get m2m:rqp.pc.m2m:sgn from JSON!");
            }
        }
        else {
            LOG_ERR("Failed to get m2m:rqp.pc from JSON!");
        }
    }
    else {
        free_json_response(j);
        give_http_sem();
        return;
    }

    // Send an acknowledge to the notification we received
    LOG_INF("Acknowledging PCH rqi: %s", rqi_value);
    char m2m_ri_echo[63];
    memset(m2m_ri_echo, 0, 63);
    sprintf(m2m_ri_echo, "X-M2M-RI: %s\r\n", rqi_value);
    const char* echo_headers[] = {
        "Content-Type: application/json\r\n",
        "Accept: application/json\r\n",
        "X-M2M-Origin: " M2M_ORIGINATOR "\r\n", 
        m2m_ri_echo,
        "X-M2M-RVI: 3\r\n",
    NULL};

    char pc_echo[601];
    memset(pc_echo,0,601);
    // Note: At this point, rqp HAS to exist b/c we just retrieved it to update the lights
    const cJSON* rqp = cJSON_GetObjectItemCaseSensitive(j, "m2m:rqp");
    cJSON* pc = cJSON_GetObjectItemCaseSensitive(rqp, "pc");
    if(!cJSON_PrintPreallocated(pc, pc_echo, 600, false)) {
        LOG_ERR("Ran out of space in buffer while printing JSON for PCH response!");
        free_json_response(j);
        give_http_sem();
        return;
    }
    clear_onem2m_request_payload();
    sprintf(onem2m_request_payload, pch_ack_payload, rqi_value, pc_echo);

    response_code = post_request(ENDPOINT_HOSTNAME, onem2m_url_buffer, onem2m_request_payload, strlen(onem2m_request_payload), echo_headers);
    if (response_code <= 0) {
        LOG_ERR("Failed to echo PCH notification!");
        free_json_response(j);
        give_http_sem();
        return;
    }
    
    free_json_response(j);
    give_http_sem();
    return;
}

