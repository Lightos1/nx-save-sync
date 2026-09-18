#include <switch.h>
#include <sync.hpp>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <fstream>

#include "../tcp/sys_connection.hpp"
#include "fs.hpp"

namespace fs {

    namespace {
        constexpr const char *BackupMount = "save";
    }

    Result MountSaveFile(AccountUid &account, u64 programId, FsFileSystem &outFs, std::string &outPath) {
        const FsSaveDataAttribute saveAttribute = {
            .application_id = programId,
            .uid            = account,
            .save_data_type = FsSaveDataType_Account,
            .save_data_rank = FsSaveDataRank_Primary,
        };

        R_TRY(fsOpenSaveDataFileSystem(&outFs, FsSaveDataSpaceId_User, &saveAttribute));

        int rc = fsdevMountDevice(BackupMount, outFs);

        if (rc == -1) {
            /* todo proper result */
            return MAKERESULT(Module_Libnx, LibnxError_NotFound);
        }

        outPath = BackupMount;
        outPath += ":/";

        R_SUCCEED();
    }

    Result GetSaveDataArchiveTimestamp(AccountUid &account, u64 programId, u64 &outTimestamp) {
        FsSaveDataInfoReader reader{};
        R_TRY(fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User));
        ON_SCOPE_EXIT { fsSaveDataInfoReaderClose(&reader); };

        FsSaveDataInfo info{};
        s64 total = 0;
        u64 saveDataId = 0;
        bool found = false;

        while (true) {
            R_TRY(fsSaveDataInfoReaderRead(&reader, &info, 1, &total));
            if (total == 0) {
                break;
            }

            if (info.save_data_type == FsSaveDataType_Account &&
                info.application_id == programId &&
                !memcmp(&info.uid, &account, sizeof(AccountUid))) {
                saveDataId = info.save_data_id;
                found = true;
                break;
            }
        }

        if (!found) {
            return SYNC_RC(Result_ArchiveTimestampNotFound);
        }

        FsSaveDataExtraData extraData{};
        R_TRY(fsReadSaveDataFileSystemExtraData(&extraData, sizeof(extraData), saveDataId));

        outTimestamp = extraData.timestamp;
        R_SUCCEED();
    }

    void UnmountSaveFile(FsFileSystem &fs) {
        fsdevUnmountDevice(BackupMount);
    }

}
