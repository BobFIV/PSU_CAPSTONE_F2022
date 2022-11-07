import axios from "axios";
import { RESOURCE_TYPES, create_random_identifier } from "./onem2m_constants.js";
import { filterUsage, discResType, notificationEventType } from "./onem2m_enums.js";

export class Resource {
    constructor(resource_type, resource_id, resource_name) {
        // Based off of the Universal Attributes from: TS-0004, Section 9.6.1.3.1
        this.resource_type = resource_type;
        this.resource_type_enum = RESOURCE_TYPES[this.resource_type];
        this.resource_id = resource_id === null ? create_random_identifier() : resource_id;
        this.resource_name = resource_name === null ? undefined : resource_name;
        this.parent_id = "";
        this.last_modified_time = "";
        this.creation_time = "";        
        this.expiration_time = "";

        this.retrieve = this.retrieve.bind(this);
        this.create = this.create.bind(this);
        this.update = this.update.bind(this);
        this.handle_update_notification = this.handle_update_notification.bind(this);
        this._resource_refresh_from_response = this._resource_refresh_from_response.bind(this);
        this._resource_to_request = this._resource_to_request.bind(this);
    }

    _resource_refresh_from_response(response) {
        if(response.data) {
            response = response.data[this.resource_type];
        }
        this.last_modified_time = response.lt;
        this.creation_time = response.ct;
        this.expiration_time = response.et;
        this.resource_name = response.rn;
        this.resource_id = response.ri;
        this.parent_id = response.pi;
        return response;
    }

    _resource_to_request(payload, update) {
        payload = payload || {};
        update = update || false; // Set update to false if not specified
        if (!update && this.resource_name) {
            payload.rn = this.resource_name;
        }
        let wrapped = {};
        wrapped[this.resource_type] = payload;
        return wrapped;
    }
    
    static discover(connection, filters) {
        let get_params = {
                fu: filterUsage.DISCOVERY,
                drt: discResType.STRUCTURED,
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
        }).then((response) => 
            this._resource_refresh_from_response(response)
        ).catch((error) => {
            console.error("Error while retrieving resource: " + this.resource_id);
            throw error;
        });
    }

    create(connection, payload) {
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
            url: this.parent_id,
            data: this._resource_to_request(payload, false)
        }).then((response) => 
            this._resource_refresh_from_response(response)
        ).catch((error) => {
            console.error("Error while creating resource: " + this.resource_id);
            throw error;
        });
    }
    
    update(connection, new_state) {
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
            data: this._resource_to_request(new_state, true)
        }).then((response) => 
            this._resource_refresh_from_response(response)
        ).catch((error) => {
            console.error("Error while updating resource: " + this.resource_id);
            throw error;
        });
    }

    handle_update_notification(notification) {
        return this._resource_refresh_from_response(notification); 
    }
}

