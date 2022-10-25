#include <stdlib.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include <cJSON.h>


#include "onem2m.h"
#include "deployment_settings.h"
#include "modules/http_module.h"

//
// These are just function stubs based off the code here: https://github.com/BobFIV/PSU_CAPSTONE_F2021/blob/main/sensor_oneM2M/src/main.c
// Feel free to change these however you see fit. I am placing them here just to show the structure of the code/files.
//

char* createACP(char *parentID, char *acpi) {
    return NULL;
}

char* createAE(char* resourceName, char* acpi) {
    return NULL;
}

char* retrieveAE(char* resourceName) {
    return NULL;
}

int deleteAE(char* resourceName) {
    return NULL;
}

char* createContainer(char* resourceName, char* parentID, int mni, char* acpi) {
    return NULL;
}

char* retrieveContainer(char* resourceName, char* parentID) {
    return NULL;
}

char* createFlexContainer(char* resourceName, char* parentID, int mni, char* acpi) {
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