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

var RESOURCE_TYPES = {
    "m2m:acp": 1,
    "m2m:ae": 2,
    "m2m:cnt": 3,
    "m2m:cb": 5,
    "m2m:grp": 9,
    "m2m:nod": 14,
    "m2m:sub": 23,
    "m2m:flex_container": 28 // NOTE: This is not the true string representation!
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

var filter_usage_enum = {
    // TS-0004, Section 6.3.4.2.31-1 m2m:filterUsage
    DISCOVERY: 1,
    CONDITIONAL: 2,
    ON_DEMAND: 3
};

var desired_id_response_type = {
    // TS-0004: Section 6.3.4.2.8-1 m2m:desIdResType
    STRUCTURED: 1,
    UNSTRUCTURED: 2
};

function create_random_identifier() {
    // WARNING: This is not a secure PRNG! 
    return btoa(Math.floor(Math.random()*10000000000));
}

export class Resource {
    constructor(resource_type, resource_id) {
        // Based off of the Universal Attributes from: TS-0004, Section 9.6.1.3.1
        this.resource_type = resource_type;
        this.resource_type_enum = RESOURCE_TYPES[this.resource_type];
        this.resource_id = resource_id === null ? create_random_identifier() : resource_id;
        this.resource_name = "";
        this.parent_id = "";
        this.last_modified_time = "";
        this.creation_time = "";        
        this.expiration_time = "";

        this.retrieve = this.retrieve.bind(this);
        this.create = this.create.bind(this);
        this.update = this.update.bind(this);
    }
    
    static discover(connection, filters) {
        let get_params = {
                fu: filter_usage_enum.DISCOVERY,
                drt: desired_id_response_type.STRUCTURED,
        };

        // Apply extra filters if there are any
        if (filters) {
            get_params = Object.assign({}, get_params, filters);
        }

        return axios({
            method: "get",
            headers: {
                "X-M2M-Origin": connection.originator,
                "X-M2M-RI": create_random_identifier(),
                "X-M2M-RVI": "3"
            },
            responseType: "json",
            baseURL: connection.url,
            url: connection.resource_id,
            params: get_params
        }).then((response) => {
            return response.data["m2m:uril"];
        }).catch((error) => {
            console.error("Error while discovering resources!");
            throw error;
        });
    }

    retrieve(connection) {
        return axios({
            method: "get",
            headers: {
                "X-M2M-Origin": connection.originator,
                "X-M2M-RI": create_random_identifier(),
                "X-M2M-RVI": "3"
            },
            responseType: "json",
            baseURL: connection.url,
            url: this.resource_id
        }).then((response) => {
            response = response.data[this.resource_type];
            this.last_modified_time = response.lt;
            this.creation_time = response.ct;
            this.expiration_time = response.et;
            this.resource_name = response.rn;
            this.parent_id = response.pi;
            return response;
        }).catch((error) => {
            console.error("Error while retrieving resource: " + this.resource_id);
            throw error;
        });
    }

    create(connection) {
        return axios({
            method: "post",
            headers: {
                "X-M2M-Origin": connection.originator,
                "X-M2M-RI": create_random_identifier(),
                "X-M2M-RVI": "3",
                "Content-Type": "application/json;ty=" + this.resource_type_enum
            },
            responseType: "json",
            baseURL: connection.url,
            url: this.resource_id
        }).then((response) => {
            response = response.data[this.resource_type];
            this.last_modified_time = response.lt;
            this.creation_time = response.ct;
            this.expiration_time = response.et;
            this.resource_name = response.rn;
            this.parent_id = response.pi;
            return response;
        }).catch((error) => {
            console.error("Error while creating resource: " + this.resource_id);
            throw error;
        });
    }
    
    update(connection, new_state) {
        let wrapped_new_state = {};
        wrapped_new_state[this.resource_type] = new_state;
        return axios({
            method: "put",
            headers: {
                "X-M2M-Origin": connection.originator,
                "X-M2M-RI": create_random_identifier(),
                "X-M2M-RVI": "3"
            },
            responseType: "json",
            baseURL: connection.url,
            url: this.resource_id,
            data: wrapped_new_state
        }).then((response) => {
            return response.data[this.resource_type];
        }).catch((error) => {
            console.error("Error while updating resource: " + this.resource_id);
            throw error;
        });
    }
}

export class CSE_Connection extends Resource {
    // Represents both a CSE, and manages the connection to the CSE
    constructor() {
        super("m2m:cb", "cse-in");
        this.connected = false;
        this.connect = this.connect.bind(this);
        this.disconnect = this.disconnect.bind(this);
    }

    connect(url, originator, base_ri) {
        this.url = url;
        this.originator = originator;
        this.resource_id = base_ri;
        return Resource.discover(this).then((response) => {
            this.connected = true;
            return response;
        });
    }

    disconnect() {
        this.connected = false;
    }
};

export class ACP extends Resource {
    constructor(acp_name, acp_identifier) {
        var id = acp_identifier === undefined ? null : acp_identifier;
        super("m2m:acp", id, acp_name);
        console.log(this);
    }

};

export class AE extends Resource {
    constructor(ae_identifier) {
        super("m2m:ae", ae_identifier);
        this.ae_identifier = ae_identifier;      // aei
        this.app_identifier = "";                // api
        this.acp_ids = [];                       // acpi
    }
    
};

export class FlexContainer extends Resource {
    constructor(container_type, resource_id) {
        super("m2m:flex_container", resource_id);
        // NOTE: The below resource_type_enum is normally set by the Resource class, but we 
        // must override it b/c of specialized flex container type
        this.resource_type = container_type;
        this.container_definition = "";
    
        this.retrieve = this.retrieve.bind(this);
        this.update = this.update.bind(this);
    }

    static discover(connection, filters) {
        filters = filters || {};
        return super.discover(connection, 
            Object.assign({}, filters, {ty: RESOURCE_TYPES["m2m:flex_container"]}));
    }

    retrieve(connection) {
        return super.retrieve(connection).then((response) => {
            this.container_definition = response.cnd;
            return response;
        });
    }

    update(connection, new_state) {
        return super.update(connection, new_state);
    }
};

export default Resource;
