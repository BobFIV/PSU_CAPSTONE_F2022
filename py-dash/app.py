#############################################
# Team PJ5C Capstone Dashboard

import logging
import dash
from dash import dcc
from dash import html
from dash.dependencies import Input, Output, State
from dash import dash_table as dash_table
import plotly.graph_objs as go
import dash_daq as daq
import plotly.express as px
import pandas as pd
import datetime as datetime
import argparse
from configparser import ConfigParser
import requests
from json import loads, dumps
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
import time
from requests.api import request

#### Read Config File ####
parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("-c", "--config", type=str, default='device.cfg', help="Config file name")
args = parser.parse_args()
configFile = args.config

parser = ConfigParser()
parser.optionxform = str
parser.read(configFile)
appIP = parser.get('CONFIG_DATA', 'appIP')
appPort = int(parser.get('CONFIG_DATA', 'appPort'))
appName = parser.get('CONFIG_DATA', 'appName')
dashHost = parser.get('CONFIG_DATA', 'dashHost')
cseIP = parser.get('CONFIG_DATA', 'cseIP')
csePort = parser.get('CONFIG_DATA', 'csePort')
cseID = parser.get('CONFIG_DATA', 'cseID')
cseName = parser.get('CONFIG_DATA', 'cseName')

#### Dash App IP & Port ####
ip = dashHost
port = appPort

### AE-ID for this device ###
originator = 'C'+appName

# Hold sensor and actuator data 
sensors = []
actuators = []

# Handle subs for sensors and actuators
newSensorList = []
newActuatorList = []

#### Style, Color, and theme variable initialization ####
alertStyle = {
  'display': 'flex',
  'flex-direction': 'column',
  'justify-content': 'space-evenly',
  'align-items': 'center',
  'align-self': 'center',
  'color': 'white',
  'padding-left': '2rem',
  'padding-right': '2rem',
  'width': '100%',
  'text-align': 'center'
}

alertStyle2 = {
  'display': 'flex',
  'flex-direction': 'column',
  'justify-content': 'space-evenly',
  'align-items': 'center',
  'align-self': 'center',
  'color': 'white',
  'padding-left': '2rem',
  'padding-right': '2rem',
  'width': '100%',
  'text-align': 'center',
  'margin-bottom': '2rem'
}

indicatorOffColor = '#1E2130'
indicatorOnColor = '#91dfd2'

alertStyleHidden = {'color': 'white'}

darkTheme = {
    'dark': True,
    'primary': '#161a28',
    'secondary': 'white',
    'detail': 'white', 
}

darkThemeBar = {
    'dark': True,
    'primary': indicatorOnColor,
    'secondary': 'black',
    'detail': 'white', 
    'margin-left': '2rem',
    'margin-right': '2rem',
    'margin-bottom': '3rem',
    'width': '100%',
    'justify-content': 'space-evenly'

}

barStyle = {'font-size': '1.6rem'}

#### Setup App Metadata ####
app = dash.Dash(
    __name__,
    meta_tags=[{"name": "viewport", "content": "width=device-width, initial-scale=1"}],
)
app.title = "Smart City Dashboard"

app.config["suppress_callback_exceptions"] = True

# Disable excessive http dash component post logs on command line
# Focuses on oneM2M primitives, notifications, and errors logging. 
log = logging.getLogger('werkzeug')
log.disabled = True

def generateGraph(x, y):
    figure =  go.Figure(
    {
        "data": [
            {
                "x": x,
                "y": y,
                "mode": "lines",
            }
        ],
        "layout": {
            "paper_bgcolor": "rgba(0,0,0,0)",
            "plot_bgcolor": "rgba(0,0,0,0)",
            "xaxis": dict(
                showline=False, showgrid=False, zeroline=False
            ),
            "yaxis": dict(
                showgrid=False, showline=False, zeroline=False
            ),
        },
    })
    figure.update_layout(height = 400, margin=dict(t=0,r=0,l=0,b=0,pad=0))
    return figure

#### Populate Dropdown List ####
def build_dropdown(devType):
    options = []
    if (devType == "sensor"):
        for sensor in sensors:
            options.append({"label": sensor['rn'], "value": sensor['url']})
    elif (devType == "actuator"):
        for actuator in actuators:
            options.append({"label": actuator['rn'], "value": actuator['url']})
    return options

#### Generate Buttons ####
def create_button(id, child, classN = ""):
    return html.Div(
        id = "buttonDiv",
        children = [
            html.Div(
                id = "card-1",
                children = [
                    html.Button(id = id, children = [child], n_clicks = 0)
                ]
            )
        ]
    )

#### Section Banners ####
def generate_section_banner(title):
    return html.Div(className="section-banner", children=title)

#### Banner ####
def build_banner():
    return html.Div(
        id="banner",
        className="banner",
        children=[
            html.Div(
                id="banner-text",
                children=[
                    html.H5("oneM2M-based Irrigation System Dashboard"),
                    html.H6("Capstone Team PJ3C"),
                ],
            ),
            html.Div(
                id="banner-logo",
                children=[
                    html.A(
                        id = "logo-container2",
                        children = [
                            html.Img(id="logo1", src="../assets/exacta_logo.jpg"),
                        ],
                        href = "http://www.exactagss.com/"
                    ),
                    html.A(
                        id = "logo-container",
                        children = [
                            html.Img(id="logo2", src="../assets/psu_logo.png"),
                        ],
                        href = "https://www.lf.psu.edu/"
                    )
                ],
            ),
        ],
    )

#### Tabs ####
def build_tabs():
    return html.Div(
        id="tabs",
        className="tabs",
        children=[
            dcc.Tabs(
                id="app-tabs",
                value="tab2",
                className="custom-tabs",
                children=[
                    dcc.Tab(
                        id="Control-chart-tab",
                        label="Device & Sensor Monitoring",
                        value="tab2",
                        className="custom-tab",
                        selected_className="custom-tab--selected",
                    ),
                    dcc.Tab(
                        id="Specs-tab",
                        label="Device Control",
                        value="tab1",
                        className="custom-tab",
                        selected_className="custom-tab--selected",
                    ),
                    
                ],
            )
        ],
    )

#### Sensor Tab ####

