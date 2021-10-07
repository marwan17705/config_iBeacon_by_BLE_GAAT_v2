# config_iBeacon_by_BLE_GAAT_v2

- procedure for config iBeacon<br>
1- hold pin0 (GPIO_0) around 6 seconds to enter ibeacon config mode.<br>
2- LED (at GPIO_2) power on if esp32 is entering ibeacon config mode.<br>

  ----iBeacon config mode----<br>
  1- Go to SERVICES: 4fafc201-1fb5-459e-8fcc-c5c9c331914bin<br>
  2- Write something in SERVICES<br>
  CHARACTERISTIC : beb5483e-36e1-4688-b7f5-ea07361b26a8 to config major<br>
  CHARACTERISTIC : beb5483e-36e1-4688-b7f5-ea07361b26a9 to config minor<br>
  CHARACTERISTIC : beb5483e-36e1-4688-b7f5-ea07361b26aa to config tx power<br>
  (help::  0:-12 dbm   0:-9 dbm   0:-6 dbm   0:-3 dbm   0:0 dbm   0:3 dbm   0:6 dbm   0:9 dbm)<br>
  CHARACTERISTIC : beb5483e-36e1-4688-b7f5-ea07361b26ab to config time interval<br>

3- again press pin0 (GPIO_0) to summit modification so that esp32 again broadcasts iBeacon.<br>



