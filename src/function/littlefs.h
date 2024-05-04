#ifndef LITTLEFS_FUNCTIONS
#define LITTLEFS_FUNCTIONS

#include "config/enums.h"

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <ArduinoJson.h> // ArduinoJson : https://github.com/bblanchon/ArduinoJson
#include "function/ha.h"

#include "uptime.h"

#if defined(ESP32) || defined(ESP32ETH)
  #include <FS.h>
  #include "SPIFFS.h"
  #define LittleFS SPIFFS // Fonctionne, mais est-ce correct? 
#else
  #include <LittleFS.h>
#endif

constexpr const char *mqtt_conf = "/mqtt.json";
constexpr const char *filename_conf = "/config.json";
constexpr const char *wifi_conf = "/wifi.json";
constexpr const char *programme_conf = "/programme.json";


extern Config config; 

extern Logs logging; 

extern String node_id; 
extern Wifi_struct  wifi_config_fixe; 

//flag for saving data
extern bool shouldSaveConfig ;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

String loguptime(String log) {
  String uptime_stamp;
  uptime::calculateUptime();
  uptime_stamp = String(uptime::getDays())+":"+String(uptime::getHours())+":"+String(uptime::getMinutes())+":"+String(uptime::getSeconds())+ "\t"+log +"\r\n";
  return uptime_stamp;
}

// Loads the configuration from a file
void loadConfiguration(const char *filename, Config &config) {
  // Open file for reading
  File configFile = LittleFS.open(filename_conf, "r");

   // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  DynamicJsonDocument doc(2048);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println(F("Failed to read configuration file, using default configuration"));
    logging.Set_log_init("Failed to read file config File, use default\r\n" ); 
    }
  // Copy values from the JsonDocument to the Config
  
  strlcpy(config.hostname,                  // <- destination
          doc["hostname"] | "192.168.1.22", // <- source
          sizeof(config.hostname));         // <- destination's capacity
  config.port = doc["port"] | 1883;
  strlcpy(config.Publish,                 
          doc["Publish"] | "domoticz/in", 
          sizeof(config.Publish));        
  config.IDXTemp = doc["IDXTemp"] | 200; 
  config.maxtemp = doc["maxtemp"] | 60; 
  config.IDXAlarme = doc["IDXAlarme"] | 202; 
  config.IDX = doc["IDX"] | 201; 
  config.startingpow = doc["startingpow"] | 0; 
  config.minpow = doc["minpow"] | 5;
  config.maxpow = doc["maxpow"] | 50; 
  config.charge = doc["charge"] | 1000; 
  config.charge1 = doc["charge1"] | 1000; 
  config.charge2 = doc["charge2"] | 0; 
  config.charge3 = doc["charge3"] | 0; 
  strlcpy(config.child,                  
          doc["child"] | "", 
          sizeof(config.child));         
  strlcpy(config.mode,                  
          doc["mode"] | "off", 
          sizeof(config.mode));
  strlcpy(config.SubscribePV,                 
        doc["SubscribePV"] | "none", 
        sizeof(config.SubscribePV));    
  strlcpy(config.SubscribeTEMP,                 
        doc["SubscribeTEMP"] | "none", 
        sizeof(config.SubscribeTEMP));
  config.dimmer_on_off = doc["dimmer_on_off"] | 1; 
  config.HA = doc["HA"] | true; 
  config.JEEDOM = doc["JEEDOM"] | true; 
  config.DOMOTICZ = doc["DOMOTICZ"] | true; 
  strlcpy(config.PVROUTER,
        doc["PVROUTER"] | "mqtt", 
        sizeof(config.PVROUTER)); 
  strlcpy(config.DALLAS,
        doc["DALLAS"] | "28b1255704e13c62", 
        sizeof(config.DALLAS));   
  strlcpy(config.say_my_name,                 
        doc["name"] | "", 
        sizeof(config.say_my_name));

  configFile.close();
  
      
}

//***********************************
//************* Gestion de la configuration - sauvegarde du fichier de configuration
//***********************************