# Real Time Stats Panel
def build_quick_stats_panel():
    return html.Div(
        id="quick-stats",
        
        children=[
            dcc.Interval(id = 'clkDevS', interval = 1*1000, n_intervals = 0),
            dcc.Interval(id = 'clkVal', interval = 4*1000, n_intervals = 0),
            generate_section_banner("Real Time Sensor Values"),
            
            html.Div(
                id="dropdown",
                children=[
                    dcc.Dropdown(id = "ddMenuS", options = build_dropdown("sensor"), clearable = True, placeholder= "Select Device ...", searchable = False)
                ],
            ),

            html.Div(
                className= 'alert', children = [
                    html.Span(id = 'alertboxS', hidden = True, children = [], style=alertStyleHidden)
                ]
            ),
            html.Div(
                id = "side-buttons-container",
                children = [
                    html.Div(
                        id = 'card-temp',  
                        children=[
                            html.P("Temperature"),
                            daq.Gauge(
                                id = "temp-gauge",
                                max = 115,
                                min = -5,
                                showCurrentValue = False,
                                value = None,
                                units = "°F",
                                size = 125
                            )
                        ],
                    ),
                    html.Div(
                        id="card-hum",
                        children=[
                            html.P("Air Humidity"),
                            daq.Gauge(
                                id = "hum-gauge",
                                max = 100,
                                min = 0,
                                showCurrentValue = False,
                                value = None,
                                units = "%",
                                size = 125
                            )
                        ],
                    ),
                ]
            ),
            html.Div(
                id = "side-buttons-container",
                children = [
                    html.Div(
                        id="card-mois",
                        children=[
                            html.P("Soil Moisture"),
                            daq.Gauge(
                                id = "mois-gauge",
                                max = 100,
                                min = 0,
                                showCurrentValue = False,
                                value = None,
                                units = "%",
                                size = 125
                            )
                        ],
                    ),
                    html.Div(
                        id="card-rain",
                        children=[
                            html.P("Rainfall Indicator"),
                            daq.Indicator(
                                id = 'rainfallIndicator',
                                value = False,
                                label = '',
                                labelPosition = 'bottom',
                                color=indicatorOffColor,
                            )
                        ],
                    ),
                ]
            ),
        ],
    )

# Top Panel
def build_top_panel():
    return html.Div(
        id="top-section-container",
        className="row",
        children=[
            # Temperature Graph
            html.Div(
                id="metric-summary-session",
                className="four columns",
                children=[
                    generate_section_banner("Temperature History"),
                    html.Div(
                        dcc.Graph(id = "temp-graph", figure=generateGraph([], [])),
                    ),
                ],
            ),
            # Humidity Graph
            html.Div(
                id="ooc-piechart-outer",
                className="four columns",
                children=[
                    generate_section_banner("Air Humidity History"),
                    html.Div(
                        dcc.Graph(id = "hum-graph", figure=generateGraph([], []))
                    ),
                ],
            ),
        ],
    )

# Bottom Panel 
def build_chart_panel():
    return html.Div(
        id="bottom-section-container",
        className="row",
        children=[
            # Soil Moisture Graph
            html.Div(
                id="metric-summary-session",
                className="four columns",
                children=[
                    generate_section_banner("Soil Moisture History"),
                    html.Div(
                        dcc.Graph(id = "mois-graph", figure=generateGraph([], [])),
                    ),
                ],
            )
        ]
    )


#### Control Tab ####

# Side panel 
def build_control_side():
    return html.Div(
        id="control-side",
        children=[
            dcc.Interval(id = 'clkDevC', interval = 1*1000, n_intervals = 0),
            generate_section_banner("Link Devices"),
            html.Div(
                id = "card-3",
                children = [
                    html.P("Sensor Device")
                ]
            ),
            html.Div(
                id="dropdown1",
                children=[
                    dcc.Dropdown(id = "ddMenuSensor", options = build_dropdown("sensor"), clearable = True, placeholder= "Select Device ...", searchable = False)
                ],
            ),
            html.Div(
                className= 'alert', children = [
                    html.Span(id = 'alertboxSensor', className = "center", hidden = True, children = [], style=alertStyleHidden)
                ], 
            ),
            html.Div(
                id = "card-1",
                children = [
                    html.P("Valve Control Device")
                ]
            ),
            html.Div(
                id="dropdown2",
                children=[
                    dcc.Dropdown(id = "ddMenuActuator", options = build_dropdown("actuator"), clearable = True, placeholder= "Select Device ...", searchable = False)
                ],
            ),   
            html.Div(
                className= 'alert', children = [
                    html.Span(id = 'alertboxActuator', className = "center", hidden = True, children = [], style=alertStyleHidden)
                ], 
            ),
            html.Div(
                id = "side-buttons-container",
                className = "specialContainer",
                children = [
                    create_button("linkButton", "Link"),
                    create_button("unlinkButton", "Unlink")
                ]
            ),
            html.Div(
                id = "card-1",
                children = [
                    html.P("Sensor Battery Level"),
                    daq.DarkThemeProvider(
                        theme = darkThemeBar, 
                        children =[
                            daq.GraduatedBar(
                                id = "sBattery",
                                value = None,
                                showCurrentValue = False,
                                max = 100,
                                min = 0, 
                                step = 2,
                                label = {'label':'', 'style': barStyle},
                                labelPosition = "bottom"
                            ),  
                        ]
                    ),       
                ]
            ),
            html.Div(
                id = "card-2",
                children = [
                    html.P("Valve Control Battery Level"),
                    daq.DarkThemeProvider(
                        theme = darkThemeBar, 
                        children =[
                            daq.GraduatedBar(
                                id = "aBattery",
                                value = None,
                                showCurrentValue = False,
                                max = 100,
                                min = 0, 
                                step = 2,
                                label = {'label':'', 'style': barStyle},
                                labelPosition = "bottom"
                            ),  
                        ]
                    ),   
                ]
            ),
            
        ]
    )

# Settings Column
def build_settings_column(id, cardID, title, max = None, min = None):
    if (id == "transmitP"):
        default = 10
    else:
        default = 1
    if (max == None):
        input = daq.NumericInput(
            id = id,
            value = default,
            max = 999,
            min = 1
    )
    else:
        input = daq.NumericInput(
            id = id,
            value = default,
            max = max, 
            min = min
        )
    return html.Div(
            html.Div(  
                id = cardID,
                children = [ 
                    html.P(title),
                    input
                ]
            )
    )

# Left settings column
def build_settings_panel_left():
    return html.Div(
        id = "settings-col",
        children=[
            build_settings_column("numAvg", "card-5", "Sensor values to average", min = 1, max = 100),
            build_settings_column("transmitP", "card-2", "Transmit time (s)"),
        ]
    )

# Right settings column
def build_settings_panel_right():
    return html.Div(
        id = "settings-col",
        children=[
            build_settings_column("sampleP", "card-3", "Time between sensor readings (s)"),
            create_button("settingsButton", "Submit"),
            html.Div(
                className= 'alert', children = [
                    html.Span(id = 'settingsAlertbox', hidden = True, children = [], style=alertStyleHidden)
                ]
            ),
        ]
    )

# Top panel 
def build_control_top():
    return html.Div(
        id="control-container-big",
        children = [
            generate_section_banner("Sensor Settings"),
            html.Div(
                id="top-section-container-control",
                className="row",
                children=[
                    html.Div(
                        id="metric-summary-session",
                        className="six columns",
                        children=[
                            build_settings_panel_left()
                        ],
                    ),
                    html.Div(
                        id="metric-summary-session",
                        className="six columns",
                        children=[
                            build_settings_panel_right()
                        ],
                    ),
                ]
            )
        ]
    )

