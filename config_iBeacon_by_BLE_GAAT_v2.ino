#include "sys/time.h"

#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "esp_sleep.h"
#include <EEPROM.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t temprature_sens_read();
//uint8_t g_phyFuns;

#ifdef __cplusplus
}
#endif

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
BLEAdvertising *pAdvertising;
BLEServer *gattServer;
BLEService *gattService;
struct timeval now;
// gatt_uuid
#define SERVICE_UUID_GATT       "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_GATT "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_GATT2 "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define CHARACTERISTIC_UUID_GATT3 "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define CHARACTERISTIC_UUID_GATT4 "beb5483e-36e1-4688-b7f5-ea07361b26ab"

#define BEACON_UUID           "b4b89731-4797-dc8a-3a48-d53c424fa9c8" // UUID 1 128-Bit (may use linux tool uuidgen or random numbers via https://www.uuidgenerator.net/)
#define MFID 0x4C00
//#define MAJOR 1200
//#define MINOR 270
#define TX_POWER 0xC3
#define APPLE_FLAG 0x04
//#define MIN_INTV 0x64
//#define MAX_INTV 0x0800

#define PIN_interupt 0
#define PIN_led 2
#define EEPROM_SIZE 64

typedef struct
{
  uint16_t MAJOR;
  uint16_t MINOR;
  esp_power_level_t tx_power;
  uint8_t tx_power_level;
  uint16_t interval;
  uint8_t rssi_1m;
} ibeacon_conf;

static ibeacon_conf ibeacon_cf;

volatile byte in_cf_mode = 0;
volatile byte enter_cf = LOW;
uint64_t timestamp = 0;
void setBeacon();
/**
   Bluetooth TX power level(index), it's just a index corresponding to power(dbm).
 * * ESP_PWR_LVL_N12 (-12 dbm)  0
 * * ESP_PWR_LVL_N9  (-9 dbm)  1
 * * ESP_PWR_LVL_N6  (-6 dbm)  2
 * * ESP_PWR_LVL_N3  (-3 dbm)  3
 * * ESP_PWR_LVL_N0  ( 0 dbm)  4
 * * ESP_PWR_LVL_P3  (+3 dbm)  5
 * * ESP_PWR_LVL_P6  (+6 dbm)  6
 * * ESP_PWR_LVL_P9  (+9 dbm)  7
*/

#define POWER_LEVEL ESP_PWR_LVL_N3

void EEPROM_write(int index, String text) 
{
  for (int i = index; i < text.length() + index; i++) 
  {
    EEPROM.put(i, text[i - index]);
  }
    EEPROM.commit();
}

void store_eeprom(void * arg,int type,int row)
{
  if(type==1)
  {
    uint8_t dec = *((uint8_t*)arg);
    printf("EEPROM.write: %d %d\n",row*10,dec);
    EEPROM.write(row*10, dec);

  }
  else if(type==2)
  {
    uint16_t dec = *((uint16_t*)arg);
    printf("EEPROM.write: %d %d\n",row*10,dec);
    EEPROM.write(row*10, dec);
    EEPROM.write(row*10+1, dec>>8);
  }
    EEPROM.commit();
}

void read()
{
    uint16_t ch= EEPROM.read(0)+ EEPROM.read(1)*256;
    printf("EEPROM.read:0 %d\n",ch);
    uint8_t ch1= EEPROM.read(1*10);
    printf("EEPROM.read:10 %d\n",ch1);
}

