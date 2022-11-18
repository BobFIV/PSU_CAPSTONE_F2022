import React from 'react';
import { CSE_Connection, Resource, ACP, ACR, AE, FlexContainer } from "./onem2m.js";
import { IntersectionFlexContainer } from "./Intersection.js";

export class AddIntersectionButton extends React.Component {
    constructor(props) {
        super(props);
        this.clickHandler = this.clickHandler.bind(this);   
    }

    clickHandler() {
        let name = prompt("New Intersection Name:");
        let connection = new CSE_Connection();
        connection.originator = "C" + name;
        connection.url = this.props.connection_template.url;
        connection.resource_id = this.props.connection_template.resource_id;

        let acp = new ACP(null, name + "ACP");
        acp.privileges.push(new ACR(63, [connection.originator, "Cdashboard", "Cdashboard1", "Cdashboard2", "Cdashboard3" ]));
        acp.self_privileges.push(new ACR(63, [connection.originator, "Cdashboard", "Cdashboard1", "Cdashboard2", "Cdashboard3" ]));
        acp.parent_id = connection.resource_id;
        acp.create(connection).then((response) => {
            // Create the AE
            let ae = new AE(connection.originator, connection.originator);
            console.log("creating AE");
            ae.app_identifier = "NtrafficAPI";
            ae.acp_ids.push(response.ri);
            ae.parent_id = connection.resource_id;
            return ae.create(connection);
        }).then((response) => {
            // Create the flex container
            var i = new IntersectionFlexContainer(null, "intersection");
            i.light1_state = "red";
            i.light2_state = "red";
            i.ble_state = "disconnected";
            i.parent_id = response.ri;
            i.acp_ids = response.acpi;
            return i.create(connection);
        });
    }

    render() {
        
        return (<button type="button" onClick={this.clickHandler} >+</button>);
    }

};

export default AddIntersectionButton;