# Column 1 of bottom panel
def build_control_panel_left():
    return html.Div(
        id = "control-col",
        children = [
            html.Div(
                id = "card-4",
                children = [
                    html.P("Manual Override"),
                ]
            ),
            html.P(id = "warning", children = ["Warning: Setting manual override on will disable automated irrigation functionality."]),
            html.Div(
                id = "card-1",
                children = [
                    daq.DarkThemeProvider(
                        theme = darkTheme, 
                        children =[
                            daq.PowerButton(
                            id ='manualOverride',
                            label = '',
                            on = False,
                            labelPosition = 'top',
                            color = indicatorOnColor
                            ),
                        ]
                    ) 
                ]
            ),
            html.Div(
                id = "card-4",
                children = [
                    html.P("Manual Control"),
                ]
            ),
            html.Div(
                id = "side-buttons-container",
                children = [
                    create_button("valveOn", "On"),
                    create_button("valveOff", "off")
                ]
            ),
            html.Div(
                className= 'alert', children = [
                    html.Span(id = 'alertboxOn', hidden = True, children = [], style=alertStyleHidden)
                ]
            ),
            html.Div(
                className= 'alert', children = [
                    html.Span(id = 'alertboxOff', hidden = True, children = [], style=alertStyleHidden)
                ]
            ),
        ]
    )

# Column 2 of bottom panel
def build_control_panel_right():
    return html.Div(
        id = "control-col",
        children = [
            html.Div(
                id = "card-3",
                children = [
                    
                    html.P("Control Valve Status"),
                    dcc.Interval(id = 'clkActStatus', interval = 1*1000, n_intervals = 0),
                    daq.Indicator(
                        id = 'valveStatus',
                        value = False,
                        label = '',
                        labelPosition = 'bottom',
                        color=indicatorOffColor,
                    )
                ]
            ),
        ]
    )

# Bottom Panel
def build_control_bottom():
    return html.Div(
        id = 'control-chart-container',
        children = [
            generate_section_banner("Manual Valve Controls"),
            html.Div(
                id="bottom-section-container-control",
                className="row",
                children=[
                    html.Div(
                        id="metric-summary-session",
                        className="six columns",
                        children=[
                            build_control_panel_left()
                        ],
                    ),
                    html.Div(
                        id="metric-summary-session",
                        className="six columns",
                        children=[
                            build_control_panel_right()
                        ],
                    ),
                ]
            )
        ]
    )

#### App Layout ####
app.layout = html.Div(
    id="big-app-container",
    children=[
        build_banner(),
        html.Div(
            id="app-container",
            children=[
                build_tabs(),
                # Main app
                html.Div(id="app-content", children = []),
            ],
        ),
    ],
)

#### Tab Switching Callback ####
@app.callback(
    Output("app-content", "children"),
    [Input("app-tabs", "value")],
)
def render_tab_content(tab_switch):
    if tab_switch == "tab1":
        return html.Div(
                id="status-container",
                children=[
                    build_control_side(),
                    html.Div(
                        id = "graphs-container",
                        children = [build_control_top(), build_control_bottom()],
                    ),
                ],
            )

    return html.Div(
            id="status-container",
            children=[
                build_quick_stats_panel(),
                html.Div(
                    id="graphs-container",
                    children=[build_top_panel(), build_chart_panel()],
                ),
            ],
        )

############################
#                          #
#   Sensor Tab Callbacks   #
#                          #
############################

#### Update Device List with subscriptions ####
@app.callback(
    Output('ddMenuS', 'options'),
    [Input('clkDevS', 'n_intervals')],
    [Input('ddMenuS', 'options')],
    prevent_initial_call = True
)   
def updateSensorList(n, ddMenu):
    global sensors
    newOptions = []
    for sensor in sensors:
        newOptions.append(sensor['url'])
        
    oldOptions = []
    for sensor in ddMenu:
        oldOptions.append(sensor['value'])
        
    newOptions.sort()
    oldOptions.sort()

    if (newOptions != oldOptions):
        options = []
        for sensor in sensors:
            options.append({'label': sensor['rn'], 'value': sensor['url']})
        return options
    return dash.no_update

#### Temperature Values and Graph Updates ####
@app.callback(
    Output('temp-gauge', 'value'),
    Output('temp-gauge', 'max'),
    Output('temp-gauge', 'min'),
    Output('temp-gauge', 'showCurrentValue'),
    Output('temp-graph', 'figure'),
    [Input('ddMenuS', 'value')],
    [Input('clkVal', 'n_intervals')],
    [Input('temp-graph', 'figure')],
    [Input('temp-gauge', 'max')],
    [Input('temp-gauge', 'min')],
    prevent_initial_call = True
)
def showTempVal(value, n, figure, maxVal, minVal):
    if (value is not None):
        oldDates = figure['data'][0]['x']
        values, dates = getAllData(value, "Temperature")
        if (dates == []):
            return None, 115, -5, False, generateGraph([], [])
        latestVal = values[dates.index(max(dates))]  
        if (latestVal > 115):
            maxVal = int(latestVal) + 5
            minVal = -5
        elif (latestVal < -5):
            maxVal = 115
            minVal = int(latestVal) - 5
        elif (maxVal != 115) or (minVal != -5):
            maxVal = 115
            minVal = -5
        if (oldDates == []):
            return latestVal, maxVal, minVal, True, generateGraph(dates, values)
        elif (oldDates[-1] != dates[-1]):
            return latestVal, maxVal, minVal, True, generateGraph(dates, values)
        return dash.no_update, maxVal, minVal, True, dash.no_update
    if (maxVal != 115) or (minVal != -5):
        return None, 115, -5, False, generateGraph([], [])
    return None, dash.no_update, dash.no_update, False, generateGraph([], [])

#### Soil Moisture Values and Graph Updates ####
@app.callback(
    Output('mois-gauge', 'value'),
    Output('mois-gauge', 'showCurrentValue'),
    Output('mois-graph', 'figure'),
    [Input('ddMenuS', 'value')],
    [Input('clkVal', 'n_intervals')],
    [Input('mois-graph', 'figure')],
    prevent_initial_call = True
)
def showMoisVal(value, n, figure):
    if (value is not None):
        oldDates = figure['data'][0]['x']
        values, dates = getAllData(value, "SoilMoisture")
        if (dates == []):
            return None, False, generateGraph([], [])    
        elif (oldDates == []):
            latestVal = values[dates.index(max(dates))]
            return latestVal, True, generateGraph(dates, values)
        elif (oldDates[-1] != dates[-1]):
            latestVal = values[dates.index(max(dates))]
            return latestVal, True, generateGraph(dates, values)
        return dash.no_update, True, dash.no_update
    return None, False, generateGraph([], [])

