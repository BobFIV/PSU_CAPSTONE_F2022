#ifndef TRAFFIC_LIGHT_NRF9160_ONEM2M_PAYLOADS_H_
#define TRAFFIC_LIGHT_NRF9160_ONEM2M_PAYLOADS_H_

/*
    This file holds oneM2M request payloads.
    Helps clear up the oneM2M.c file.
*/

#include "deployment_settings.h"

static char* acp_create_payload = "{\
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

static char* ae_create_payload = "{\
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
}";

static char* flex_container_create_payload = "{\
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
}";

static char* pch_ack_payload = "{\"m2m:rsp\":{\"rqi\": \"%s\", \"pc\": %s, \"rsc\": 2004, \"rvi\": \"3\" }}";

#endif // TRAFFIC_LIGHT_NRF9160_ONEM2M_PAYLOADS_H_