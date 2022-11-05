import React from 'react';

class CSEAddressEntryComponent extends React.Component {
    constructor(props) {
        super(props);
        this.url_input = React.createRef();
        this.originator_input = React.createRef();
        this.ri_input = React.createRef();
        this.onAddressConnect = this.onAddressConnect.bind(this);
    }

    onAddressConnect(e) {
        // Called when the user presses the connect button

        // Do the callback to the App to connect to the CSE
        this.props.connectCallback(this.url_input.current.value, 
                                   this.originator_input.current.value, 
                                   this.ri_input.current.value);
        e.preventDefault();
    }

    render() {
        console.log(this.props.connectCallback);        
        return ( 
        <form className="cse-input" onSubmit={this.onAddressConnect} >
            <label>
                CSE Address: <input type="text" defaultValue={this.props.defaultURL} ref={this.url_input} /> 
                Originator: <input type="text" defaultValue={this.props.defaultOriginator} ref={this.originator_input} /> 
                Base RI: <input type="text" defaultValue={this.props.defaultRI} ref={this.ri_input} /> 
            </label>
            <input type="submit" value="Connect" />
        </form>
        );
    }
}

export default CSEAddressEntryComponent;
