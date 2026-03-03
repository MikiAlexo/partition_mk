#include <Arduino.h>
#include "partitions_mk.h"
#include "data_structure.h"




partition_mk storage;  
partition_mk errLog;


int packetId = 0;


void printMenu() {
    Serial.println("\n __choose operation__");
    Serial.println("[w] Append new data");
    Serial.println("[r] Full dump");
    Serial.println("[s] Erase Sector 0 only");
    Serial.println("[f] Formate");
    Serial.println("[o] Overwrite at offset 0 address");
    Serial.println("[i] Info");
    // Serial.print("Current Pointer: ");
    // Serial.println(storage.get_pointer());
}

void setup() {
  bool err = false;
    Serial.begin(115200);
    partition_mk::init();

    Serial.println("Initializing Partitions...");

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
    
    

    Serial.println("Initialization Complete.");
    printMenu();
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        

        while(Serial.available()) Serial.read(); 

        switch (cmd) {
            
            case 'w': {
                SensorData myData;
                myData.id = packetId++;
                myData.timestamp = millis();
                myData.temperature = random(2000, 3000) / 100.0;
                myData.humidity = random(4000, 8000) / 100.0;


                if (storage.append_data(&myData, sizeof(SensorData))) {
                    Serial.printf("Appended Packet #%d at offset %d\n", myData.id, storage.get_pointer() - sizeof(SensorData));
                } else {
                    Serial.println("Append Failed, Partition full?");
                    errLog.append_data(&log_codes[APPEND_ERR], 1);
                }
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
                            Serial.println("[Empty Flash Encountered]");
                            break; 
                        }

                        Serial.printf("[%d] ID: %d | Time: %u | Temp: %.2f | Hum: %.2f\n", 
                                      readPos, buffer.id, buffer.timestamp, buffer.temperature, buffer.humidity);
                        
                        readPos += sizeof(SensorData);
                    } else {
                        Serial.println("Read Error");
                        errLog.append_data(&log_codes[READ_ERR],1);
                        break;
                    }
                }
                Serial.println("--- END OF READ ---");
                break;
            }

            case 's': {
                Serial.println("Erasing Sector 0 (0 - 4096 bytes)...");
                if (storage.erase_sector(0)) {
                    Serial.println("Sector 0 Erased.");
                } else {
                    Serial.println("Erase Failed");
                    errLog.append_data(&log_codes[ERASE_ERR],1);
                }
                break;
            }

            case 'f': {
                Serial.println("Formatting entire partition (This takes time)...");
                if (storage.erase_full_partition()) {
                    Serial.println("Partition Wiped. Pointer reset to 0.");
                    packetId = 0; 
                } else {
                    Serial.println("Format Failed");
                    errLog.append_data(&log_codes[ERASE_ERR],1);
                }
                break;
            }

            case 'o': {
                Serial.println("Overwriting address 0 manually...");
                storage.erase_sector(0);

                SensorData manualData = {9999, millis(), 99.99, 99.99};
                storage.write_data(0, &manualData, sizeof(SensorData));
                Serial.println("Overwrite complete.");
                break;
            }

            case 'i': {
                Serial.printf("Partition Size: %u bytes\n", storage.get_size());
                Serial.printf("Current Write Pointer: %d\n", storage.get_pointer());
                break;
            }
            
            default:
                printMenu();
                break;
        }
    }
}