export class CSE_Connection extends Resource {
    // Represents both a CSE, and manages the connection to the CSE
    constructor() {
        super("m2m:cb", "cse-in", null);
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

export class ACR {
    constructor(operation, originators) {
        this.operation = operation;
        this.originators = originators;
        this.to_request = this.to_request.bind(this);
    }

    static from_response(response) {
       return new ACR(response.acop, response.acor);
    }

    to_request() {
        return { acop: this.operation, acor: this.originators };
    }
}

export class ACP extends Resource {
    constructor(acp_identifier, acp_name) {
        super("m2m:acp", acp_identifier, acp_name);
        this.privileges = [];
        this.self_privileges = [];

        this.create = this.create.bind(this);
        this.update = this.update.bind(this);
        this.retrieve = this.retrieve.bind(this);
        this._refresh_from_response = this._refresh_from_response.bind(this);
        this._to_request = this._to_request.bind(this);
    }

    static discover(connection, filters) {
        filters = filters || {};
        return super.discover(connection, 
            Object.assign({}, {ty: RESOURCE_TYPES["m2m:acp"]}, filters));
    }

    _refresh_from_response(response) {
        let pv_list = [];
        let pvs_list = [];
        for (let p  = 0; p < response.pv.acr.length; p++) {
            pv_list.push(ACR.from_response(response.pv.acr[p]));
        }
        for (let p  = 0; p < response.pvs.acr.length; p++) {
            pvs_list.push(ACR.from_response(response.pvs.acr[p]));
        }
        this.privileges = pv_list;
        this.self_privileges = pvs_list;
        return response; 
    }

    _to_request() {
        let pv_list = [];
        let pvs_list = [];
        for (let p = 0; p < this.privileges.length; p++) {
            pv_list.push(this.self_privileges[p].to_request());
        }
        for (let p = 0; p < this.self_privileges.length; p++) {
            pvs_list.push(this.self_privileges[p].to_request());
        }
            
        return { 
                pv: { acr: pv_list },
                pvs: { acr: pvs_list }
        };
    }

    create(connection) {
        return super.create(connection, this._to_request());
    }
    
    retrieve(connection) {
        return super.retrieve(connection).then((response) => { this._refresh_from_response(response)});
    }

    update(connection) {
        return super.update(connection, this._to_request()).then((response) => this._refresh_from_response(response));
    }
};

export class AE extends Resource {
    constructor(ae_identifier, ae_name) {
        super("m2m:ae", ae_identifier, ae_name);
        this.point_of_access = [];               // poa
        this.ae_identifier = ae_identifier;      // aei
        this.app_identifier = "";                // api
        this.acp_ids = [];                       // acpi
        this.remote_reachable = false;           // rr   
        this.supported_release_versions = [ "3" ]; // srv
 
        this.retrieve = this.retrieve.bind(this);
        this.create = this.create.bind(this);
        this.update = this.update.bind(this);
        this._refresh_from_response = this._refresh_from_response.bind(this);
        this._to_request = this._to_request.bind(this);
    }

    static discover(connection, filters) {
        filters = filters || {};
        return super.discover(connection, 
            Object.assign({}, filters, {ty: RESOURCE_TYPES["m2m:ae"]}));
    }

    _refresh_from_response(response) {
        this.acp_ids = response.acpi;
        this.ae_identifier = response.aei;
        this.app_identifier = response.api;
        this.point_of_access = response.poa;
        this.remote_reachable = response.rr;
        this.supported_release_versions = response.srv;
        return response;
    }
        
    _to_request () {
        return { 
            poa: this.point_of_access,
            acpi: this.acp_ids,
            api: this.app_identifier,
            rr: this.remote_reachable,
            srv: this.supported_release_versions
        };
    }

    create(connection) {
        return super.create(connection, this._to_request()).then((response) => this._refresh_from_response(response));
    }
    
    retrieve(connection) {
        return super.retrieve(connection).then((response) => this._refresh_from_response(response));
    }

    update(connection) {
        return super.update(connection, this._to_request()).then((response) => this._refresh_from_response(response));
    }
};

export class FlexContainer extends Resource {
    constructor(container_type, resource_id, resource_name) {
        super("m2m:flex_container", resource_id, resource_name);
        // NOTE: The below resource_type_enum is normally set by the Resource class, but we 
        // must override it b/c of specialized flex container type
        this.resource_type = container_type;
        this.container_definition = "";
    
        this.retrieve = this.retrieve.bind(this);
        this.update = this.update.bind(this);
        this.handle_update_notification = this.handle_update_notification.bind(this);
    }

    static discover(connection, filters) {
        filters = filters || {};
        return super.discover(connection, 
            Object.assign({}, filters, {ty: RESOURCE_TYPES["m2m:flex_container"]}));
    }

    handle_update_notification(notification) {
        return super.handle_update_notification({ data: notification });
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

export class Subscription extends Resource {
    constructor(resource_id, resource_name, notification_uri_list) {
        super("m2m:sub", resource_id, resource_name);
        notification_uri_list = notification_uri_list || [];
        this.acp_ids = [];
        this.notification_uri_list = notification_uri_list;
        this.notification_content_type = 1;
        this.event_notification_criteria = { "net": [1,2,3,4 ] };
        
        this.create = this.create.bind(this);
        this.update = this.update.bind(this);
        this.retrieve = this.retrieve.bind(this);
        this._to_request = this._to_request.bind(this);
        this._refresh_from_response = this._refresh_from_response.bind(this);
    }
    
    static discover(connection, filters) {
        filters = filters || {};
        return super.discover(connection, 
            Object.assign({}, filters, {ty: RESOURCE_TYPES["m2m:sub"]}));
    }

    _refresh_from_response(response) {
        this.acp_ids = response.acpi;
        this.notification_uri_list = response.nu;  
        this.notification_content_type = response.nct;  
        this.event_notification_criteria = response.enc;
        return response;
    }
        
    _to_request () {
        return { 
            acpi: this.acp_ids,
            nu: this.notification_uri_list,
            nct: this.notification_content_type,
            enc: this.event_notification_criteria
        };
    }

    create(connection) {
        return super.create(connection, this._to_request()).then((response) => this._refresh_from_response(response));
    }
    
    retrieve(connection) {
        return super.retrieve(connection).then((response) => this._refresh_from_response(response));
    }

    update(connection) {
        return super.update(connection, this._to_request()).then((response) => this._refresh_from_response(response));
    }
}

export class PollingChannel extends Resource {
    constructor(resource_id, resource_name) {
        super("m2m:pch", resource_id, resource_name);
        this.create = this.create.bind(this);
        this.update = this.update.bind(this);
        this.retrieve = this.retrieve.bind(this);
        this._refresh_from_response = this._refresh_from_response.bind(this);
        this._to_request = this._to_request.bind(this);
        
        this.is_polling = false;
        this.start_poll = this.start_poll.bind(this);
        this.stop_poll = this.stop_poll.bind(this);
        this._poll_loop = this._poll_loop.bind(this);
    }
    
    static discover(connection, filters) {
        filters = filters || {};
        return super.discover(connection, 
            Object.assign({}, {ty: RESOURCE_TYPES["m2m:pch"]}, filters));
    }

    _refresh_from_response(response) {
        return response;
    }

    _to_request() {
        return {};
    }

    create(connection) {
        return super.create(connection, this._to_request());
    }

    retrieve(connection) {
        return super.retrieve(connection).then((response) => this._refresh_from_response(response));
    }

    update(connection) {
        return super.update(connection, this._to_request()).then((response) => this._refresh_from_response(response));
    }

    _poll_loop() {
    
        if(this.is_polling) {
            axios({
                method: "get",
                headers: {
                    "X-M2M-Origin": this._connection.originator,
                    "X-M2M-RI": create_random_identifier(),
                    "X-M2M-RVI": "3"
                },
                responseType: "json",
                baseURL: this._connection.url,
                url: this.resource_id + "/pcu"
            }).then((response) => {
                response = response.data["m2m:rqp"]
                this._response_callback(response);
                let request_id_echo = response.rqi;

                let acknowledge_data = { "m2m:rsp": {
                    rqi: request_id_echo,
                    pc: response.pc,
                    rsc: 2004,
                    rvi: "3"
                }};
                
                // Acknowledge the update
                axios({
                    method: "post",
                    headers: {
                        "X-M2M-Origin": this._connection.originator,
                        "X-M2M-RI": request_id_echo,
                        "X-M2M-RVI": "3"
                    },
                    responseType: "json",
                    baseURL: this._connection.url,
                    url: this.resource_id + "/pcu",
                    data: acknowledge_data
                }).then(() => {
                    // Perform the next long poll request
                    setTimeout(this._poll_loop, 50);
                });
            }).catch((error) => {
                if (error.response && error.response.status === 504) {
                    // long poll timed out, this means there wasn't any updates
                    setTimeout(this._poll_loop, 50);
                }
                else {
                    console.error("Error while polling channel!");
                    console.error(error);
                    throw error;
                }
            });
        }
    }

    start_poll(connection, callback) {
        if (!this.is_polling) {
            this._response_callback = callback;
            this._connection = connection;
            setTimeout(this._poll_loop, 50);
            this.is_polling = true;
        }
    }

    stop_poll() {
        this.is_polling = false;
    }
}

export default Resource;
