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
    constructor(resource_type, resource_id, resource_name, parent_id) {
        // Based off of the Universal Attributes from: TS-0004, Section 9.6.1.3.1
        this.resource_type = resource_type;
        this.resource_id = resource_id === null ? resource_name + create_random_identifier() : resource_id;
        this.resource_name = resource_name;
        this.parent_id = parent_id;
    }
}

export class CSE_Connection extends Resource {
    // Represents both a CSE, and manages the connection to the CSE
    constructor() {
        super(resource_type_enum.CSE, "", "cse-in", "");
        this.connected = false;
        this.connect = this.connect.bind(this);
        this.disconnect = this.disconnect.bind(this);

        this.retrieveResource = this.retrieveResource.bind(this);
        this.createResource = this.createResource.bind(this);
        this.updateResource = this.updateResource.bind(this);
        this.discoverResources = this.discoverResources.bind(this);
    }

    discoverResources(resource_type, resource_name) {
        let filters = {};
        if (resource_type) {
            filters["ty"] = resource_type;
        }
        if (resource_name) {
            filters["rn"] = resource_name;
        }

        return axios({
            method: "get",
            headers: {
                "X-M2M-Origin": this.originator,
                "X-M2M-RI": create_random_identifier(),
                "X-M2M-RVI": "3"
            },
            responseType: "json",
            baseURL: this.url,
            url: this.resource_id,
            params: {
                fu: filter_usage_enum.DISCOVERY,
                drt: desired_id_response_type.STRUCTURED,
                ty: filters["ty"],
                rn: filters["rn"]
            }
        }).then((response) => {
            return response.data["m2m:uril"];
        }).catch((error) => {
            console.error("Error while discovering resources!");
            throw error;
        });
    }

    retrieveResource(resource) {
        return axios({
            method: "get",
            headers: {
                "X-M2M-Origin": this.originator,
                "X-M2M-RI": create_random_identifier(),
                "X-M2M-RVI": "3"
            },
            responseType: "json",
            baseURL: this.url,
            url: resource.resource_id
        }).then((response) => {
            return response.data;
        }).catch((error) => {
            console.error("Error while retrieving resource: " + resource.resource_id);
            throw error;
        });
    }

    createResource(resource) {
    
    }
    
    updateResource(resource, new_state) {
        return axios({
            method: "put",
            headers: {
                "X-M2M-Origin": this.originator,
                "X-M2M-RI": create_random_identifier(),
                "X-M2M-RVI": "3"
            },
            responseType: "json",
            baseURL: this.url,
            url: resource.resource_id,
            data: new_state
        }).then((response) => {
            return response.data;
        }).catch((error) => {
            console.error("Error while retrieving resource: " + resource.resource_id);
            throw error;
        });
    
    }

    connect(url, originator, base_ri) {
        this.url = url;
        this.originator = originator;
        this.resource_id = base_ri;
        let discover_promise = this.discoverResources();
        return discover_promise;
    }

    disconnect() {
        this.connected = false;
    }
};

export class ACP extends Resource {
    constructor(acp_name, acp_identifier, parent_resource_id) {
        var id = acp_identifier === undefined ? null : acp_identifier;
        super(resource_type_enum.ACP, id, acp_name, parent_resource_id);
        console.log(this);
    }

};

export class AE extends Resource {
    constructor(ae_name, ae_identifier, app_identifier, acp_ids, parent_resource_id) {
        super(resource_type_enum.APPLICATION_ENTITY, ae_identifier, ae_name, parent_resource_id);
        this.ae_identifier = ae_identifier;      // aei
        this.app_identifier = app_identifier;    // api
        this.acp_ids = acp_ids;                  // acpi
    }
    
    static discover(connection) {
        // TODO: Fill this in using the below FlexContainer as an example
    }
};

export class FlexContainer extends Resource {
    constructor(container_definition, container_type, resource_name, resource_id, parent_resource_id) {
        super(resource_type_enum.FLEX_CONTAINER, resource_id, resource_name, parent_resource_id);
        this.container_definition = container_definition; // reverse FQDN
        this.container_type = container_type;  // namespace:type
    
        this.update = this.update.bind(this);
    }

    static async discover(connection, container_type) {
        // We first have to wait for the list of flex containers
        let data = await connection.discoverResources(resource_type_enum.FLEX_CONTAINER);
        // Now make an array of promises to retrieve all those flex containers
        let discover_flex_container_promises = [];
        
        for (let f_id of data) {
            let p = connection.retrieveResource({ resource_id: f_id }).then((f) => {
                f = f[container_type];
                let flex_con = new FlexContainer(f.cnd, container_type, f.rn, f.ri, f.pi);
                flex_con._raw = f;
                return flex_con;
            });
            discover_flex_container_promises.push(p);
        }
        // Return a single promise that resolves once all of the flex containers are retrieved
        return Promise.all(discover_flex_container_promises); 
    }

    update(connection, new_state) {
        let state_container = {}
        state_container[this.container_type] = new_state;
        connection.updateResource(this, state_container);
    }
};

export default Resource;
