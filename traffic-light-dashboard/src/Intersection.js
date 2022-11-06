import React from 'react';
import { Resource, FlexContainer } from "./onem2m.js";
import TrafficLightComponent from "./TrafficLight.js";

export class IntersectionFlexContainer extends FlexContainer {

    static _definition = "edu.psu.cse.traffic.trafficLightIntersection";

    constructor(resource_id) {
        super("traffic:trfint", resource_id);
        this.container_definition = IntersectionFlexContainer._definition;
        this.light1_state = "";
        this.light2_state = "";
        this.ble_state = "";

        this.retrieve = this.retrieve.bind(this);
        this.update = this.update.bind(this);
    }

    static discover(connection, filters) {
        filters = filters || {};
        filters = Object.assign({}, {"cnd": IntersectionFlexContainer._definition }, filters);
        return super.discover(connection, filters);
    }

    retrieve(connection) {
        return super.retrieve(connection).then((response) => {
            this.ble_state = response.bts;
            this.light1_state = response.l1s;
            this.light2_state = response.l2s;
        });
    }

    update(connection, new_state) {
        return super.update(connection, new_state).then((response) => {
            this.ble_state = response.bts;
            this.light1_state = response.l1s;
            this.light2_state = response.l2s;
        });
    }
};

export class IntersectionComponent extends React.Component {

    constructor(props) {
        super(props);

        this.clickHandler = this.clickHandler.bind(this);   
    }

    clickHandler(light, value) {
        console.log("Light " + light + " change to " + value);
        this.props.onClick(this.props.id, light, value);
    }

    render() {
        let intersection = this.props.intersection;
        let light1_values = [];
        let light2_values = [];

        let light_colors = [ "red", "yellow", "green", "off" ];
        for (let l of light_colors) {
            light1_values.push([l, l == intersection.light1_state]);
            light2_values.push([l, l == intersection.light2_state]);
        }
        
        let bt_status = intersection.ble_state == "connected" ? "Connected" : "Disconnected";
        let bt_state = intersection.ble_state;
        
        return (<div className="intersection-container" >

            <div className="intersection-id" >ID: {intersection.resource_id}</div>
            <div>Bluetooth: <span className={bt_state} >{bt_status}</span></div>
            <TrafficLightComponent lightvalues={light1_values} 
                                    name="1" onClick={this.clickHandler} key="1" />
            <TrafficLightComponent lightvalues={light2_values} 
                                    name="2" onClick={this.clickHandler} key="2" />
        </div>);
    }

};

export default IntersectionComponent;
