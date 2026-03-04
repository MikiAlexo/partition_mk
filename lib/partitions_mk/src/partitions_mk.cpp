#include "partitions_mk.h"

#if DEBUG_MODE
static const char* TAG = "PartMK";
#endif

Preferences partition_mk::config_pref;
Preferences partition_mk::info_pref;
Preferences partition_mk::cowavg_pref;

partition_mk::partition_mk(void) {
    _part_handle = NULL;
}

bool partition_mk::begin(const char* partitionName) {
    _name = partitionName;
    
    _part_handle = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, partitionName);

    if (_part_handle == NULL) {

        #if DEBUG_MODE
        ESP_LOGE(TAG, "partition '%s' not found", partitionName);
        #endif

        return false;
    }

    #if DEBUG_MODE
    ESP_LOGI(TAG, "Mounted '%s' at 0x%x, Size: %d", partitionName, _part_handle->address, _part_handle->size);
    #endif

    /*The  loop below  finds where  previous data ends so it doesn't overwrite it.
     it looks for the first three byte that equals 0xFF.
    */
current_pointer = 0;
const int chunk_size = 256;
uint8_t buffer[chunk_size];

int ff_count = 0;
size_t ff_start = 0;

while (current_pointer < _part_handle->size) {
    int left = _part_handle->size - current_pointer;
    int to_read = (left < chunk_size) ? left : chunk_size;

    esp_partition_read(_part_handle, current_pointer, buffer, to_read);

    for (int i = 0; i < to_read; i++) {
        if (buffer[i] == 0xFF) {
            if (ff_count == 0) {
                ff_start = current_pointer + i;
            }
            ff_count++;

            if (ff_count == 3) {
                current_pointer = ff_start;
                // return; should be break or the loop continues even after finding the end parity (i think, not sure)
                break;
            }
        } else {
            ff_count = 0;
        }
    }

    current_pointer += to_read;
}

#if DEBUG_MODE
    ESP_LOGI(TAG, "rartition head found at offset: %d", current_pointer);
#endif
    return true;
}

bool partition_mk::read_data(int offset, void* buffer, size_t size) {
    if (!_part_handle) return false;

    if (offset + size > _part_handle->size) {
        #if DEBUG_MODE
        ESP_LOGE(TAG, "read out of bounds");
        #endif
        return false;
    }

    esp_err_t err = esp_partition_read(_part_handle, offset, buffer, size);
    if (err != ESP_OK) {
        #if DEBUG_MODE
        ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
        #endif
        return false;
    }
    return true;
}

bool partition_mk::write_data(int offset, const void* data, size_t size) {
    if (!_part_handle) return false;

    if (offset + size > _part_handle->size) {
        #if DEBUG_MODE
        ESP_LOGE(TAG, "write out of bounds");
        #endif
        return false;
    }

    esp_err_t err = esp_partition_write(_part_handle, offset, data, size);
    
    if (err != ESP_OK) {
        #if DEBUG_MODE
        ESP_LOGE(TAG, "write failed :( %s", esp_err_to_name(err));
        #endif
        return false;
    }
    else {

        if (offset + size > current_pointer) {
            current_pointer = offset + size;
        }
        return true;
    }
}


bool partition_mk::append_data(const void* data, size_t size) {
    return write_data(current_pointer, data, size);
}

bool partition_mk::erase_sector(int offset) {
    if (!_part_handle) return false;

    if (offset % 4096 != 0) {
        #if DEBUG_MODE
        ESP_LOGE(TAG, "erase offset %d is not sector aligned", offset);
        #endif
        return false;
    }

    esp_err_t err = esp_partition_erase_range(_part_handle, offset, 4096);
    
    if (err == ESP_OK) {
        
        if (current_pointer >= offset && current_pointer < (offset + 4096)) {
            current_pointer = offset;
            #if DEBUG_MODE
            ESP_LOGI(TAG, "pointer reset to %d because active sector was erased", current_pointer);
            #endif
        }
        return true;
    }
    return false;
}

