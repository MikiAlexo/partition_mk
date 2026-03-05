#include <Arduino.h>
#include "partitions_mk.h"
#include "data_structure.h"


partition_mk storage;  
partition_mk errLog;


int packetId = 0;

void menu(){
    Serial.println("\n choose operation");
    Serial.println("[w] Append new data");
    Serial.println("[r] Full dump");
    Serial.println("[s] Erase Sector 0 only");
    Serial.println("[f] Formate");
    Serial.println("[o] Overwrite at offset 0 address");
    Serial.println("[a] read wifi credentials");
    Serial.println("[b] read target info");
    Serial.println("[e] read device id");
    Serial.println("[c] change wifi credentials");
    Serial.println("[d] change target info");
    Serial.println("[g] change device id");
    Serial.println("[i] Info");
    // Serial.print("Current Pointer: ");
    // Serial.println(storage.get_pointer());
}

void setup() {
  bool err = false;
    Serial.begin(115200);
     partition_mk::init_NVS();

    Serial.println("initializing partitions...");

    if (!errLog.begin("errorlog")) {
        Serial.println("failed to mount errorlog");
        err = true;
        for(;;);
    }

    if (!storage.begin("storage")) {
        Serial.println("failed mount storage. error code: 0x01");
        (err? :errLog.append_data(&log_codes[STOR_INI_ERR],1));
        for(;;);
    }
    
    Serial.println("initialization complete, press any key to continue");

}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        

        while(Serial.available()) Serial.read(); 

        switch (cmd) {
            
            case 'w': {
                SensorData data;
                data.id = packetId++;
                data.timestamp = millis();
                data.temperature = random(600, 10000) / 100.0;
                data.humidity = random(4000, 8000) / 100.0;


                if (storage.append_data(&data, sizeof(SensorData))) {
                    Serial.printf("appended packet #%d at offset %d\n", data.id, storage.get_pointer() - sizeof(SensorData));
                } else {
                    Serial.printf("append failed, partition full? %d \n", storage.get_pointer());
                    errLog.append_data(&log_codes[APPEND_ERR], 1);
                }
                Serial.println("operation complete");
                break;
            }

            case 'r': {
                Serial.println("\n READING CONTENT...");
                
                int readPos = 0; 
                int endPos = storage.get_pointer();
                
                SensorData buffer;

                while (readPos < endPos) {
  
                    if (storage.read_data(readPos, &buffer, sizeof(SensorData))) {
                        
                        if (buffer.id == 0xFFFFFFFF) {
                            Serial.println("[found end parity]");
                            break; 
                        }

                        Serial.printf("[%d] ID: %d | Time: %u | Temp: %.2f | Hum: %.2f\n", 
                                      readPos, buffer.id, buffer.timestamp, buffer.temperature, buffer.humidity);
                        
                        readPos += sizeof(SensorData);
                    } else {
                        Serial.println("read error");
                        errLog.append_data(&log_codes[READ_ERR],1);
                        break;
                    }
                }
                Serial.println("operation complete");
                break;
            }

            case 's': {
                Serial.println("erasing Sector 0 (0 - 4096 bytes)...");
                if (storage.erase_sector(0)) {
                    Serial.println("sector 0 erased.");
                } else {
                    Serial.println("erase failed");
                    errLog.append_data(&log_codes[ERASE_ERR],1);
                }
                Serial.println("operation complete");
                break;
            }

            case 'f': {
                Serial.println("formatting entire partition (This takes time)...");
                if (storage.erase_full_partition()) {
                    Serial.printf("partition Wiped. pointer reset to 0. %d", storage.get_pointer());
                    packetId = 0; 
                } else {
                    Serial.println("format failed");
                    errLog.append_data(&log_codes[ERASE_ERR],1);
                }
                Serial.println("operation complete");
                break;
            }

            case 'o': {
                Serial.println("overwriting address 0 manually...");
                storage.erase_sector(0);

                SensorData manualData = {9999, millis(), 99.99, 99.99};
                storage.write_data(0, &manualData, sizeof(SensorData));
                Serial.println("overwrite complete");
                Serial.println("operation complete");
                break;
            }
            case 'a':{
                char* ssid;
                char *pass;
                if(partition_mk::read_wificredentials_to(ssid,pass)){
                    Serial.print("SSID:");
                    Serial.println(ssid);
                    Serial.print("Password:");
                    Serial.println(pass);
                } 
                else{
                    Serial.println("failed to read credentuals");
                    errLog.append_data(&log_codes[NVS_READ_ERR], 1); 
                }
                Serial.println("operation complete");
                break;
            }
            case 'e':{
                 Serial.println("Loading device ID...");
                 char* device_id;
                if(partition_mk::read_device_id_to(device_id)){
                     Serial.printf("found device with ID %s", device_id);
                }
                else{
                     Serial.println("failed to load ID");
                     errLog.append_data(&log_codes[NVS_READ_ERR], 1);
                }
                Serial.println("operation complete");
                break;
            }
            case 'c': {
                char ssid[33];  
                char password[65];
                Serial.println("enter new wifi SSID:");
                while (!Serial.available()) {}
                Serial.readBytesUntil('\n', ssid, sizeof(ssid));
                ssid[strcspn(ssid, "\r")] = 0;

                Serial.println("enter new wifi password:");
                while (!Serial.available()) {}
                Serial.readBytesUntil('\n', password, sizeof(password));
                password[strcspn(password, "\r")] = 0;

                Serial.printf("SSID: %s , Password: %s\n", ssid, password);

                if(partition_mk::change_wificredentials_to(ssid, password)){
                    Serial.printf("changed ssid and pass to: %s, %s", ssid, password);
                }
                else{
                    Serial.println("failed to change wifi credentials");
                    errLog.append_data(&log_codes[NVS_WRITE_ERR], 1);
                }
                Serial.println("operation complete");
                break;
            }

            case 'd':{
                char farm_id[8];
                char cow_id[9];
                Serial.println("enter new farm Id: ");
                while(!Serial.available());
                Serial.readBytesUntil('\n',farm_id, sizeof(farm_id));
                farm_id[strcspn(farm_id,"\r")] = 0;

                Serial.println("enter new cow Id: ");
                while(!Serial.available());
                Serial.readBytesUntil('\n', cow_id, sizeof(cow_id));
                cow_id[strcspn(cow_id,"\r")] = 0;

                Serial.printf("registering with farm Id: %s, cow Id %s", farm_id, cow_id);

                if(partition_mk::change_target_to(farm_id, cow_id)){
                    Serial.println("successfully changed target id");
                }
                else{
                    Serial.println("failed to change target id");
                    errLog.append_data(&log_codes[NVS_WRITE_ERR], 1);
                }
                Serial.println("operation complete");
                break;

            }
            case 'g': {
                    char device_id[10];
                    Serial.println("enter new dvice Id");
                    while(!Serial.available());
                    Serial.readBytesUntil('\n', device_id,sizeof(device_id));
                    device_id[strcspn(device_id,"\r")] = 0;
                     
                    if(partition_mk::change_device_id_to(device_id))
                    Serial.printf("chnaged device id to #%s", device_id);
                else{
                    Serial.println("failed to chang device id");
                    errLog.append_data(&log_codes[NVS_WRITE_ERR], 1);
                }
                Serial.println("operation complete");
                break;
            }

            case 'i': {
                Serial.printf("sotrage partition size: %u bytes\n", storage.get_size());
                Serial.printf("error_log partition size: %u bytes\n", errLog.get_size());
                Serial.printf("storage partition current write pointer: %d\n", storage.get_pointer());
                Serial.printf("error_log partition current write pointer: %d\n", errLog.get_pointer());
                Serial.printf("configuration namespace free entries: %d\n", partition_mk::config_pref.freeEntries());
                Serial.printf("info namespace free entries: %d\n", partition_mk::info_pref.freeEntries());
                Serial.printf("cow average namespace free entries: %d\n", partition_mk::cowavg_pref.freeEntries());
                Serial.println("operation complete");
                break;
            }
            
            default:
                menu();
                break;
        }
    }
}