#### Humidity Values and Graph Updates ####
@app.callback(
    Output('hum-gauge', 'value'),
    Output('hum-gauge', 'showCurrentValue'),
    Output('hum-graph', 'figure'),
    [Input('ddMenuS', 'value')],
    [Input('clkVal', 'n_intervals')],
    [Input('hum-graph', 'figure')],
    prevent_initial_call = True
)
def showHumVal(value, n, figure):
    if (value is not None):
        oldDates = figure['data'][0]['x']
        values, dates = getAllData(value, "Humidity")
        if (dates == []):
            return None, False, generateGraph([], [])    
        elif (oldDates == []):
            latestVal = values[dates.index(max(dates))]
            return latestVal, True, generateGraph(dates, values)
        elif (oldDates[-1] != dates[-1]):
            latestVal = values[dates.index(max(dates))]
            return latestVal, True, generateGraph(dates, values)
        return dash.no_update, True, dash.no_update
    return None, False, generateGraph([], [])

#### Rainfall Trigger Update ####
@app.callback(
    Output('rainfallIndicator', 'value'),
    Output('rainfallIndicator', 'label'),
    Output('rainfallIndicator', 'color'),
    [Input('ddMenuS', 'value')],
    [Input('clkVal', 'n_intervals')],
    [Input('rainfallIndicator', 'label')],
    prevent_initial_call = True
)
def showRainIndicator(value, n, label):
    if (value is not None):
        latestVal = getLatestData(value, "RainfallTrigger")
        indicator = True
        indicatorColor = indicatorOffColor
        if latestVal in ['OFF', 'off', 'Off']:
            newLabel = 'OFF'
        elif latestVal in ['ON', 'on', 'On']:
            newLabel = 'ON'
            indicatorColor = indicatorOnColor
        else:
            indicator = False
            newLabel = ''
        if (label != newLabel): 
            return indicator, newLabel, indicatorColor
        return dash.no_update, dash.no_update, dash.no_update
    return False, '', indicatorOffColor

#############################
#                           #
#   Control Tab Callbacks   #
#                           #
#############################

#### Update Sensor Device List ####
@app.callback( 
    Output('ddMenuSensor', 'options'),
    [Input('clkDevC', 'n_intervals')],
    [Input('ddMenuSensor', 'options')],
    prevent_initial_call = True
)   
def updateCSensorList(n, ddMenu):
    global sensors
    newOptions = []
    for sensor in sensors:
        newOptions.append(sensor['url'])
        
    oldOptions = []
    for sensor in ddMenu:
        oldOptions.append(sensor['value'])
        
    newOptions.sort()
    oldOptions.sort()

    if (newOptions != oldOptions):
        options = []
        for sensor in sensors:
            options.append({'label': sensor['rn'], 'value': sensor['url']})
        return options
    return dash.no_update

#### Update Actuator Device List ####
@app.callback( 
    Output('ddMenuActuator', 'options'),
    [Input('clkDevC', 'n_intervals')],
    [Input('ddMenuActuator', 'options')],
    prevent_initial_call = True
)   
def updateActuatorList(n, ddMenu):
    global actuators
    newOptions = []
    for actuator in actuators:
        newOptions.append(actuator['url'])
        
    oldOptions = []
    for actuator in ddMenu:
        oldOptions.append(actuator['value'])
        
    newOptions.sort()
    oldOptions.sort()

    if (newOptions != oldOptions):
        options = []
        for actuator in actuators:
            options.append({'label': actuator['rn'], 'value': actuator['url']})
        return options
    return dash.no_update

#### Sensor Settings ####
@app.callback(
    Output('settingsAlertbox', 'children'),
    Output('settingsAlertbox', 'style'),
    Output('settingsAlertbox', 'hidden'),
    Output('settingsButton', 'n_clicks'),
    [Input('ddMenuSensor', 'value')],
    [Input('numAvg', 'value')],
    [Input('transmitP', 'value')],
    [Input('sampleP', 'value')],
    [Input('settingsButton', 'n_clicks')],
    prevent_initial_call = True
)
def settingsInput(value, numAvg, transmitP, sampleP, button):
    if ((button > 0) and (value is not None)):
        
        checkVal = transmitP - (numAvg * sampleP)
        if (checkVal < 4):
            errorMessage = [
                "Error: The transmit time has to be greater than 4 seconds " + 
                "more than the product of the sensor values to " +
                "average and the time between sensor readings!"
            ]
            return errorMessage, alertStyle, False, 0

        discovery = discover(type="Container", location = value + "/Settings")

        numLoc = value + "/Settings/numAverages"
        sampleLoc = value + "/Settings/samplePeriod"
        transmitLoc = value + "/Settings/transmitPeriod"

        if (numLoc in discovery) and (sampleLoc in discovery) and (transmitLoc in discovery):

            createContentInstance(str(numAvg), numLoc)
            createContentInstance(str(sampleP), sampleLoc)
            createContentInstance(str(transmitP), transmitLoc)
            if (checkVal >= 9):
                return [], alertStyleHidden, True, 0
            return ["Warning: GPS might not update properly with these settings!"], alertStyle, False, 0
        
        return ["Error: Settings cannot be modified on this sensor device!"], alertStyle, False, 0

    if ((button > 0) and (value is None)):
        return "Error: Please choose a device first!", alertStyle, False, 0

    return [], alertStyleHidden, True, 0

#### Turn on Valve ####
@app.callback(
    Output('valveOn', 'n_clicks'),
    Output('alertboxOff', 'children'),
    Output('alertboxOff', 'style'),
    Output('alertboxOff', 'hidden'),
    [Input('valveOn', 'n_clicks')],
    [Input('ddMenuActuator', 'value')],
    [Input('manualOverride', 'on')],
    prevent_initial_call = True
)
def actuatorOn(n, value, manualOverride):
    if (n > 0) and (value is not None) and (manualOverride):
        createContentInstance("11", value + "/requestedState")
        return 0, [], alertStyleHidden, True
    if (n > 0) and (manualOverride == False) and (value is not None):
        return 0, ["Please turn on manual override first!"], alertStyle, False
    return 0, [], alertStyleHidden, True

#### Turn off Valve ####
@app.callback(
    Output('valveOff', 'n_clicks'),
    Output('alertboxOn', 'children'),
    Output('alertboxOn', 'style'),
    Output('alertboxOn', 'hidden'),
    [Input('valveOff', 'n_clicks')],
    [Input('ddMenuActuator', 'value')],
    [Input('manualOverride', 'on')],
    prevent_initial_call = True
)
def actuatorOff(n, value, manualOverride):
    if (n > 0) and (value is not None) and (manualOverride):
        createContentInstance("10", value + "/requestedState")
        return 0, [], alertStyleHidden, True
    if (n > 0) and (manualOverride == False) and (value is not None):
        return 0, ["Please turn on manual override first!"], alertStyle, False
    return 0, [], alertStyleHidden, True

