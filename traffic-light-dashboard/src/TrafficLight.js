import React from 'react';

var classNames = require('classnames');

class TrafficLightButton extends React.Component {

    constructor(props) {
        super(props);
        this.state = { enabled: false };
        this.clickHandler = this.clickHandler.bind(this);
    }

    clickHandler(e) {
        this.props.lightEnableCallback(this.props.value);
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
        this.onLightSelect = this.onLightSelect.bind(this);
    }

    onLightSelect(new_value) {
        // Called by TrafficLightButton when it is pressed
        this.props.onClick(this.props.name, new_value);
    }

    render() {
        
        var lights = this.props.lightvalues.map((pair) => { 
            let value = pair[0];
            let enabled = pair[1];

            return  (
                <TrafficLightButton 
                    label={value.toUpperCase()} 
                    value={value} key={value}
                    enabled={enabled} 
                    lightEnableCallback={this.onLightSelect} />
            );
        });
        return (<div className="traffic-light-container" >{lights}</div>);
    }
}

export default TrafficLightComponent;
