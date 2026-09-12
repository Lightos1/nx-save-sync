#pragma once

namespace fs {

    constexpr const char *ConfigPath  = "/config/nx-save-sync/";
    constexpr const char *FileLogPath = "/config/nx-save-sync/log.txt";

    Result IterateSavefile(AccountUid &account, u64 programId);
    void Log(const char *fmt, ...);

}