#### Manual Override Control ####
@app.callback(
    Output('manualOverride', 'on'),
    [Input('manualOverride', 'on')],
    [Input('ddMenuActuator', 'value')],
    prevent_initial_call = True
)
def manualOverrideC(manualOverride, value):
    trigger = dash.callback_context
    triggerID = trigger.triggered[0]['prop_id'].split('.')[0]
    if (triggerID == 'manualOverride') and (value is not None):
        if (manualOverride):
            createContentInstance("10", value + "/requestedState")
        elif (manualOverride == False):
            createContentInstance("00", value + "/requestedState")
    elif (triggerID == 'ddMenuActuator') and (value is not None):
        status, date = requestVal(value + "/requestedState/la")
        if (len(status) == 2):
            if (status[0] == "1"):
                return True
        return False
    if (value is None):
        return False
    return dash.no_update

#### Valve Status Update ####
@app.callback(
    Output('valveStatus', 'value'),
    Output('valveStatus', 'label'),
    Output('valveStatus', 'color'),
    [Input('ddMenuActuator', 'value')],
    [Input('clkActStatus', 'n_intervals')],
    [Input('valveStatus', 'label')],
    prevent_initial_call = True
)
def showActuatorIndicator(value, n, label):
    if (value is not None):
        latestVal = getLatestData(value, "actuatorState", aeType="actuator")
        indicator = True
        indicatorColor = indicatorOffColor
        if latestVal == "0":
            newLabel = 'OFF'
        elif latestVal == "1":
            newLabel = 'ON'
            indicatorColor = indicatorOnColor
        else:
            indicator = False
            newLabel = ''
        if (label != newLabel):
            return indicator, newLabel, indicatorColor
        return dash.no_update, dash.no_update, dash.no_update
    return False, '', indicatorOffColor

#### Link Devices ####
@app.callback(
    Output('linkButton', 'n_clicks'),
    [Input('linkButton', 'n_clicks')],
    [Input('ddMenuActuator', 'value')],
    [Input('ddMenuSensor', 'value')],
    prevent_initial_call = True
)
def linkDev(n, aVal, sVal):
    if (n != 0) and (n != 7) and (n != 8) and (aVal is not None) and (sVal is not None):
        aDevName = aVal.split("/")[-1]
        sDevName = sVal.split("/")[-1]
        aLbl = requestVal(aVal, resourceType="AE")
        sLbl = requestVal(sVal, resourceType="AE")
        if (aDevName in sLbl) and (sDevName in aLbl):
            return 7
        if (aDevName not in sLbl):
            sLbl.append(aDevName)
            updateLabel(sVal, sLbl)
        if (sDevName not in aLbl):
            aLbl.append(sDevName)
            updateLabel(aVal, aLbl)
        return 8
    elif (n > 0) and (aVal is not None) and (sVal is not None):
        return dash.no_update
    return 0

#### Unlink Devices ####
@app.callback(
    Output('unlinkButton', 'n_clicks'),
    [Input('unlinkButton', 'n_clicks')],
    [Input('ddMenuActuator', 'value')],
    [Input('ddMenuSensor', 'value')],
    prevent_initial_call = True
)
def unlinkDev(n, aVal, sVal):
    if (n != 0) and (n != 7) and (n != 8) and (aVal is not None) and (sVal is not None):
        aDevName = aVal.split("/")[-1]
        sDevName = sVal.split("/")[-1]
        aLbl = requestVal(aVal, resourceType="AE")
        sLbl = requestVal(sVal, resourceType="AE")
        if (aDevName not in sLbl) and (sDevName not in aLbl):
            return 7
        if (aDevName in sLbl):
            sLbl.remove(aDevName)
            updateLabel(sVal, sLbl)
        if (sDevName in aLbl):
            aLbl.remove(sDevName)
            updateLabel(aVal, aLbl)
        return 8
    elif (n > 0) and (aVal is not None) and (sVal is not None):
        return dash.no_update
    return 0

#### Display Sensor Linked Devices ####
@app.callback(
    Output('alertboxSensor', 'children'),
    Output('alertboxSensor', 'style'),
    Output('alertboxSensor', 'hidden'),
    [Input('ddMenuSensor', 'value')],
    [Input('linkButton', 'n_clicks')],
    [Input('unlinkButton', 'n_clicks')],
    prevent_initial_call = True
)
def displaySenLinked(value, linkButton, unlinkButton):
    trigger = dash.callback_context
    triggerID = trigger.triggered[0]['prop_id'].split('.')[0]
    if (value is not None) and ((triggerID == 'ddMenuSensor') or (linkButton == 8) or (unlinkButton == 8)):
        devName = value.split("/")[-1]
        discovery = discover(type = "AE", rn=devName)
        if (discovery == []):
            return ["Error finding device."], alertStyle, False
        devLbl = requestVal(value, resourceType="AE")
        devLbl.remove("sensor")
        linkedDevList = ", ".join(devLbl)
        return ["Currently Linked Devices: " + linkedDevList], alertStyle, False
    if (value is None):
        return [], alertStyleHidden, True
    return dash.no_update, dash.no_update, dash.no_update

#### Display Actuator Linked Devices ####
@app.callback(
    Output('alertboxActuator', 'children'),
    Output('alertboxActuator', 'style'),
    Output('alertboxActuator', 'hidden'),
    [Input('ddMenuActuator', 'value')],
    [Input('linkButton', 'n_clicks')],
    [Input('unlinkButton', 'n_clicks')],
    prevent_initial_call = True
)
def displayActLinked(value, linkButton, unlinkButton):
    trigger = dash.callback_context
    triggerID = trigger.triggered[0]['prop_id'].split('.')[0]
    if (value is not None) and ((triggerID == 'ddMenuActuator') or (linkButton == 8) or (unlinkButton == 8)):
        devName = value.split("/")[-1]
        discovery = discover(type = "AE", rn=devName)
        if (discovery == []):
            return ["Error finding device."], alertStyle2, False
        devLbl = requestVal(value, resourceType="AE")
        devLbl.remove("actuator")
        linkedDevList = ", ".join(devLbl)
        return ["Currently Linked Devices: " + linkedDevList], alertStyle2, False
    if (value is None):
        return [], alertStyleHidden, True
    return dash.no_update, dash.no_update, dash.no_update

#### Sensor Battery Level ####
@app.callback(
    Output('sBattery', 'value'),
    Output('sBattery', 'label'),
    [Input('ddMenuSensor', 'value')],
    [Input('clkDevC', 'n_intervals')],
    [Input('sBattery', 'label')],
    prevent_initial_call = True
)
def showSensorBatteryLevel(value, n, batteryLevel):
    if (value is not None):
        battery = getLatestData(value, "Battery")
        if (battery == ''):
            return None, ""
        latestVal = str(round(float(battery), 2))
        if (batteryLevel != (latestVal + "%")):
            return float(latestVal), (latestVal + "%")
        return dash.no_update, dash.no_update
    return None, ""

