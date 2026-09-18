#pragma once

namespace fs {

    Result GetSaveDataArchiveTimestamp(AccountUid &account, u64 programId, u64 &outTimestamp);
    Result MountSaveFile(AccountUid &account, u64 programId, FsFileSystem &outFs, std::string &outPath);
    void UnmountSaveFile(FsFileSystem &fs);

}
