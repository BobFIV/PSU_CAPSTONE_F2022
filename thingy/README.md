## Compiling on Windows
To compile this code, you will need to install the nRF Connect SDK from Nordic Semiconductor.

The Thingy:91 has two chips on it: the nRF9160, and the nRF52840.  
This means that source code has to be developed for each chip and compiled independently.  

### Compiling the Thingy91:nRF52840 Code
Steps to compile:
1. `cd traffic_light_nrf52840`
2. Set the environment variable`
  - On Windows this is: `set ZEPHYR_BASE=E:\nRF_SDK_ncs\v2.1.0\zephyr`
  - On Linux: `ZEPHYR_BASE=<your-nrf-install-location>/v2.1.0/zephyr`
2. `cmake CMakeLists.txt -Bbuild -DCMAKE_PREFIX_PATH=E:\nRF_SDK_ncs\toolchains\v2.1.0 -DBOARD=thingy91_nrf52840 -GNinja`
3. `cd build`
4. `ninja`

On a Windows system, you will need to make sure that CMake and Ninja are both on your `PATH` environment variable. 


## Linux based thingy:91 dev notes
---

Follow instructions on Nordic's website for downloading nRF connect, VS Code etc.


