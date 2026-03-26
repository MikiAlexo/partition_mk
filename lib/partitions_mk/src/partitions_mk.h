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
    
public:
    static Preferences config_pref;// make these objects private after testing is done
    static Preferences info_pref;
    static Preferences cowavg_pref;

    partition_mk(void);

    bool begin(const char* partitionName);

    bool read_data(int offset, void* buffer, size_t size);
    
    bool write_data(uint32_t offset, const void* data, size_t size);
    bool append_data(const void* data, size_t size);

    bool erase_sector(int offset);
    bool erase_full_partition(void);

    size_t get_size(void);

    int get_pointer(void) { return current_pointer; }
    void set_pointer(int offset) { current_pointer = offset; }

    int check_remaining_chunk(const void *data) { return (get_size()-current_pointer)/sizeof(data); }

    static bool change_wificredentials_to(const char* ssid, const char* password);
    static bool change_device_id_to(char* device_id); //g
    static bool change_target_to(char* farm_id, char* cow_id); //d
    static bool change_cowAvg_to();
    static bool write_pointer(const char* partition_name, int ptr);

    static bool read_wificredentials_to(char*ssid, char* password);
    static bool read_device_id_to(char* device_id);    //e
    static bool read_target_to(char* farm_id, char* cow_id); //b
    static bool read_cowAvg_to();
    static int read_pointer(const char* partition_name);
    static void init_NVS(void);
    static void end_NVS(void);

  // if enough time remains implement the satic NVS methods inside of a new sub class    

    /*write example test code   {saving wifi credentials, connecting to wifi by loading from NVS,
                                saving sensor data in sotrage partition, loading stored sensor data,
                                creating intentional errors to verify if the error logging system is working } 
    */
};

#endif