export var RESOURCE_TYPES = {
    "m2m:acp": 1,
    "m2m:ae": 2,
    "m2m:cnt": 3,
    "m2m:cb": 5,
    "m2m:grp": 9,
    "m2m:nod": 14,
    "m2m:pch": 15,
    "m2m:sub": 23,
    "m2m:flex_container": 28 // NOTE: This is not the true string representation!
};

export var access_control_operation_enum = {
    //TS-0004, Section 6.3.4.2.29-1 m2m:accessControlOperations
    CREATE: 1,
    RETRIEVE: 2,
    UPDATE: 4,
    DELETE: 8,
    NOTIFY: 16,
    DISCOVERY: 42
};

export var notification_event_type_enum = {
    // TS-0004, Section 6.3.4.2.19-1 m2m:notificationEventType
    UPDATE: 1,
    DELETE: 2,
    UPDATE_CHILD: 3,
    DELETE_CHILD: 4,
    RETRIEVE_CONTAINER_NO_CHILD: 5,
    TRIGGER_RECIEVED_FOR_AE: 6,
    BLOCKING_UPDATE: 7,
    REPORT_MISSING_DATA_POINTS: 8
};

export var cse_type_enum = {
    // TS-0004, Section 6.3.4.2.2 m2m:cseTypeID
    IN_CSE: 1,
    MN_CSE: 2,
    ASN_CSE: 3
};

export var operation_enum = {
    // TS-0004, Section 6.3.4.2.5 m2m:operation
    CREATE: 1,
    RETRIEVE: 2,
    UPDATE: 3,
    DELETE: 4,
    NOTIFY: 5
};

export var response_type_enum = {
    // TS-0004, Section 6.3.4.2.6 m2m:responseType
    NONBLOCKING_SYNC: 1,
    NONBLOCKING_ASYNC: 2,
    BLOCKING: 3,
    FLEX_BLOCKING: 4,
    NO_RESPONSE: 5
};

export var filter_usage_enum = {
    // TS-0004, Section 6.3.4.2.31-1 m2m:filterUsage
    DISCOVERY: 1,
    CONDITIONAL: 2,
    ON_DEMAND: 3
};

export var desired_id_response_type_enum = {
    // TS-0004: Section 6.3.4.2.8-1 m2m:desIdResType
    STRUCTURED: 1,
    UNSTRUCTURED: 2
};

export function create_random_identifier() {
    // WARNING: This is not a secure PRNG! 
    return btoa(Math.floor(Math.random()*10000000000));
}
export default RESOURCE_TYPES;
