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

#### Banner ####
def build_banner():
    return html.Div(
        id="banner",
        className="banner",
        children=[
            html.Div(
                id="banner-text",
                children=[
                    html.H5("oneM2M-based Smart City Dashboard"),
                    html.H6("Capstone Team PJ5C"),
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


#### App Layout ####
app.layout = html.Div(
    id="big-app-container",
    children=[
        build_banner()
    ]
)

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
    hdrs = {'X-M2M-RVI':'3', 'X-M2M-RI':"CAE_Test",'X-M2M-Origin':origin,'Content-Type':"application/json;ty=2"}
    r = requests.post(url, data=dumps(payld), headers=hdrs)
    print ("AE Create Response")
    print (r.text)
    return getResId('m2m:ae',r)

def deleteAE(resourceName):
    url = 'http://' + cseIP + ':' + csePort + '/' + cseName + '/'+ resourceName
    hdrs = {'X-M2M-RVI':'3','X-M2M-RI':"CAE_Test",'X-M2M-Origin':originator,'Content-Type':"application/json"}
    r = requests.delete(url,  headers=hdrs)
    print ("AE Delete Response")
    print (r.text)

def createContainer(resourceName, parentID, origin, acpi, mni = 1):
    payld = { "m2m:cnt": { "rn": resourceName, "acpi": [acpi], "mni":mni} } 
    print ("CNT Create Request")
    print (payld)
    url = 'http://' + cseIP + ':' + csePort + '/' + parentID
    hdrs = {'X-M2M-RVI':'3','X-M2M-RI':"CAE_Test",'X-M2M-Origin':origin,'Content-Type':"application/json;ty=3"}
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

def discover(type="", label="", location=cseID, rn="", lvl="", api=""):
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
    if api != "":
        api = "&api=" + api

    url = 'http://' + cseIP + ':' + csePort + '/' + location + '?fu=1' + ty + label + rn + lvl + api
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

#### Run app and start background threads ####
if __name__ == "__main__":

    # Start HTTP server to handle subscription notifications
    #threading.Thread(target=run_http_server, daemon=True).start()
    # Run dashboard app
    app.run_server(debug=True, host=dashHost, port=appPort, use_reloader=False)

    print("Discovering dashboard AE and ACP...")

    # Verify that AE, ACP are created
    discoverAE = discover(type="AE", rn = appName)
    discoverACP = discover(type="ACP", rn = appName + "ACP")

    # Create ACP if it doesnt exist
    if discoverACP == []:
        print("creating ACP")
        acpID = createACPNotif(cseID, appName + "ACP")

    # Create AE if it doesn't exist
    if discoverAE == []:     
        print("creating AE") 
        aeID = createAE(appName, acpi="cse-in/" + appName + "ACP")

    print("Discovering Intersection AEs...")

    # Obtain all intersections
    discoveredIntersections = discover(type="AE", label="sensor")
    for i in discoveredIntersections:
        print(i)   
