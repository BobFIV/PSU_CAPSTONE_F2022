import axios from "axios";

var resource_type_enum = {
    // Taken from TS-0004, Section 6.3.4.2.1 m2m:resourceType
    // Note: This is incomplete
    ACCESS_CONTROL_POLICY: 1, // ACP
    APPLICATION_ENTITY: 2,    // AE
    CONTAINER: 3,             // CNT
    CONTENT_INSTANCE: 4,
    CSE_BASE: 5,
    GROUP: 9,
    NODE: 14,
    POLLING_CHANNEL: 15,
    FLEX_CONTAINER: 28
};

var cse_type_enum = {
    // TS-0004, Section 6.3.4.2.2 m2m:cseTypeID
    IN_CSE: 1,
    MN_CSE: 2,
    ASN_CSE: 3
};

var operation_enum = {
    // TS-0004, Section 6.3.4.2.5 m2m:operation
    CREATE: 1,
    RETRIEVE: 2,
    UPDATE: 3,
    DELETE: 4,
    NOTIFY: 5
};

var response_type_enum = {
    // TS-0004, Section 6.3.4.2.6 m2m:responseType
    NONBLOCKING_SYNC: 1,
    NONBLOCKING_ASYNC: 2,
    BLOCKING: 3,
    FLEX_BLOCKING: 4,
    NO_RESPONSE: 5
};

function create_random_identifier() {
    // WARNING: This is not a secure PRNG! 
    return btoa(Math.floor(Math.random()*10000000000));
}

class Resource {
    constructor(resource_type, resource_id, resource_name, parent_id) {
        // Based off of the Universal Attributes from: TS-0004, Section 9.6.1.3.1
        this.resource_type = resource_type;
        this.resource_id = resource_id === null ? resource_name + create_random_identifier() : resource_id;
        this.resource_name = resource_name;
        this.parent_id = parent_id;
    }
}

class CSE_Connection extends Resource {
    // Represents both a CSE, and manages the connection to the CSE
    constructor(identifier, originator) {
        super(resource_type_enum.CSE, identifier, "cse-in", "");
        this.client = axios.create({
            headers: {
                "X-M2M-Origin": originator
            }
        });
        this.originator = originator;
    }
};

class ACP extends Resource {
    constructor(acp_name, parent_resource, acp_identifier) {
        var id = acp_identifier === undefined ? null : acp_identifier;
        super(resource_type_enum.ACP, id, acp_name, parent_resource.resource_id);
        console.log(this);
    }

};

class AE extends Resource {
    constructor(ae_name, ae_identifier, app_identifier, acp_ids, parent_resource) {
        super(resource_type_enum.APPLICATION_ENTITY, ae_identifier, ae_name, parent_resource.resource_id);
        this.ae_identifier = ae_identifier;      // aei
        this.app_identifier = app_identifier;    // api
        this.acp_ids = acp_ids;                  // acpi
        this.supported_release_versions = [ 3 ]; // srv
        console.log(this);
    }
};

class Container extends Resource {
    
};

class FlexContainer extends Resource {

};

export { ACP, AE, FlexContainer, Container };