#### Actuator Battery Level ####
@app.callback(
    Output('aBattery', 'value'),
    Output('aBattery', 'label'),
    [Input('ddMenuActuator', 'value')],
    [Input('clkDevC', 'n_intervals')],
    [Input('aBattery', 'label')],
    prevent_initial_call = True
)
def showActuatorBatteryLevel(value, n, batteryLevel):
    if (value is not None):
        battery = getLatestData(value, "Battery", "actuator")
        if (battery == ''):
            return None, ""
        latestVal = str(round(float(battery), 2))
        if (batteryLevel != (latestVal + "%")):
            return float(latestVal), (latestVal + "%")
        return dash.no_update, dash.no_update
    return None, ""

########################
#   oneM2M Functions   #
########################

#### Parse oneM2M response outputs ####
def getResId(tag,r):
    try:
        resId = r.json()[tag]['ri']
    except:
        resId = ""
    return resId

def getCon(tag,r):
    try:
        con = r.json()[tag]['con']
    except:
        con = ""
    return con

def getCt(tag, r):
    try:
        ct = r.json()[tag]['ct']
        ct = formatDate(ct)
    except:
        ct = ""
    return ct

def getDis(tag, r):
    try:
        list = r.json()[tag]
    except:
        list = []
    return list

def getCONs(tag, r):
    outputCONs = []
    outputDates = []    
    try:
        list = r.json()[tag]["m2m:cin"]
        for element in list:
            try:
                outputCONs.append(float(element["con"]))
                date = element["ct"]
                outputDates.append(date)
            except:
                pass
    except:
        pass
    return outputCONs, formatDate(outputDates)

def getlbl(tag, r):
    try:
        labels = r.json()[tag]['lbl']
    except:
        labels = []
    return labels

#### Functions to send oneM2M primitives and requests to the CSE ####
def createAE(resourceName, origin = originator, acpi=""):
    poa = 'http://{}:{}'.format(appIP,appPort)
    if (acpi == ""):
        payld = { "m2m:ae": { "rr": True, "api": "NR_AE001", "apn": "IOTApp", "csz": [ "application/json" ], "srv": [ "2a" ], "rn": resourceName, "poa": [ poa ] } }
    else:
                payld = { "m2m:ae": { "rr": True, "acpi": [acpi], "api": "NR_AE001", "apn": "IOTApp", "csz": [ "application/json" ], "srv": [ "2a" ], "rn": resourceName, "poa": [ poa ] } }
    print ("AE Create Request")
    print (payld)
    url = 'http://' + cseIP + ':' + csePort + '/' + cseID
    hdrs = {'X-M2M-RVI':'2a', 'X-M2M-RI':"CAE_Test",'X-M2M-Origin':origin,'Content-Type':"application/json;ty=2"}
    r = requests.post(url, data=dumps(payld), headers=hdrs)
    print ("AE Create Response")
    print (r.text)
    return getResId('m2m:ae',r)

def deleteAE(resourceName):
    url = 'http://' + cseIP + ':' + csePort + '/' + cseName + '/'+ resourceName
    hdrs = {'X-M2M-RVI':'2a','X-M2M-RI':"CAE_Test",'X-M2M-Origin':originator,'Content-Type':"application/json"}
    r = requests.delete(url,  headers=hdrs)
    print ("AE Delete Response")
    print (r.text)

def createContainer(resourceName, parentID, origin, acpi, mni = 1):
    payld = { "m2m:cnt": { "rn": resourceName, "acpi": [acpi], "mni":mni} } 
    print ("CNT Create Request")
    print (payld)
    url = 'http://' + cseIP + ':' + csePort + '/' + parentID
    hdrs = {'X-M2M-RVI':'2a','X-M2M-RI':"CAE_Test",'X-M2M-Origin':origin,'Content-Type':"application/json;ty=3"}
    r = requests.post(url, data=dumps(payld), headers=hdrs)
    print ("CNT Create Response")
    print (r.text)
    return getResId('m2m:cnt',r)

def createACP(parentID, rn, devName, origin):
    payld = { "m2m:acp": { "rn": rn, "pv": { "acr": [{"acor": [originator], "acop": 63}, {"acor": [origin], "acop": 63}, {"acor": [devName], "acop": 63}] }, "pvs": {"acr": [{"acor": [origin], "acop": 63}]}}} 
    print ("ACP Create Request")
    print (payld)
    url = 'http://' + cseIP + ':' + csePort + '/' + parentID
    hdrs = {'X-M2M-RVI':'2a','X-M2M-RI':"CAE_Test",'X-M2M-Origin':origin,'Content-Type':"application/json;ty=1"}
    r = requests.post(url, data=dumps(payld), headers=hdrs)
    print ("ACP Create Response")
    print (r.text)
    return getResId('m2m:acp',r)

def createACPNotif(parentID, rn):
    payld = { "m2m:acp": { "rn": rn, "pv": { "acr": [{"acor": ["all"], "acop": 16}, {"acor": [originator], "acop": 63}] }, "pvs": {"acr": [{"acor": [originator], "acop": 63}]}}} 
    print ("ACP Create Request")
    print (payld)
    url = 'http://' + cseIP + ':' + csePort + '/' + parentID
    hdrs = {'X-M2M-RVI':'2a','X-M2M-RI':"CAE_Test",'X-M2M-Origin':"CAdmin",'Content-Type':"application/json;ty=1"}
    r = requests.post(url, data=dumps(payld), headers=hdrs)
    print ("ACP Create Response")
    print (r.text)
    return getResId('m2m:acp',r)

def requestVal(addr, resourceType="CIN"):
    url = 'http://' + cseIP + ':' + csePort + '/' + addr
    hdrs = {'X-M2M-RVI':'2a','X-M2M-RI':"CAE_Test",'X-M2M-Origin':originator, 'Accept':"application/json"}
    r = requests.get(url, headers=hdrs)
    if resourceType == "AE":
        return getlbl('m2m:ae', r)
    return getCon('m2m:cin',r), getCt('m2m:cin',r)

def requestAllCIN(addr):
    url = 'http://' + cseIP + ':' + csePort + '/' + addr + "?rcn=4"
    hdrs = {'X-M2M-RVI':'2a','X-M2M-RI':"CAE_Test",'X-M2M-Origin':originator, 'Accept':"application/json"}
    r = requests.get(url, headers=hdrs)
    return getCONs('m2m:cnt', r)

def updateLabel(addr, lbl):
    payld = {"m2m:ae": {"lbl": lbl}}
    url = 'http://' + cseIP + ':' + csePort + '/' + addr
    hdrs = {'X-M2M-RVI':'2a','X-M2M-RI':'CAE_Test','X-M2M-Origin':originator, 'Accept':'application/json', 'Content-Type':'application/json'}
    r = requests.put(url, data=dumps(payld), headers=hdrs)

