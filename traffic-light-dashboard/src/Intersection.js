import React from 'react';
import { FlexContainer } from "./onem2m.js";
import TrafficLightComponent from "./TrafficLight.js";

export class IntersectionFlexContainer extends FlexContainer {

    constructor(resource_id, resource_name, parent_resource_id, light1_state, light2_state, ble_state) {
        super("edu.psu.cse.traffic.trafficLightIntersection", "traffic:trfint", resource_name, resource_id, parent_resource_id);
        this.light1_state = light1_state;
        this.light2_state = light2_state;
        this.ble_state = ble_state;
    }

    static discover(connection) {
        let flex_discover = FlexContainer.discover(connection, "traffic:trfint");
        return flex_discover.then((flex_containers) => { 
            console.log(flex_containers);
            console.log(flex_containers.length);
            let discovered_intersections = [];
            for (let j = 0; j < flex_containers.length; j++) {
                let f = flex_containers[j];
                console.log(f);
                let i = new IntersectionFlexContainer(f.resource_id, 
                    f.resource_name, f.parent_resource_id, 
                    f._raw.l1s, f._raw.l2s, f._raw.bts);
                discovered_intersections.push(i);
            }
            
            return discovered_intersections;
        });
    }  
};

export class IntersectionComponent extends React.Component {

    constructor(props) {
        super(props);

        this.clickHandler = this.clickHandler.bind(this);   
        this.state = this.props.fcont;
    }

    clickHandler(light, value) {
        console.log("Light " + light + " change to " + value);
        // Really simple business logic to disallow invalid states (ie. both lights green)
        let light1_new = this.state.light1_state;
        let light2_new = this.state.light2_state;
        
        if (light == "1") {
            light1_new = value;
            if (value != "red" && this.state.light2_state != "red") {
                light2_new = "red";
            }
        }
        else {
            light2_new = value;
            if (value != "red" && this.state.light1_state != "red") {
                light1_new = "red";
            }
        }
       
        this.setState({ light1_state: light1_new, light2_state: light2_new });
 
        // Update the CSE side
        this.state.update(this.props.connection, { "l1s": light1_new, "l2s": light2_new });
    }

    render() {
        let light1_values = [];
        let light2_values = [];

        let light_colors = [ "red", "yellow", "green", "off" ];
        for (let l of light_colors) {
            light1_values.push([l, l == this.state.light1_state]);
            light2_values.push([l, l == this.state.light2_state]);
        }
        
        let bt_status = this.state.ble_state == "connected" ? "Connected" : "Disconnected";
        let bt_state = this.state.ble_state;
        
        return (<div className="intersection-container" >

            <div className="intersection-id" >ID: {this.state.resource_id}</div>
            <div>Bluetooth: <span className={bt_state} >{bt_status}</span></div>
            <TrafficLightComponent lightvalues={light1_values} 
                                    name="1" onClick={this.clickHandler} key="1" />
            <TrafficLightComponent lightvalues={light2_values} 
                                    name="2" onClick={this.clickHandler} key="2" />
        </div>);
    }

};

export default IntersectionComponent;
