import React, { useState, useEffect } from "react";
import axios from "axios";

import CSEAddressEntryComponent from "./CSEAddressEntry.js";
import { Resource, CSE_Connection, AE, ACP, ACR, FlexContainer, PollingChannel, Subscription } from "./onem2m.js";
import { IntersectionFlexContainer, IntersectionComponent } from "./Intersection.js";

class DashboardComponent extends React.Component {

    constructor(props) {
        super(props);
        this.connection = new CSE_Connection();

        this.state = { intersections: [], connected: false };
    
        this.connect = this.connect.bind(this);
        this.userSelectLight = this.userSelectLight.bind(this);
        this.handleNotification = this.handleNotification.bind(this);
    }

    connect(url, originator, base_ri) {
        let dashName = originator.substr(1);
        return this.connection.connect(url, originator, base_ri).then((response) => {
            // Create or retrieve the ACP
            ACP.discover(this.connection, { rn: dashName + "ACP" }).then((response) => {
                this.acp = new ACP(null, dashName + "ACP");
                if (response.length == 0) {
                    console.log("creating ACP");
                    this.acp.privileges.push(new ACR(63, [this.connection.originator]));
                    this.acp.self_privileges.push(new ACR(63, [this.connection.originator]));
                    this.acp.parent_id = this.connection.resource_id;
                    return this.acp.create(this.connection);
                }
                else {
                    console.log("retrieving ACP");
                    this.acp.resource_id = response[0];
                    return this.acp.retrieve(this.connection);
                }
            }).then((response) => {
                console.log("ACP present now.");
            }).then(() => {
                // Create or recieve the AE
                return AE.discover(this.connection, { aei: this.connection.originator }).then((response) => {
                    this.ae = new AE(this.connection.originator, this.connection.originator);
                    if (response.length == 0) {
                        console.log("creating AE");
                        this.ae.app_identifier = "NtrafficDashAPI";
                        this.ae.acp_ids.push(this.acp.resource_id);
                        this.ae.parent_id = this.connection.resource_id;
                        return this.ae.create(this.connection);
                    }
                    else {
                        console.log("retrieving AE");
                        return this.ae.retrieve(this.connection);
                    }
                });
            }).then(() => {
                console.log("PCH");
                return PollingChannel.discover(this.connection, { rn: dashName + "PCH" }).then((response) => {
                    this.pch = new PollingChannel(null, dashName + "PCH", "dash-pch");
                    if (response.length == 0) {
                        // create PCH
                        this.pch.parent_id = this.ae.resource_id;
                        return this.pch.create(this.connection);
                    }
                    else {
                        // retrieve PCH
                        this.pch.resource_id = response[0];
                        return this.pch.retrieve(this.connection).then(() => {
                            console.log("PCH retrieved");
                        });
                    }
                });
            }).then(() => {
                return IntersectionFlexContainer.discover(this.connection).then((intersection_ids) => {
                    let retrieve_promises = [];
                    for (let i of intersection_ids) {
                        let new_intersection = new IntersectionFlexContainer(i);
                        retrieve_promises.push(
                            new_intersection.retrieve(this.connection).then(() => new_intersection)
                        );
                    }

                    return Promise.all(retrieve_promises);
                });
            }).then((intersection_flex_cons) => {
                let refresh_subscription_promises = [];
                for (let i of intersection_flex_cons) {
                    let p = Subscription.discover(this.connection, { rn: dashName + "SUB" }).then((response) => {
                        if (response.length == 0) {
                            // Create subscription
                            console.log("creating sub");
                            let sub = new Subscription(null, dashName + "SUB", [ this.connection.originator ]);
                            sub.parent_id = i.resource_id;
                            sub.acp_ids = [ this.acp.resource_id ];
                            return sub.create(this.connection);
                        }
                        else {
                            // Retrieve subscription
                            console.log("retrieving sub");
                            let sub = new Subscription(response[0], dashName + "SUB");
                            return sub.retrieve(this.connection).then((response) => {
                                // Check to make sure that the subscription is pointing to the correct originator
                                if (sub.notification_uri_list[0] != this.connection.originator) {
                                    sub.notification_uri_list[0] = this.connection.originator;
                                    return sub.update(this.connection);
                                }
                            });
                        }
                    });
                    refresh_subscription_promises.push(p);
                }
                return [intersection_flex_cons, Promise.all(refresh_subscription_promises)];
            }).then((pair) => {
                let intersection_flex_cons = pair[0];
                this.pch.start_poll(this.connection, this.handleNotification);
                this.setState({ intersections: intersection_flex_cons, connected: true });
            }).catch((error) => {
                console.error("Exception while connecting:")
                console.error(error);
                this.setState({ intersections: [], connected: false });
            });
        });
    }

    handleNotification(notification) {
        let update_representation = notification.pc["m2m:sgn"].nev.rep;
        let updated_id = update_representation["traffic:trfint"].ri;
        let intersections = this.state.intersections;
        for (let i = 0; i < intersections.length; i++) {
            if (intersections[i].resource_id == updated_id) {
                intersections[i].handle_update_notification(update_representation);
            }
        }
        
        this.setState({ intersections: intersections, connected: true });
    }

    componentWillUnmount() {
        if (this.state.connected) {
            this.pch.stop_poll();
        }
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
            intersectionComponents.push(<IntersectionComponent 
                    key={j} id={j}
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
