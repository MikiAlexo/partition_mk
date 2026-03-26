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

    // implemented wear-leveling ( shi needs a lot of testing tho);
    current_pointer = 0;
    int ptr = read_pointer(partitionName);

    if (ptr == -1)
        write_pointer(partitionName, 0);

    current_pointer = ptr;

#if DEBUG_MODE
    ESP_LOGI(TAG, "partition head set at offset: %d", current_pointer);
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
/// also update read so that it doesn't load junk(0xFF,0x34) and the order doesn't get fucked cause of circular buffer
/// add retry if it fails the first time
bool partition_mk::write_data(uint32_t offset, const void* data, size_t size) {
    if (!_part_handle) return false;

    if (offset + size > _part_handle->size) {
       
        uint8_t buf;

        if(read_data(0, &buf, 1)){

            if( buf != 0xFF){
                #if DEBUG_MODE
                ESP_LOGE(TAG, "write out of bounds, failed to reset pointer");
                #endif
                return false;
            }
            else{
                #if DEBUG_MODE
                ESP_LOGE(TAG, "write out of bounds, pointer reset to 0");
                #endif
                if(write_pointer(_name, 0)) offset = 0;
                else return false;
            }
        }
        else{
            #if DEBUG_MODE
            ESP_LOGE(TAG, "failed to verify sector availablity, write out of bounds");
            #endif
            return false;
        }
    }
/// checks if sector is empty, if not move to the next sector, erase it and write
/* it doesn't reset to origin if the new offset is >= flash size, could lock the system from writing 
  it should not erase previously stored data, the current ring buffer implementation erases them
  if i want to write 5000 bytes, it only clears 4096 bytes the last 904 bytes will corrupt b/c next sector wasn't erased 
*/
    uint32_t buf;
    if(read_data(offset, &buf, 4)){
        if(buf != 0xFFFFFFFF){
            if(!erase_sector(offset)){
                offset = (offset & ~4095) + 4096;//moves offset to the next sector
                if(offset >= _part_handle->size || offset + size > _part_handle->size){
                    #if DEBUG_MODE
                    ESP_LOGE(TAG, "no more sectors available, write failed");
                    #endif
                    return false;
                }
                erase_sector(offset);// fuck, what if the next sector contains data
            }
           #if DEBUG_MODE
           ESP_LOGI(TAG,"sector erased at offset %d", offset);
           #endif
        }
    }
    if (offset + size < current_pointer){
         #if DEBUG_MODE
            ESP_LOGE(TAG, "can't write backwards[(offset) %d + (size) %d < (pointer) %d]", offset, size, current_pointer);
        #endif
        return false;
    }
    esp_err_t err = esp_partition_write(_part_handle, offset, data, size);//writes to size but only a single sector is erased,bad
    
    if (err != ESP_OK) {
        #if DEBUG_MODE
        ESP_LOGE(TAG, "write failed :( %s", esp_err_to_name(err));
        #endif
        return false;
    }
    else {
            current_pointer = offset + size;
            write_pointer(_name, current_pointer);
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
void partition_mk::init_NVS(){
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

 int partition_mk::read_pointer(const char* partition_name = "storage"){
return  config_pref.getInt(partition_name, -1);
 }

  bool partition_mk::write_pointer( const char* partition_name, int ptr){
    if (config_pref.putInt(partition_name,ptr) > 0){
        #if DEBUG_MODE
        ESP_LOGI(TAG,"pointer at partition %s set to : %d", partition_name, ptr);
        #endif
        return true;
    } 
    else{
        #if DEBUG_MODE
        ESP_LOGE(TAG,"failed to set pointer");
        #endif
        return false;
    }
 }