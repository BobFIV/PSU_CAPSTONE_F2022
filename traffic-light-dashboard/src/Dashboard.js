import React, { useState, useEffect } from "react";
import axios from "axios";

import CSEAddressEntryComponent from "./CSEAddressEntry.js";
import { Resource, CSE_Connection, AE, ACP, FlexContainer } from "./onem2m.js";
import { IntersectionFlexContainer, IntersectionComponent } from "./Intersection.js";

class DashboardComponent extends React.Component {

    constructor(props) {
        super(props);
        this.connection = new CSE_Connection();

        this.state = { connected: false, intersections: [] };
    
        this.connect = this.connect.bind(this);
    }

    connect(url, originator, base_ri) {
        this.connection.connect(url, originator, base_ri).then((data) => {
            return IntersectionFlexContainer.discover(this.connection).then((intersection_flex_cons) => {
                this.setState({ intersections: intersection_flex_cons, connected: true });
            }).catch(
                (error) => { throw error; }
            );
        }).catch((error) => {
            console.error("Exception while connecting:")
            console.error(error);
            this.setState({ intersections: [], connected: false });
        });
    }

    render() {
        let connectionStatus = this.state.connected ? "Connected" : "Disconnected";

        let intersectionComponents = [];

        let j = 0;
        for (let i of this.state.intersections) {
            intersectionComponents.push(<IntersectionComponent fcont={i} 
                    connection={this.connection} key={j} />);
            j++;
        }

        return (
            <div className="dashboard-container" >
                <div id="top-bar" >
                    <CSEAddressEntryComponent connectCallback={this.connect} 
                    defaultURL={this.props.defaultCSE} 
                    defaultOriginator={this.props.defaultOriginator} 
                    defaultRI={this.props.defaultRI} />
                    <span className="connect-status" >{connectionStatus}</span>
                </div>
    
                <div className="traffic-light-control-group" >
                    {intersectionComponents}
                </div>
            
            </div>
        );
    }

}

export default DashboardComponent;
