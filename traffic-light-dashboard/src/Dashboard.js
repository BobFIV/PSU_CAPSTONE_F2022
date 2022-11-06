import React, { useState, useEffect } from "react";
import axios from "axios";

import CSEAddressEntryComponent from "./CSEAddressEntry.js";
import { Resource, CSE_Connection, AE, ACP, FlexContainer } from "./onem2m.js";
import { IntersectionFlexContainer, IntersectionComponent } from "./Intersection.js";

class DashboardComponent extends React.Component {

    constructor(props) {
        super(props);
        this.connection = new CSE_Connection();

        this.state = { intersections: [], connected: false };
    
        this.connect = this.connect.bind(this);
        this.userSelectLight = this.userSelectLight.bind(this);
    }

    connect(url, originator, base_ri) {
        return this.connection.connect(url, originator, base_ri).then((response) => {
            this.setState({ intersections: [], connected: true });
            return response;
        }).catch((error) => {
            console.error("Exception while connecting:")
            console.error(error);
            this.setState({ intersections: [], connected: false });
        });
    }

    componentDidMount() {
        this.poll_interval = setInterval(() => {
            if (this.state.connected) {
                IntersectionFlexContainer.discover(this.connection).then((intersection_ids) => {
                    let retrieve_promises = [];
                    for (let i of intersection_ids) {
                        let new_intersection = new IntersectionFlexContainer(i);
                        retrieve_promises.push(
                            new_intersection.retrieve(this.connection).then(() => new_intersection)
                        );
                    }

                    Promise.all(retrieve_promises).then((intersection_flex_cons) => {
                        console.log("got all");
                        this.setState({ intersections: intersection_flex_cons });
                    });
                }).catch(
                    (error) => { throw error; }
                );
            }
        }, 500);
    }

    componentWillUnmount() {
        clearInterval(this.poll_interval);
    }

    userSelectLight(intersection_index, light, value) {
        let intersection = this.state.intersections[intersection_index];

        // Really simple business logic to disallow invalid states (ie. both lights green)
        let light1_new = intersection.light1_state;
        let light2_new = intersection.light2_state;

        if (light == "1") {
            light1_new = value;
            if (value != "red" && intersection.light2_state != "red") {
                light2_new = "red";
            }
        }
        else {
            light2_new = value;
            if (value != "red" && intersection.light1_state != "red") {
                light1_new = "red";
            }
        }
        let new_state = this.state.intersections;
        new_state[intersection_index].light1_state = light1_new;
        new_state[intersection_index].light2_state = light2_new;

        this.setState({ intersections: new_state });

        // Update the CSE side
        intersection.update(this.connection, { "l1s": light1_new, "l2s": light2_new });
    }

    render() {
        let connectionStatus = this.state.connected ? "Connected" : "Disconnected";

        let intersectionComponents = [];

        let j = 0;
        for (let i of this.state.intersections) {
            intersectionComponents.push(<IntersectionComponent key={j} id={j}
                    intersection={i} 
                    onClick={this.userSelectLight} />);
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
