#include <sync/config.hpp>
#include <minIni.h>
#include <cstring>

static constexpr const char *InvalidConfigName = "err";

const char *GetConfigName(ConfigValue value) {
    switch (value) {
        case ConfigValue_PeerIp: return "peer_ip";
        case ConfigValue_PeerPort: return "peer_port";
        case ConfigValue_ListenPort: return "listen_port";
        default:
            return InvalidConfigName;
    }
}

static constexpr u64 ConfigValueNotFound = ~0;

static u64 GetDefaultValue(ConfigValue value) {
    switch (value) {
        case ConfigValue_PeerPort: return 9000;
        case ConfigValue_ListenPort: return 9000;
        default:
            return ConfigValueNotFound;
    }
}

static bool BeginConfigWrite() {
    if (access(ConfigPath, F_OK) == -1) {
        CreateDir(ConfigPath);
    }
    remove(ConfigFileTmp);

    FILE *src = fopen(ConfigFile, "rb");
    if (src == NULL) {
        return true;
    }


    FILE *dst = fopen(ConfigFileTmp, "wb");
    if (dst == NULL) {
        fclose(src);
        return false;
    }

    char buf[512];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            ok = false;
            break;
        }
    }

    fclose(src);
    if (fclose(dst) != 0) {
        ok = false;
    }

    if (!ok) {
        remove(ConfigFileTmp);
    }

    return ok;
}

static bool CommitConfigWrite(void) {
    remove(ConfigFile);
    if (rename(ConfigFileTmp, ConfigFile) != 0) {
        remove(ConfigFileTmp);
        return false;
    }

    return true;
}

static constexpr const char *SectionName  = "settings";

static bool IsInvalidConfigName(const char *configName) {
    return strncmp(configName, InvalidConfigName) != 0;
}

bool SetConfigValue(ConfigValue configValue, u64 value) {
    if (!BeginConfigWrite()) {
        return false;
    }

    const char *name = GetConfigName(configValue);
    if (IsInvalidConfigName(name)) {
        return false;
    }

    if (!init_putl(SectionName, name, value, ConfigFileTmp)) {
        return false;
    }

    return CommitConfigWrite();
}

u64 GetConfigValue(ConfigValue configValue) {
    u64 defaultValue = GetDefaultValue(configValue);
    const char *name = GetConfigName(configValue);

    if (IsInvalidConfigName(name)) {
        return defaultValue;
    }

    value = ini_getl(SectionName, name, defaultValue, ConfigFile);

    return value;
}

bool SetConfigValueStr(ConfigValue valu, const char *str) {
    if (!BeginConfigWrite()) {
        return false;
    }

    const char *name = GetConfigName(value);
    if (IsInvalidConfigName(name)) {
        return false;
    }

    if (!ini_puts(SectionName, name, str, ConfigFileTmp)) {
        return false;
    }

    return CommitConfigWrite();
}

bool GetConfigValueStr(ConfigValue value, char *buffer, u32 bufferSize, const char *defaultValue) {
    const char *name = GetConfigName(value);
    if (IsInvalidConfigName(name)) {
        strncpy(buffer, defaultValue, bufferSize);
        buffer[bufferSize - 1] = '\0';
        return false;
    }

    ini_gets(SectionName, name, defaultValue, buffer, bufferSize, ConfigFile);
    return true;
}