ibeacon_conf open_ibeacon_cf_from_eeprom()
{
  ibeacon_conf ibeacon_cf;
  ibeacon_cf.MAJOR= EEPROM.read(0)+ EEPROM.read(1)*256;
  ibeacon_cf.MINOR= EEPROM.read(10)+ EEPROM.read(11)*256;
  ibeacon_cf.tx_power_level= EEPROM.read(20);
  if(ibeacon_cf.tx_power_level > 7 ||ibeacon_cf.tx_power_level <0 )
    ibeacon_cf.tx_power_level= 4 ;
  ibeacon_cf.tx_power = getTx_power(ibeacon_cf.tx_power_level);
  ibeacon_cf.rssi_1m = getRssi_1m(ibeacon_cf.tx_power);
  ibeacon_cf.interval=  EEPROM.read(30)+ EEPROM.read(31)*256;

  if(ibeacon_cf.tx_power_level > 7 ||ibeacon_cf.tx_power_level <0 )
    ibeacon_cf.tx_power_level= 4 ;
  if(ibeacon_cf.interval < 100)
    ibeacon_cf.interval= 100;

  
  return ibeacon_cf;
}

void store_ibeacon_cf_to_eeprom(ibeacon_conf cf)
{
  store_eeprom(&cf.MAJOR,2,0);
  store_eeprom(&cf.MINOR,2,1);
  store_eeprom(&cf.tx_power_level,1,2);
  store_eeprom(&cf.interval,2,3);
}

void Reset_EEPROM()
{
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.put(i, 0);
    }
    if(EEPROM.commit()){
      Serial.println("Reset pass");
    }
    delay(500);
}

uint8_t getRssi_1m(esp_power_level_t tx_power)
{
  if (tx_power == ESP_PWR_LVL_N12)
    return 0xB3;//-77;
  else if (tx_power == ESP_PWR_LVL_N9)
    return 0xB6; //-74
  else if (tx_power == ESP_PWR_LVL_N6)
    return 0xB9;//-71
  else if (tx_power == ESP_PWR_LVL_N3)
    return 0xBC ;//-68
  else if (tx_power == ESP_PWR_LVL_N0)
    return 0xBF;//-65
  else if (tx_power == ESP_PWR_LVL_P3)
    return 0xC3;//-61
  else if (tx_power == ESP_PWR_LVL_P6)
    return 0xC9;//-55
  else if (tx_power == ESP_PWR_LVL_P9)
    return 0xCC;//-52
}


esp_power_level_t getTx_power(char tx_power)
{
  if (tx_power == 0)
    return ESP_PWR_LVL_N12;
  else if (tx_power == 1)
    return ESP_PWR_LVL_N9; //
  else if (tx_power == 2)
    return ESP_PWR_LVL_N6;//
  else if (tx_power == 3)
    return ESP_PWR_LVL_N3;
  else if (tx_power == 4)
    return ESP_PWR_LVL_N0;
  else if (tx_power == 5)
    return ESP_PWR_LVL_P3;//
  else if (tx_power == 6)
    return ESP_PWR_LVL_P6;//
  else if (tx_power == 7)
    return ESP_PWR_LVL_P9;//
}



class gattCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      std::string uuid_str = pCharacteristic->getUUID().toString().c_str();

      if (uuid_str == CHARACTERISTIC_UUID_GATT)
      {
        Serial.print("major:");
        if (value.length() <= 2 && value.length() > 0)
        {
          ibeacon_cf.MAJOR = value[0] + value[1] * 256;
          Serial.println(ibeacon_cf.MAJOR);
        }
        else
          Serial.println("invalid size data");
      }
      else if (uuid_str == CHARACTERISTIC_UUID_GATT2)
      {
        Serial.print("minor:");
        if (value.length() <= 2 && value.length() > 0)
        {
          ibeacon_cf.MINOR = value[0] + value[1] * 256;
          Serial.println(ibeacon_cf.MINOR);
        }
        else
          Serial.println("invalid size data");
      }
      else if (uuid_str == CHARACTERISTIC_UUID_GATT3)
      {
        Serial.print("tx_power");
        if (value.length() == 1 && value.length() > 0)
        {
          ibeacon_cf.tx_power_level=value[0];
          ibeacon_cf.tx_power = getTx_power(value[0]);
          ibeacon_cf.rssi_1m = getRssi_1m(ibeacon_cf.tx_power);
          Serial.print(ibeacon_cf.tx_power_level);
          Serial.print(" at 1meter  ");
          Serial.println(ibeacon_cf.rssi_1m);
          if(ibeacon_cf.tx_power_level > 7 ||ibeacon_cf.tx_power_level <0 )
            ibeacon_cf.tx_power_level= 4 ;
        }
        else
          Serial.println("invalid size data");
      }
      else if (uuid_str == CHARACTERISTIC_UUID_GATT4)
      {
        Serial.print("interval:");
        if (value.length() <= 2 && value.length() > 0)
        {
          ibeacon_cf.interval = value[0] + value[1] * 256;
          Serial.println(ibeacon_cf.interval);
          if(ibeacon_cf.interval < 100)
            ibeacon_cf.interval= 100;
        }
        else
          Serial.println("invalid size data");
      }
      else
        Serial.println("incorrect uuid");
    }
};




