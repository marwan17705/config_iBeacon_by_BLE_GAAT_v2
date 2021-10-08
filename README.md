# config_iBeacon_by_BLE_GAAT_v2

- procedure for config iBeacon (or see video https://www.youtube.com/watch?v=niM6pOsLDkg)<br>


1- hold pin0 (GPIO_0) around 6 seconds to enter ibeacon config mode.<br>
2- LED (at GPIO_2) on if esp32 is entering ibeacon config mode and displays .<br>

      ----iBeacon config mode----
      1- Go to SERVICE: 4fafc201-1fb5-459e-8fcc-c5c9c331914b 
      2- Write something in CHARACTERISTICS
      CHARACTERISTIC : beb5483e-36e1-4688-b7f5-ea07361b26a8 to config major
      CHARACTERISTIC : beb5483e-36e1-4688-b7f5-ea07361b26a9 to config minor
      CHARACTERISTIC : beb5483e-36e1-4688-b7f5-ea07361b26aa to config tx power
      (help::  0:-12 dbm   0:-9 dbm   0:-6 dbm   0:-3 dbm   0:0 dbm   0:3 dbm   0:6 dbm   0:9 dbm)
      CHARACTERISTIC : beb5483e-36e1-4688-b7f5-ea07361b26ab to config time interval

3- again press pin0 (GPIO_0) to summit modification so that esp32 again broadcasts iBeacon.<br>



this video shows current consumption for this code.<br>
https://www.youtube.com/watch?v=O3cVfEwkkng
**(Click subtitle to display various modes.)