def discover(type="", label="", location=cseID, rn="", lvl=""):
    if (type == "AE"):
        ty = "&ty=2"
    elif (type == "Container"):
        ty = "&ty=3"
    elif (type == "Content Instance"):
        ty = "&ty=4"
    elif (type == "Sub"):
        ty = "&ty=23"
    elif (type == "ACP"):
        ty = "&ty=1"
    else:
        ty = ""
    
    if (label != ""):
        label = "&lbl=" + label
    if (rn != ""):
        rn = "&rn=" + rn
    if (lvl != ""):
        lvl = "&lvl=" + lvl

    url = 'http://' + cseIP + ':' + csePort + '/' + location + '?fu=1' + ty + label + rn + lvl
    hdrs = {'X-M2M-RVI':'2a','X-M2M-RI':"CAE_Test",'X-M2M-Origin':originator, 'Accept':"application/json"}
    r = requests.get(url, headers=hdrs)
    return getDis('m2m:uril',r)

def createSubscription(resourceName, parentID, net, origin=originator):
    payld = { "m2m:sub": { "rn": resourceName, "enc": {"net":net}, "nu":[originator]} }
    print ("Sub Create Request")
    print (payld)
    url = 'http://' + cseIP + ':' + csePort + '/' + parentID
    hdrs = {'X-M2M-RVI':'2a','X-M2M-RI':"Sub",'X-M2M-Origin':origin,'Content-Type':"application/json;ty=23"}
    r = requests.post(url, data=dumps(payld), headers=hdrs)
    print ("SUB Create Response")
    print (r.text)
    return getResId('m2m:sub',r)

def createContentInstance(content, parentID):
    payld = { "m2m:cin": {"cnf": "application/text:0", "con": content} }
    print ("CIN Create Request")
    print (payld)
    url = 'http://' + cseIP + ':' + csePort + '/' + parentID
    hdrs = {'X-M2M-RVI':'2a','X-M2M-RI':"CAE_Test",'X-M2M-Origin':originator,'Content-Type':"application/json;ty=4"}
    r = requests.post(url, data=dumps(payld), headers=hdrs)
    print ("CIN Create Response")
    print (r.text)


##########################
#     Functions to       #
#   manage sensor and    #
#  actuator data values  #
##########################

#### Fromat Dates to fit expected graph formatting and ET timezone ####
def formatDate(ct):
    if (isinstance(ct, str)):
        return adjustTimeZone([ct[0:4] + "-" + ct[4:6] + "-" + ct[6:11] + ":" + ct[11:13] + ":" + ct[13:15]])[0]
    return adjustTimeZone([date[0:4] + "-" + date[4:6] + "-" + date[6:11] + ":" + date[11:13] + ":" + date[13:15] for date in ct])
    
def adjustTimeZone(dates):
    dates = pd.to_datetime(dates)
    dates = dates.tz_localize('UTC').tz_convert('US/Eastern')
    return [x.strftime("%Y-%m-%dT%H:%M:%S") for x in dates]

#### Convert temperatures to Fahrenheit ####
def convertToFahrenheit(tempC):
    return [(((9/5) * temp) + 32) for temp in tempC]

#### Obtain Sensor Data Values from CSE #### 
def obtainSensorData(url):
    global sensors
    sensorName = url.split("/")[-1]
    sensorDict = {'url': url, 'rn': sensorName, 'cnt': []}
    discoverContainers = discover(type="Container", location=url)
    for cnt in discoverContainers:
        cntName = cnt.split("/")[-1]
        if (cntName in ["Temperature", "SoilMoisture", "Humidity", "RainfallTrigger", "Battery", "GPS"]):
            if (cntName in ["RainfallTrigger", "Battery", "GPS"]):
                latestVal, latestDate = requestVal(cnt + "/la")
                sensorDict['cnt'].append({'rn': cntName, 'cin': latestVal, 'ct': latestDate})
            else:                        
                values, dates = requestAllCIN(cnt)
                if (cntName == "Temperature"):
                    values = convertToFahrenheit(values)
                if (len(values) > 1):
                    # Sort data based on dates (ascending order - i.e. latest is last)
                    sorted_lists = sorted(zip(dates, values))
                    dates, values = [list(tuple) for tuple in zip(*sorted_lists)]
                sensorDict['cnt'].append({'rn': cntName, 'cin': values, 'ct': dates})

            subCheck = discover(type = "23", location = cnt, rn = cntName + "Sub")
            if (subCheck == []):
                createSubscription(cntName + "Sub", cnt, [3])

    sensors.append(sensorDict)

#### Obtain Sensor Data Values from CSE #### 
def obtainActuatorData(url):
    global actuators
    actuatorName = url.split("/")[-1]
    actuatorDict = {'url': url, 'rn': actuatorName,'cnt': []}
    discoverContainers = discover(type="Container", location=url)
    for cnt in discoverContainers:
        cntName = cnt.split("/")[-1]
        if (cntName == "actuatorState") or (cntName == "Battery"):
            latestVal, latestDate = requestVal(cnt + "/la")
            actuatorDict['cnt'].append({'rn': cntName, 'cin': latestVal, 'ct': latestDate})

            subCheck = discover(type = "23", location = cnt, rn = cntName + "Sub")
            if (subCheck == []):
                createSubscription(cntName + "Sub",cnt, [3])

    actuators.append(actuatorDict)

#### Add newly recieved content instance  ####
def addNewCIN(rn, cntName, ct, cin, aeType = "sensor"):

    global sensors
    global actuators

    if (cntName in ["Temperature", "Humidity", "SoilMoisture"]) and (cin != ""):
        cin = float(cin)
    
    if aeType == "actuator":
        actIndex, cntIndex = None, None
        for actuator in actuators:
            if actuator['rn'] == rn:
                actIndex = actuators.index(actuator)
                for cnt in actuator['cnt']:
                    if cnt['rn'] == cntName:
                        cntIndex = actuator['cnt'].index(cnt)
                        break
                break

        if (actIndex != None) and (cntIndex != None):
            actuators[actIndex]['cnt'][cntIndex]['ct'] = ct
            actuators[actIndex]['cnt'][cntIndex]['cin'] = cin
    else:
        sensorIndex, cntIndex = None, None
        for sensor in sensors:
            if sensor['rn'] == rn:
                sensorIndex = sensors.index(sensor)
                for cnt in sensor['cnt']:
                    if cnt['rn'] == cntName:
                        cntIndex = sensor['cnt'].index(cnt)
                        break
                break
        if (sensorIndex != None) and (cntIndex != None): 
            if cntName in ["RainfallTrigger", "Battery", "GPS"]:
                sensors[sensorIndex]['cnt'][cntIndex]['ct'] = ct
                sensors[sensorIndex]['cnt'][cntIndex]['cin'] = cin
            else:
                if (cntName == "Temperature") and (cin != ""):
                    cin = convertToFahrenheit([cin])[0]
                if (len(sensors[sensorIndex]['cnt'][cntIndex]['ct']) >= 50):
                    sensors[sensorIndex]['cnt'][cntIndex]['ct'] = sensors[sensorIndex]['cnt'][cntIndex]['ct'][-49:]
                    sensors[sensorIndex]['cnt'][cntIndex]['cin'] = sensors[sensorIndex]['cnt'][cntIndex]['cin'][-49:]
                sensors[sensorIndex]['cnt'][cntIndex]['ct'].append(ct)
                sensors[sensorIndex]['cnt'][cntIndex]['cin'].append(cin)

