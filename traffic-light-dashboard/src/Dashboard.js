import React from "react";
import axios from "axios";

import CSEAddressEntryComponent from "./CSEAddressEntry.js";
import IntersectionComponent from "./Intersection.js";
import { Resource, CSE_Connection } from "./onem2m.js";

class Dashboard {
    constructor() {
        this.connection = new CSE_Connection();
        this.intersections = [];
        console.log("Dashboard constructor");
        console.log(this);
    }
}

class DashboardComponent extends React.Component {

    constructor(props) {
        super(props);
    }

    render() {
        var dashboard = this.props.dashboard;
        var connectionStatus = dashboard.connection.connected ? "Connected" : "Disconnected";

        var intersectionComponents = [];

        for (let i of dashboard.intersections) {
            intersectionComponents.push(<IntersectionComponent />);
        }

        return (
            <div className="dashboard-container" >
                <CSEAddressEntryComponent connectCallback={this.props.dashboard.connection.connect} 
                    defaultURL={this.props.defaultCSE} 
                    defaultOriginator={this.props.defaultOriginator} 
                    defaultRI={this.props.defaultRI} />

                <div className="connect-status" >{connectionStatus}</div>
                <div className="traffic-light-control-group" >
                    {intersectionComponents}
                </div>
            
            </div>
        );
    }

}

export { Dashboard, DashboardComponent };
export default DashboardComponent;
