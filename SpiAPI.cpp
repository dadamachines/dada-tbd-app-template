#include "SpiAPI.h"

#include <SPI.h>
// defines for spi0, RP2350 SPI stream to P4 for rest-api
#define SPI_SPEED 30000000 // 30MHz seems to still work for receiving from p4, sending works up to 62.5MHz
#define SPI_SCLK 34
#define SPI_MOSI 35
#define SPI_MISO 32
#define SPI_CS 33

static SPISettings spiSettings(SPI_SPEED, MSBFIRST, SPI_MODE3);

static uint8_t out_buf[2048], in_buf[2048];
// params
uint8_t * const request_type = &out_buf[2]; // request type
uint8_t * const uint8_param_0 = &out_buf[3]; // first request parameter, e.g. channel, favorite number, ...
uint8_t * const uint8_param_1 = &out_buf[4];; // second request parameter, e.g. preset number, ...
int32_t * const int32_param_2 = (int32_t*)&out_buf[5]; // third request parameter, e.g. value, ...
uint8_t* const string_param_3 = (uint8_t*)&out_buf[9]; // fourth request parameter, e.g. plugin name, parameter name, ...


void SpiAPI::Init(){
    // p4 rp2350 SpiAPI stream init
    SPI.setMISO(SPI_MISO);
    SPI.setMOSI(SPI_MOSI);
    SPI.setCS(SPI_CS);
    SPI.setSCK(SPI_SCLK);
    SPI.begin(true); // hw CS assertion

    out_buf[0] = 0xCA;
    out_buf[1] = 0xFE; // fingerprint
    delay(100); // wait for the SPI bus to stabilize
}

bool SpiAPI::receiveData(std::string& response, const RequestType_t request){
    SPI.beginTransaction(spiSettings);
    SPI.transferAsync(out_buf, in_buf, 2048);
    while (!SPI.finishedAsync());
    SPI.endTransaction();
    delay(1);

    // fingerprint check
    if (in_buf[0] != 0xCA || in_buf[1] != 0xFE){
        response = "FP wrong: " + std::to_string(in_buf[0]) + " " + std::to_string(in_buf[1]);
        return false;
    }

    // check request type acknowledgment
    const uint8_t requestType = in_buf[2];
    if (requestType != request){
        response = "ACK wrong: " + std::to_string(requestType);
        return false;
    }

    // read the response
    const uint32_t* resLength = (uint32_t*)&in_buf[3];
    const uint32_t totalResponseLength = *resLength;
    response.reserve(*resLength); // reserve space for the JSON string
    uint32_t bytes_received = *resLength > 2048 - 7 ? 2048 - 7 : *resLength; // 7 bytes for fingerprint and length
    uint32_t bytes_to_be_received = *resLength - bytes_received;
    response.append((char*)&in_buf[7], bytes_received); // skip the first 7 bytes (fingerprint and length)

    while (bytes_to_be_received > 0){
        SPI.beginTransaction(spiSettings);
        SPI.transferAsync(out_buf, in_buf, 2048);
        while (!SPI.finishedAsync());
        SPI.endTransaction();
        delay(1);

        // fingerprint check
        if (in_buf[0] != 0xCA || in_buf[1] != 0xFE){
            response = "FP wrong: " + std::to_string(in_buf[0]) + " " + std::to_string(in_buf[1]);
            return false;
        }

        // check request type acknowledgment
        const uint8_t requestType = in_buf[2];
        if (requestType != request){
            response = "ACK wrong: " + std::to_string(requestType);
            return false;
        }

        // append the received data to the json string
        bytes_received = *resLength > 2048 - 7 ? 2048 - 7 : *resLength; // 7 bytes for fingerprint and length
        response.append((char*)&in_buf[3], bytes_received);
        bytes_to_be_received -= bytes_received;
    }
    if (response.size() != totalResponseLength){
        response = "LEN error: " + std::to_string(totalResponseLength) + ", got " + std::to_string(response.size());
        return false;
    }
    return true;
}

bool SpiAPI::SetActivePlugin(const uint8_t channel, const std::string& pluginID){
    *request_type = RequestType_t::SetActivePlugin; // request type
    *uint8_param_0 = channel; // channel number
    *int32_param_2 = (uint32_t)pluginID.length(); // length of pluginID
    uint8_t* pluginIDField = string_param_3;
    memcpy(pluginIDField, pluginID.c_str(), pluginID.length() + 1); // copy pluginID to buffer, ensure null-termination
    send();
    delay(2000); // wait for TBD to execute the command

    return true;
}

bool SpiAPI::GetActivePlugin(const uint8_t channel, std::string& response){
    // send GetPlugins request
    response.clear();
    *request_type = RequestType_t::GetActivePlugin; // request type
    *uint8_param_0 = channel; // channel number
    send();

    return receiveData(response, RequestType_t::GetActivePlugin);
}

