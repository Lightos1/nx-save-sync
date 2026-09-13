#pragma once

namespace fs {

    Result IterateSavefile(AccountUid &account, u64 programId);
    void Log(const char *fmt, ...);

}