void setBeacon() {
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ibeacon_cf.tx_power);
  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(MFID); // fake Apple 0x004C LSB (ENDIAN_CHANGE_U16!)
  oBeacon.setProximityUUID(BLEUUID(BEACON_UUID));
  oBeacon.setMajor(ibeacon_cf.MAJOR);
  oBeacon.setMinor(ibeacon_cf.MINOR);
  oBeacon.setSignalPower(ibeacon_cf.rssi_1m);

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();

  oAdvertisementData.setFlags(APPLE_FLAG); // BR_EDR_NOT_SUPPORTED 0x04

  // String ib_name = "OneBeacon-" + String(ibeacon_cf.MAJOR) + "-" + String(ibeacon_cf.MINOR);
  //  oAdvertisementData.setName(ib_name.c_str());

  std::string strServiceData = "";

  strServiceData += (char)26;     // Len
  strServiceData += (char)0xFF;   // Type
  strServiceData += oBeacon.getData();

  oAdvertisementData.addData(strServiceData);

  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponseData(oScanResponseData);
  pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);

  uint16_t interval = ibeacon_cf.interval / 0.625 ;
  pAdvertising->setMinInterval(interval);
  pAdvertising->setMaxInterval(interval);
}

void config_ibeacon()
{
  // Serial.println(millis()-timestamp);
  if (millis() - timestamp > 1000)
  {
    enter_cf = !enter_cf;
    Serial.println(enter_cf);
    timestamp = millis();
  }
}




void init_gatt()
{

//  BLEDevice::init("config_onechat_ibeacon");

  gattServer = BLEDevice::createServer();
  gattService = gattServer->createService(SERVICE_UUID_GATT);
  BLECharacteristic *pCharacteristic_major = gattService->createCharacteristic(
        CHARACTERISTIC_UUID_GATT,
        //  BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE
      );
  BLECharacteristic *pCharacteristic_minor = gattService->createCharacteristic(
        CHARACTERISTIC_UUID_GATT2,
        BLECharacteristic::PROPERTY_WRITE
      );
  BLECharacteristic *pCharacteristic_tx_power = gattService->createCharacteristic(
        CHARACTERISTIC_UUID_GATT3,
        BLECharacteristic::PROPERTY_WRITE
      );
  BLECharacteristic *pCharacteristic_interval = gattService->createCharacteristic(
        CHARACTERISTIC_UUID_GATT4,
        BLECharacteristic::PROPERTY_WRITE
      );

  pCharacteristic_major->setCallbacks(new gattCallbacks());
  pCharacteristic_minor->setCallbacks(new gattCallbacks());
  pCharacteristic_tx_power->setCallbacks(new gattCallbacks());
  pCharacteristic_interval->setCallbacks(new gattCallbacks());

  pCharacteristic_major->setValue("major");
  pCharacteristic_minor->setValue("minor");
  pCharacteristic_tx_power->setValue("tx_power");
  pCharacteristic_interval->setValue("interval");



  //  gattService->start();
  //  BLEAdvertising *pAdvertising = gattServer->getAdvertising();
  //  pAdvertising->start();

}