bool SpiAPI::GetPresets(const uint8_t channel, std::string& response){
    response.clear();
    *request_type = RequestType_t::GetPresets; // request type
    *uint8_param_0 = channel; // channel number
    send();

    return receiveData(response, RequestType_t::GetPresets);
}


bool SpiAPI::GetPresetData(const std::string& pluginID, std::string& response){
    response.clear();
    *request_type = RequestType_t::GetPresetData; // request type
    *int32_param_2 = (uint32_t)pluginID.length(); // length of pluginID
    uint8_t* pluginIDField = string_param_3;
    memcpy(pluginIDField, pluginID.c_str(), pluginID.length() + 1); // copy pluginID to buffer, ensure null-termination
    send();

    return receiveData(response, RequestType_t::GetPresetData);
}

bool SpiAPI::LoadPreset(const uint8_t channel, const int8_t presetID){
    *request_type = RequestType_t::LoadPreset; // request type
    *uint8_param_0 = channel; // channel number
    *uint8_param_1 = presetID; // preset ID
    send();
    delay(1000); // wait for TBD to execute the command

    return true;
}

bool SpiAPI::SavePreset(const uint8_t channel, const std::string & presetName, const int8_t presetID){
    *request_type = RequestType_t::SavePreset; // request type
    *uint8_param_0 = channel; // channel number
    *uint8_param_1 = presetID; // preset ID
    *int32_param_2 = (uint32_t)presetName.length(); // length of presetName
    uint8_t* param_preset_name_field = string_param_3;
    memcpy(param_preset_name_field, presetName.c_str(), presetName.length() + 1);
    send();
    delay(1000); // wait for TBD to execute the command

    return true;
}


void SpiAPI::send(){
    SPI.beginTransaction(spiSettings);
    SPI.transferAsync(out_buf, in_buf, 2048);
    while (!SPI.finishedAsync());
    SPI.endTransaction();
    delay(1);
}

bool SpiAPI::GetActivePluginParams(const uint8_t channel, std::string& response){
    // send GetActivePluginParams request
    response.clear();
    *request_type = RequestType_t::GetActivePluginParams; // request type
    *uint8_param_0 = channel; // channel number
    send();

    return receiveData(response, RequestType_t::GetActivePluginParams);
}

bool SpiAPI::SetActivePluginParam(const uint8_t channel, const std::string& paramName, const int32_t value){
    *request_type = RequestType_t::SetPluginParam; // request type
    *uint8_param_0 = channel; // channel number
    *int32_param_2 = value; // value to set
    uint8_t* param_name_field = string_param_3;
    memcpy(param_name_field, paramName.c_str(), paramName.length() + 1);
    send();
    return true;
}

bool SpiAPI::SetActivePluginCV(const uint8_t channel, const std::string& paramName, const int32_t value){
    *request_type = RequestType_t::SetPluginParamCV; // request type
    *uint8_param_0 = channel; // channel number
    *int32_param_2 = value; // value to set
    uint8_t* param_name_field = string_param_3;
    memcpy(param_name_field, paramName.c_str(), paramName.length() + 1);
    send();
    return true;
}

bool SpiAPI::SetActivePluginTrig(const uint8_t channel, const std::string& paramName, const int32_t value){
    *request_type = RequestType_t::SetPluginParamTRIG; // request type
    *uint8_param_0 = channel; // channel number
    *int32_param_2 = value; // value to set
    uint8_t* param_name_field = string_param_3;
    memcpy(param_name_field, paramName.c_str(), paramName.length() + 1);
    send();
    return true;
}

bool SpiAPI::GetAllFavorites(std::string& response){
    // send GetAllFavorites request
    response.clear();
    *request_type = RequestType_t::GetAllFavorites; // request type
    send();

    return receiveData(response, RequestType_t::GetAllFavorites);
}

bool SpiAPI::LoadFavorite(const int8_t favoriteID){
    // send LoadFavorite request
    *request_type = RequestType_t::LoadFavorite; // request type
    *uint8_param_0 = favoriteID; // favorite ID
    send();
    delay(1000); // wait for TBD to execute the command

    return true;
}

bool SpiAPI::GetConfiguration(std::string& response){
    // send GetConfiguration request
    response.clear();
    *request_type = RequestType_t::GetConfiguration; // request type
    send();

    return receiveData(response, RequestType_t::GetConfiguration);
}

bool SpiAPI::GetPlugins(std::string& response){
    // send GetPlugins request
    response.clear();
    *request_type = RequestType_t::GetPlugins; // request type
    send();

    return receiveData(response, RequestType_t::GetPlugins);
}
