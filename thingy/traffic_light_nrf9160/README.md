# Thingy:91 nRF9160 Building Notes  
You must set the board type to the non-secure variant: `BOARD_TYPE=thingy91_nrf9160_ns`  

Because of using the ARM TFM, the CMake build system will generate VERY long path names.  
On Windows machines, long path names cause the builds to fail.  
If you are compiling on a Windows machine, set your CMake build directory to the top of your drive (ie. `-BE:\build_traffic_light_ae_9160`)  