void setup() {


  EEPROM.begin(EEPROM_SIZE);

  ibeacon_cf = open_ibeacon_cf_from_eeprom(); 

  Serial.begin(115200);
  Serial.println("new started...");
  BLEDevice::init("");
//  BLEDevice::init("config_onechat_ibeacon");

//  init_gatt();

  // Create the BLE Server
  pAdvertising = BLEDevice::getAdvertising();
  setBeacon();
  //Start advertising
  pAdvertising->start();
  pinMode(PIN_led, OUTPUT);
  pinMode(PIN_interupt, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_interupt), config_ibeacon, RISING);
  digitalWrite(PIN_led, 1);
  //Serial.println("Advertizing started...");
  
//   vTaskDelay( 300 / portTICK_PERIOD_MS );
//  //  pAdvertising->stop();
//  //  Serial.printf("enter deep sleep\n");
//  if(!enter_cf)
//    esp_deep_sleep(5e5);
//  //  Serial.printf("in deep sleep\n");
}

void display_menu_ibeacon_setting()
{
    Serial.println("----iBeacon config mode----");
    Serial.println("1- Go to CHARACTERISTIC: 4fafc201-1fb5-459e-8fcc-c5c9c331914bin");
    Serial.println("2- Write something in SERVICES");
    Serial.print("SERVICES : ");
    Serial.print(CHARACTERISTIC_UUID_GATT);
    Serial.println(" to config major");
    Serial.print("SERVICES : ");
    Serial.print(CHARACTERISTIC_UUID_GATT2);
    Serial.println(" to config minor");
    Serial.print("SERVICES : ");
    Serial.print(CHARACTERISTIC_UUID_GATT3);
    Serial.println(" to config tx power");
    Serial.println("(help::  0:-12 dbm   0:-9 dbm   0:-6 dbm   0:-3 dbm   0:0 dbm   0:3 dbm   0:6 dbm   0:9 dbm)");
    Serial.print("SERVICES : ");
    Serial.print(CHARACTERISTIC_UUID_GATT4);
    Serial.println(" to config time interval");
}

void loop() {

  for(int i = 0 ;i<5;i++)
  {
    vTaskDelay( 1000 / portTICK_PERIOD_MS );
    if(enter_cf)
      break;
  }
//  if(digitalRead(PIN_interupt)==LOW)
//  {
//    for(int i = 0 ; i< 10;i++)
//    {
//      vTaskDelay( 1000 / portTICK_PERIOD_MS );
//      if(enter_cf)
//        break;
//    }
//  }
  
  if (!in_cf_mode && enter_cf)
  {
    Serial.println("pAdvertising->stop();");
    pAdvertising->stop();
    digitalWrite(PIN_led, 0);
    enter_cf = 0;
    in_cf_mode = 1;
    delay(200);
    display_menu_ibeacon_setting();
    delay(200);
    init_gatt();
    pAdvertising->setAdvertisementType(ADV_TYPE_IND);
    gattService->start();
    BLEAdvertising *pAdvertising_gap = gattServer->getAdvertising();
    pAdvertising_gap->start();
  }
  else if (in_cf_mode && enter_cf)
  {
    Serial.println("config success");
    gattService->stop();
    BLEAdvertising *pAdvertising_gap = gattServer->getAdvertising();
    pAdvertising_gap->stop();
    store_ibeacon_cf_to_eeprom(ibeacon_cf);
    setBeacon();
    
    // store ibeacon cf to eeprom
    Serial.println("pAdvertising->start();");
    pAdvertising->start();
    digitalWrite(PIN_led, 1);
    in_cf_mode = 0;
    enter_cf = 0;
  }
  if (!in_cf_mode && !enter_cf)
  {
    
    pAdvertising->stop();
//    Serial.printf("enter light sleep\n");
    esp_deep_sleep(5e6);
//    esp_sleep_enable_timer_wakeup(5000*1000);
//    esp_light_sleep_start();
    pAdvertising->start();
  }

}