bool partition_mk::erase_full_partition(void) {
    if (!_part_handle) return false;

    esp_err_t err = esp_partition_erase_range(_part_handle, 0, _part_handle->size);

    if (err == ESP_OK) {
        current_pointer = 0;
        return true;
    }
   #if DEBUG_MODE
    ESP_LOGE(TAG, "full partition erase failed: %s", esp_err_to_name(err));
   #endif    
    return false;
}

size_t partition_mk::get_size(void) {
    if (!_part_handle) return 0;
    return _part_handle->size;
}
void partition_mk::init_NVS(void){
    config_pref.begin("credentials", false);
    info_pref.begin("info", false);
    cowavg_pref.begin("cow-avg", false);

}
void partition_mk::end_NVS(void){
    config_pref.end();
    info_pref.end();
    cowavg_pref.end();
}

bool partition_mk::change_wificredentials_to(const char* ssid, const char* password){
if (config_pref.putString("SSID", ssid) > 0 && config_pref.putString("Password", password) > 0 ){
 #if DEBUG_MODE
    ESP_LOGI(TAG, "wifi credentals changed to : %s-ssid, %s-password", ssid, password);
   #endif    
    return true;
}
else{
 #if DEBUG_MODE
    ESP_LOGE(TAG, "failed to change wifi credentals");
   #endif    
    return false;
}
}

bool partition_mk::change_device_id_to(char* device_id){
if(info_pref.putString("Device ID", device_id)> 0){
    #if DEBUG_MODE
    ESP_LOGI(TAG,"changed Device ID to: %s", device_id);
    #endif
    return true;
}
else{
    #if DEBUG_MODE
    ESP_LOGE(TAG,"failed to change Device ID");
    #endif
    return false;
}

}

 bool partition_mk::change_target_to(char* farm_id, char* cow_id){
 if(info_pref.putString("Farm ID", farm_id) > 0  && info_pref.putString("Cow_ID", cow_id) > 0){
    #if DEBUG_MODE
    ESP_LOGI(TAG,"cnaged farm ID and cow ID to: %s -farm id, %s -cow id", farm_id, cow_id);
    #endif
    return 1;
 }
 else{
    #if DEBUG_MODE
    ESP_LOGE(TAG,"failed to change target id");
    #endif
    return 0;
 }
 }

 bool partition_mk::read_wificredentials_to(char*ssid, char* password){
     String _ssid = config_pref.getString("SSID","CowsVille");
     String _pass = config_pref.getString("Password", "defaiult psassword");
     if(_ssid.length() !=0 && _pass.length() !=0 ){
        strcpy(ssid, _ssid.c_str());
        strcpy(password, _pass.c_str());
        #if DEBUG_MODE
        ESP_LOGI(TAG,"loaded credentials succesfully: %s, %s", ssid, password);
        #endif
        return true;
     }
     else {
        #if DEBUG_MODE
        ESP_LOGE(TAG,"failed to load credentials succesfully");
        #endif
        return false;
     }
 }

 bool partition_mk::read_device_id_to(char* device_id){
    String _device_id = info_pref.getString("Device-ID","xxxxxx");
    if(_device_id.length()!=0){
        strcpy(device_id,_device_id.c_str());
        #if DEBUG_MODE
        ESP_LOGI(TAG,"loaded device ID succesfully: %s", device_id);
        #endif
        return true;
    }else{
        #if DEBUG_MODE
        ESP_LOGI(TAG,"failed to load device id succesfully");
        #endif
        return false;
    }

 }

 bool partition_mk::read_target_to(char* farm_id, char* cow_id){
    String _farm = info_pref.getString("Farm-ID","xxx-xxxx");
    String _cow = info_pref.getString("Cow-ID","xxxx-xxxxxx");

    if(_farm.length()!=0 && _cow.length()!=0){
        strcpy(farm_id,_farm.c_str());
        strcpy(cow_id,_cow.c_str());
   #if DEBUG_MODE
        ESP_LOGI(TAG,"loaded target ID succesfully: %s -farm-ID, %s -cow-ID", farm_id, cow_id);
        #endif
        return true;
    } 
    else{
        #if DEBUG_MODE
        ESP_LOGI(TAG,"failed to load device ID succesfully");
        #endif
        return false;
    }
 }