void saveConfiguration(const char *filename, const Config &config) {
  
  // Open file for writing
   File configFile = LittleFS.open(filename_conf, "w");
  if (!configFile) {
    Serial.println(F("Failed to open config file for writing"));
    logging.Set_log_init("Failed to read file config File, use default\r\n"); 
  
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(2048);

  // Set the values in the document
  doc["hostname"] = config.hostname;
  doc["port"] = config.port;
  doc["Publish"] = config.Publish;
  doc["IDXTemp"] = config.IDXTemp;
  doc["maxtemp"] = config.maxtemp;
  doc["IDXAlarme"] = config.IDXAlarme;
  doc["IDX"] = config.IDX;  
  doc["startingpow"] = config.startingpow;
  doc["minpow"] = config.minpow;
  doc["maxpow"] = config.maxpow;
  doc["child"] = config.child;
  doc["mode"] = config.mode;
  doc["SubscribePV"] = config.SubscribePV;
  doc["SubscribeTEMP"] = config.SubscribeTEMP;
  doc["dimmer_on_off"] = config.dimmer_on_off;
  doc["charge"] = config.charge;
  doc["HA"] = config.HA;
  doc["JEEDOM"] = config.JEEDOM;
  doc["DOMOTICZ"] = config.DOMOTICZ;
  doc["PVROUTER"] = config.PVROUTER;
  doc["DALLAS"] = config.DALLAS;
  doc["name"] = config.say_my_name;
  doc["charge1"] = config.charge1;
  doc["charge2"] = config.charge2;
  doc["charge3"] = config.charge3;

  // Serialize JSON to file
  if (serializeJson(doc, configFile) == 0) {
    Serial.println(F("Failed to write to file"));
  }
  
  /// Publish on MQTT 
  char buffer[1024];// NOSONAR
  serializeJson(doc, buffer);
  client.publish(("Xlyric/sauvegarde/"+ node_id).c_str() ,0,true, buffer);
  
  // Close the file
  configFile.close();
}

//***********************************
//************* Gestion de la configuration - Lecture du fichier de configuration
//***********************************

//***********************************
//************* Gestion de la configuration - Lecture du fichier de configuration
//***********************************

bool loadmqtt(const char *filename, Mqtt &mqtt_config) {
  // Open file for reading
  File configFile = LittleFS.open(mqtt_conf, "r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  DynamicJsonDocument doc(512);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println(F("Failed to read MQTT config "));
    return false;
  }

  
  // Copy values from the JsonDocument to the Config
  
  strlcpy(mqtt_config.username,                  // <- destination
          doc["MQTT_USER"] | "", // <- source
          sizeof(mqtt_config.username));         // <- destination's capacity
  
  strlcpy(mqtt_config.password,                  // <- destination
          doc["MQTT_PASSWORD"] | "", // <- source
          sizeof(mqtt_config.password));         // <- destination's capacity
  mqtt_config.mqtt = doc["mqtt"] | true;

  configFile.close();

return true;    
}

void savemqtt(const char *filename, const Mqtt &mqtt_config) {
  
  // Open file for writing
   File configFile = LittleFS.open(mqtt_conf, "w");
  if (!configFile) {
    Serial.println(F("Failed to open config file for writing in function Save configuration"));
    return;
  } 

    // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(512);

  // Set the values in the document
  doc["MQTT_USER"] = mqtt_config.username;
  doc["MQTT_PASSWORD"] = mqtt_config.password;
  doc["mqtt"] = mqtt_config.mqtt;
  if (serializeJson(doc, configFile) == 0) {
    Serial.println(F("Failed to write to file in function Save configuration "));
    logging.Set_log_init("Failed to write MQTT config\r\n");
  }

  // Close the file
  configFile.close();
  config.restart = true;
}


//////////////wifi ip fixe 

bool loadwifiIP(const char *wifi_conf, Wifi_struct &wifi_config_fixe) {
  // Open file for reading
  File configFile = LittleFS.open(wifi_conf, "r");

      // Allocate a temporary JsonDocument
      // Don't forget to change the capacity to match your requirements.
      // Use arduinojson.org/v6/assistant to compute the capacity.
      DynamicJsonDocument doc(256);

      // Deserialize the JSON document
      DeserializationError error = deserializeJson(doc, configFile);
      if (error) {
        Serial.println(F("Failed to read wifi config"));
      }

      
      // Copy values from the JsonDocument to the Config
      
      strlcpy(wifi_config_fixe.static_ip,                  // <- destination
              doc["IP"] | "", // <- source
              sizeof(wifi_config_fixe.static_ip));         // <- destination's capacity
      
      strlcpy(wifi_config_fixe.static_sn,                  // <- destination
              doc["mask"] | "255.255.255.0", // <- source
              sizeof(wifi_config_fixe.static_sn));         // <- destination's capacity

      strlcpy(wifi_config_fixe.static_gw,                  // <- destination
              doc["gateway"] | "192.168.1.254", // <- source
              sizeof(wifi_config_fixe.static_gw));         // <- destination's capacity

      configFile.close();

return true;    
}

void savewifiIP(const char *wifi_conf, Wifi_struct &wifi_config_fixe) {
  
  // Open file for writing
   File configFile = LittleFS.open(wifi_conf, "w");
  if (!configFile) {
    Serial.println(F("Failed to open config file for writing in function Save configuration"));
    return;
  } 

    // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(256);

  // Set the values in the document
  doc["IP"] = wifi_config_fixe.static_ip;
  doc["mask"] = wifi_config_fixe.static_sn;
  doc["gateway"] = wifi_config_fixe.static_gw;

  // Serialize JSON to file
  if (serializeJson(doc, configFile) == 0) {
    Serial.println(F("Failed to write to file in function Save configuration "));
    logging.Set_log_init("Failed to write wifi config\r\n");
  }

  // Close the file
  configFile.close();
}


bool test_fs_version() {

  // Open file for reading

  File file = LittleFS.open("/version", "r");
  if (!file) {
    logging.Set_log_init("FS version is missing please update\r\n");
    return false;
  }
   // comparaison entre le contenu du fichier et la version du code FS_RELEASE
  String version = file.readStringUntil('\n');
  file.close(); 
  if (version.toInt() < String(FS_RELEASE).toInt() )  {
    logging.Set_log_init("FS version is not the same as code version please update FS\r\n");
    return false;
  }
  logging.Set_log_init("FS version Ok\r\n");
  return true;
}

#endif