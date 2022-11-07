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

void init_oneM2M() {
    // Call this at startup
    memset(acpi, 0, ACPI_LENGTH);
    memset(aeurl, 0, aei_LENGTH);
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

char* retrieveAE(char* resourceName) {
    return NULL;
}

int deleteAE(char* resourceName) {
    return 0;
}

char* createContainer(char* resourceName, char* parentID, int mni, char* acpi) {
    return NULL;
}

char* retrieveContainer(char* resourceName, char* parentID) {
    return NULL;
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

    give_http_sem();
    
    LOG_INF("Created Flex Container");

    return NULL;
}

char* retrieveFlexContainer(char* resourceName, char* parentID) {
    return NULL;
}

int createCIN(char* parentID, char* content, char* label) {
    return 0;
}

void retrieveCIN(char* parentID, char* CNTName) {
    return;
}


/*
    // Example code for parsing JSON response from an HTTP response, based off of: https://github.com/DaveGamble/cJSON#parsing-json
    // Example response from web server: { "light1":"yellow", "light2": "red" }

    cJSON *parsed_json = cJSON_ParseWithLength(http_rx_body_start, http_content_length);
    if (parsed_json == NULL)
    {
        LOG_ERR("Failed to parse light state from HTTP response!\nRESPONSE START\n%s\nRESPONSE END", http_rx_body_start);
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            LOG_ERR("Error before: %s\n", error_ptr);
        }
    }
    else {
        const cJSON* light1_json = cJSON_GetObjectItemCaseSensitive(parsed_json, "light1");
        if (cJSON_IsString(light1_json) && (light1_json->valuestring != NULL))
        {
            struct ae_event* light_event = new_ae_event();
            light_event->cmd = AE_EVENT_LIGHT_CMD;
            light_event->target_light = AE_LIGHT1;
            light_event->new_light_state = string_to_light_state(light1_json->valuestring, 10);
            APP_EVENT_SUBMIT(light_event);
        }
        else {
            LOG_ERR("Failed to parse \"light1\" JSON field!");
        }
    }

    cJSON_Delete(parsed_json);

*/