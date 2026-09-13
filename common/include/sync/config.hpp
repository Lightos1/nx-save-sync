#pragma once

constexpr const char *ConfigPath    = "/config/nx-save-sync/";
constexpr const char *ConfigFile    = "/config/nx-save-sync/config.ini";
constexpr const char *ConfigFileTmp = "/config/nx-save-sync/tmp.ini";
constexpr const char *FileLogPath   = "/config/nx-save-sync/log.txt";

enum ConfigValue {
    ConfigValue_PeerIp = 0,
    ConfigValue_PeerPort,
    ConfigValue_ListenPort,
};

const char *GetConfigName(ConfigValue value);

void SetConfigValue(ConfigValue configValue, u64 value);
u64 GetConfigValue(ConfigValue configValue);

bool SetConfigValueStr(ConfigValue value, const char *str);
bool GetConfigValueStr(ConfigValue value, char *buffer, u32 bufferSize, const char *defaultValue);
