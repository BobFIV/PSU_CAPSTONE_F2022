import React from 'react';
import TrafficLightComponent from "./TrafficLight.js"

class IntersectionComponent extends React.Component {

    constructor(props) {
        super(props);
        
    }

    dummyHandler() {
        console.log("Got click event!");
    }

    render() {
        return (
        <div className="intersection-container" >
            <TrafficLightComponent lightvalues={["red", "yellow", "green", "off"]} name="A" serial="000A" />
            <TrafficLightComponent lightvalues={["red", "yellow", "green", "off"]} name="B" serial="000B" />
        </div>
        );
    }

};

export default IntersectionComponent;
