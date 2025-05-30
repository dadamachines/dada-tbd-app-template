#pragma once

#include <string>
#include <cstdint>

typedef enum{
    GetPlugins = 0x01, // returns json cstring with all plugins
    GetActivePlugin = 0x02, // returns cstring with active plugin
    GetActivePluginParams = 0x03, // returns json cstring with all params of active plugin
    SetActivePlugin = 0x04, // sets active plugin, args [channel (uint8_t), pluginID (cstring)]
    SetPluginParam = 0x05, // sets a plugin parameter, args [channel (uint8_t), paramID (cstring), value (int32_t)]
    SetPluginParamCV = 0x06,
    // sets a plugin parameter for CV, args [channel (uint8_t), paramID (cstring), value (int8_t)]
    SetPluginParamTRIG = 0x07,
    // sets a plugin parameter for TRIG, args [channel (uint8_t), paramID (cstring), value (int8_t)]
    GetPresets = 0x08, // returns json cstring with all presets for a channel, args [channel (uint8_t)]
    GetPresetData = 0x09, // returns json cstring with all preset data for a plugin, args [pluginID (cstring)]
    SetPresetData = 0x0A, // sets a preset data for a plugin, args [pluginID (cstring), presetData (json cstring)]
    LoadPreset = 0x0B, // loads a preset for a channel, args [channel (uint8_t), presetID (int8_t)]
    SavePreset = 0x0C,
    // saves a preset for a channel, args [channel (uint8_t), presetID (int8_t), presetName (cstring)]
    GetAllFavorites = 0x0D, // returns json cstring with all favorites
    SaveFavorite = 0x0E, // saves a favorite, args [favoriteID (int8_t), json cstring with favorite data]
    LoadFavorite = 0x0F, // loads a favorite, args [favoriteID (int8_t)]
    GetConfiguration = 0x10, // returns json cstring with current configuration
    SetConfiguration = 0x11, // sets the configuration, args [json cstring with configuration data]
    GetIOCapabilities = 0x12, // returns json cstring with IO capabilities
    Reboot = 0x13, // reboots the device
} RequestType_t;

class SpiAPI{
    bool receiveData(std::string& response, RequestType_t requestType);
    void transmitData(const std::string &data, const RequestType_t reqType);
    void send();

public:
    void Init();
    bool GetPlugins(std::string& response); // true on success, response contains JSON string of plugins
    // true on success, response contains name of active plugin
    bool GetActivePlugin(const uint8_t channel, std::string& response);
    // true on success, response contains JSON string of active plugin parameters
    bool GetActivePluginParams(const uint8_t channel, std::string& response);
    // true on success, sets active plugin for the given channel
    bool SetActivePlugin(const uint8_t channel, const std::string& pluginID);
    bool SetActivePluginParam(const uint8_t channel, const std::string& paramName, const int32_t value);
    bool SetActivePluginCV(const uint8_t channel, const std::string& paramName, const int32_t value);
    bool SetActivePluginTrig(const uint8_t channel, const std::string& paramName, const int32_t value);
    bool GetPresets(const uint8_t channel, std::string& response);
    bool GetPresetData(const std::string& pluginID, std::string& response);
    bool SetPresetData(const std::string& pluginID, const std::string& data);
    bool LoadPreset(const uint8_t channel, const int8_t presetID);
    bool SavePreset(const uint8_t channel, const std::string &presetName, const int8_t presetID);
    bool GetAllFavorites(std::string& response);
    bool LoadFavorite(const int8_t favoriteID);
    bool SaveFavorite(const std::string& favoriteData);
    bool GetConfiguration(std::string& response);
    bool SetConfiguration(const std::string& configData);

};
