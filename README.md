# PSU_CAPSTONE_F2022
Code repository for Penn State CMPEN482W Engineering Capstone Project: Fall 2022

## Overview
This repository contains code and instructions for creating a cloud-controlled IoT traffic light intersection using a [Nordic Semiconductor Thingy:91](https://www.nordicsemi.com/Products/Development-hardware/Nordic-Thingy-91), and an [ESP32 based microcontroller from HiLETGo](https://www.amazon.com/HiLetgo-ESP-WROOM-32-Development-Microcontroller-Integrated/dp/B0718T232Z).

The IoT protocol empowering this design is [oneM2M](https://www.onem2m.org/).  
The oneM2M standard provides developers with the ability to rapidly create IoT devices that are scalable, secure, and interoperable.

The currently provided instructions are for Amazon Web Services, however other cloud providers can be easily used.

## Web Dashboard
A simple React JS single page web app has be created for controlling and viewing the state of the traffic lights.
The code and instructions can be found in the [traffic-light-dashboard](traffic-light-dashboard) folder.

## Thingy:91
The code and instructions for programming the Thingy:91 are located in the [thingy](thingy) folder.

## ESP32
The code and instructions for programming the ESP21 are located in the [esp32](esp32) folder.

## oneM2M CSE
The oneM2M standard requires a oneM2M network to contain a Common Services Entity (CSE).  
This CSE provides many different features, but in our case is considered as the database for our application.  

Our project uses the [ACME CSE](https://github.com/ankraft/ACME-oneM2M-CSE) developed by Andreas Kraft and other contributors.  

## Thanks
We would like to thank the sponsor for this project, [Exacta Global Smart Solutions](http://exactagss.com/). Exacta GSS is a leading edge consultant for oneM2M IoT projects.

We would also like to thank [Nordic Semicondictor](https://www.nordicsemi.com/) for their support while developing on the Thingy:91 prototyping platform.  