# Find all data to graph: only used for Temperature, Humidity, SoilMoisture containers
def getAllData(url, cntName):
    global sensors
    for sensor in sensors:
            if sensor['url'] == url:
                for cnt in sensor['cnt']:
                    if cnt['rn'] == cntName:
                            return cnt['cin'], cnt['ct']
    return [], []

# Find latest data: only used for Battery, RainfallTrigger, GPS, actuatorState containers
def getLatestData(url, cntName, aeType="sensor"):
    global sensors
    global actuators
    if aeType == "actuator":
        for actuator in actuators:
            if actuator['url'] == url:
                for cnt in actuator['cnt']:
                    if cnt['rn'] == cntName:
                        if cnt['cin'] != []:
                            return cnt["cin"]
    else:
        for sensor in sensors:
            if sensor['url'] == url:
                for cnt in sensor['cnt']:
                    if cnt['rn'] == cntName:
                        if cnt['cin'] != []:
                            return cnt["cin"]
    return ""

# Get all the currently registered AEs and the CINs of their CNTs (used at startup)
def obtainAllAEs():

    discoverSensors = discover(type="AE", label="sensor")
    for sensor in discoverSensors:
        obtainSensorData(sensor)

    discoverActuators = discover(type="AE", label="actuator")
    for actuator in discoverActuators:
        obtainActuatorData(actuator)

##########################
#                        #
#   Background Threads   #
#                        #
##########################

#### Notifications/HTTP Handler Thread ####

# HTTP Request Handler
class SimpleHTTPRequestHandler(BaseHTTPRequestHandler): 

    def do_POST(self) -> None:
        global sensors
        global actuators

        # Construct return header
        # ACME CSE requires rsc value of 2000 returned
        mySendResponse(self, 200, '2000')

        # Get headers and content data
        length = int(self.headers['Content-Length'])
        contentType = self.headers['Content-Type']
        post_data = self.rfile.read(length)
        
        # Print the content data
        print('### Notification')
        print (self.headers)
        print(post_data.decode('utf-8'))
        r = loads(post_data.decode('utf8').replace("'", '"'))
        # Obtain the net value (added or deleted)
        try:
            net = r['m2m:sgn']['nev']['net']
        except:
            net = None
        # Try a statement to parse if its an AE
        try:
            rn = r['m2m:sgn']['nev']['rep']['m2m:ae']['rn']
        except:
            rn = ""
        # If its an AE handle cases that add/remove
        if (rn != ""):
            try:
                lbl = r['m2m:sgn']['nev']['rep']['m2m:ae']['lbl']
            except:
                lbl = []
            if "sensor" in lbl:
                if (net == 3):
                    # add to sensor list to subscribe to its containers
                    newSensorList.append("cse-in/"+rn)
                elif (net == 4):
                    for sensor in sensors:
                        if (sensor['rn'] == rn):
                            sensors.remove(sensor)
                            break
            elif "actuator" in lbl:
                if (net == 3):
                    # add to actuator list to subscribe to its containers
                    newActuatorList.append("cse-in/"+rn)
                elif (net == 4):
                    for actuator in actuators:
                        if (actuator['rn'] == rn):
                            actuators.remove(actuator)
                            break
        # If its not an AE, then its probably a content instance
        elif (rn == ""):
            try:
                con = r['m2m:sgn']['nev']['rep']['m2m:cin']['con']
            except:
                con = ""
            
            if (con != ""):
                try:
                    lbl = r['m2m:sgn']['nev']['rep']['m2m:cin']['lbl']
                except:
                    lbl = []

                if (lbl != []):
                    ct = r['m2m:sgn']['nev']['rep']['m2m:cin']['ct']
                    ct = formatDate(ct)
                    lbl = lbl[0].split("/")
                    if lbl[-1] == "Battery":
                        label = requestVal("cse-in/" + lbl[0], resourceType="AE")
                        if "sensor" in label:
                            addNewCIN(lbl[0], lbl[-1], ct, con, aeType="sensor")
                        elif "actuator" in label:
                            addNewCIN(lbl[0], lbl[-1], ct, con, aeType="actuator")
                    elif lbl[-1] == "actuatorState":
                        addNewCIN(lbl[0], lbl[-1], ct, con, aeType="actuator")
                    else:
                        addNewCIN(lbl[0], lbl[-1], ct, con, aeType="sensor")

        # Do nothing if its neither an AE or CIN -> its probably a sub creation notif or acp 

# Send appropriate response for oneM2M ACME CSE
def mySendResponse(self, responseCode, rsc):
        self.send_response(responseCode)
        self.send_header('X-M2M-RSC', rsc)
        self.end_headers()

# Run/Start HTTP server
def run_http_server():
    httpd = HTTPServer(('', appPort), SimpleHTTPRequestHandler)
    print('**starting server & waiting for connections**')
    httpd.serve_forever()
            
#### Thread to create subscriptions for newly registered devices ####
# Containers for an AE won't be created till a response is generated.
# Thus, we have to manage subs after responding to an AE and waiting 
# a while (by sleeping) for the AE to create its containers.
def manage_cnt_subs():
    global newSensorList
    global newActuatorList

    while True:
        for sensor in newSensorList:
            # Wait for the AE to create all its containers
            time.sleep(5)
            # Subscribe to the desired containers
            obtainSensorData(sensor)
            newSensorList.remove(sensor)
        
        for actuator in newActuatorList:
            # Wait for the AE to create all its containers
            time.sleep(5)
            # Subscribe to the desired containers
            obtainActuatorData(actuator)
            newActuatorList.remove(actuator)

#### Run app and start background threads ####
if __name__ == "__main__":

    # Start HTTP server to handle subscription notifications
    threading.Thread(target=run_http_server, daemon=True).start()

    # Start thread to subscribe to newly connected AEs
    threading.Thread(target=manage_cnt_subs, daemon=True).start()

    # Verify that AE, ACP are created
    discoverAE = discover(type="AE", rn = appName)
    discoverACP = discover(type="ACP", rn = appName + "NotifACP")

    # Create ACP if it doesnt exist
    if (discoverACP == []):
        acpID = createACPNotif(cseID, appName + "NotifACP")

    # Create AE if it doesn't exist
    if (discoverAE == []):      
        aeID = createAE(appName, acpi="cse-in/" + appName + "NotifACP")

    # Create SUB if it doesn't exist
    discoverSUB = discover(type="Sub", rn = "deviceSub")
    if (discoverSUB == []):
        subID = createSubscription("deviceSub", cseID, [3, 4])    

    # Obtain all current sensors, actuators and their CNTs/CINs
    obtainAllAEs()
    
    # Run dashboard app
    app.run_server(debug=False, host = ip,  port=port, use_reloader=False)


