import React from 'react';

class CSEAddressEntryComponent extends React.Component {
    constructor(props) {
        super(props);
        this.input = React.createRef();
        this.onAddressConnect = this.onAddressConnect.bind(this);
    }

    onAddressConnect(e) {
        // Called when the user presses the connect button

        // Do the callback to the App to connect to the CSE
        this.props.connectCallback(this.input.current.value);
        e.preventDefault();
    }

    render() {
        
        return ( 
        <form className="cse-input" onSubmit={this.onAddressConnect} >
            <label>
                CSE Address: <input type="text" defaultValue="34.238.135.110:8080" ref={this.input} /> 
            </label>
            <input type="submit" value="Connect" />
        </form>
        );
    }
}

export default CSEAddressEntryComponent;
