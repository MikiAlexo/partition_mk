#ifndef PARTITIONS_MK_H
#define PARTITIONS_MK_H

#include <Arduino.h>
#include "esp_partition.h"
#include "esp_log.h"
#include <Preferences.h>
// #include "data_structure.h" 
// one chunk = one structure

   
class partition_mk {
private:
    const esp_partition_t* _part_handle = NULL;
    const char* _name = NULL;
    int current_pointer = 0;
    char ssid[16] = "blynk";
    char password[9] = "12345678";
    static Preferences preferences;
public:
    partition_mk();

    bool begin(const char* partitionName);

    bool read_data(int offset, void* buffer, size_t size);
    
    bool write_data(int offset, const void* data, size_t size);
    bool append_data(const void* data, size_t size);

    bool erase_sector(int offset);
    bool erase_full_partition();

    size_t get_size();

    int get_pointer() { return current_pointer; }
    void set_pointer(int offset) { current_pointer = offset; }

    int check_remaining_chunk(const void *data) { return (get_size()-current_pointer)/sizeof(data); }

    bool change_wificredentials_to(const char* ssid, const char* password);
    bool change_device_id_to(char* device_id);
    bool change_target_to(char* farm_id, char* cow_id);
    bool change_cowAvg_to();

    bool read_wificredentials_to(char*ssid, char* password);
    bool read_device_id_to(char* device_id);
    bool read_target_to(char* farm_id, char* cow_id);
    bool read_cowAvg_to(float* cow_avg);
    static void init();
    
};

#endif