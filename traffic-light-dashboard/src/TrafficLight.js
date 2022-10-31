import React from 'react';

var classNames = require('classnames');

class TrafficLightButton extends React.Component {

    constructor(props) {
        super(props);
        this.state = { enabled: false };
        this.clickHandler = this.clickHandler.bind(this);
    }

    componentDidMount() {
        // Called when the component is placed into the DOM
    }

    componentWillUnmount() {
        // Called when the componetns is about to be removed from the DOM
    }

    clickHandler(e) {
        this.props.lightEnableCallback(this);
    }

    render() {
        var class_name_list = classNames(
            "traffic-light-button", 
            // The below line will result in something like "red-disabled" or "yellow-enabled", etc.
            this.props.value + "-" + (this.props.enabled ? "enabled" : "disabled")
        );
        return <div className={class_name_list} onClick={this.clickHandler} >
                  {this.props.label}
               </div>
    }
}

class TrafficLightComponent extends React.Component {
    constructor(props) {
        super(props);
        this.state = { enabled_value: "red", connected: false };
        this.onLightEnable = this.onLightEnable.bind(this);
    }

    componentDidMount() {
        // TODO: Start AE polling loop here?
    }

    onLightEnable(traffic_light_button) {
        // Called when the user clicks on a light.
        this.setState({enabled_value: traffic_light_button.props.value });
    }

    render() {
        // TODO: Have enabled property set based upon the state
        
        var lights = this.props.lightvalues.map((value) =>
            //var uppercase_label = value.t
            <TrafficLightButton label={value.toUpperCase()} value={value} key={value}
                enabled={value == this.state.enabled_value} 
                lightEnableCallback={this.onLightEnable} />
        );
        return <div class="traffic-light-container" >
                <h2>{this.props.name}</h2>
                <div class="traffic-light-serial" >{this.props.serial}</div>
                <div class="traffic-light-connect-status" >{this.state.connected ? "Connected" : "Disconnected" }</div>
                    {lights}
                </div>;
    }
}

export default TrafficLightComponent;
