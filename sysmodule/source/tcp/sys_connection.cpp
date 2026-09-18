#include <sync.hpp>
#include <switch.h>

#include <string>
#include <fstream>

#include "../fs/fs.hpp"
#include "../ns/ns.hpp"
#include "../account.hpp"

namespace tcp {

    Result EstablishConnection(int &socket) {
        char ip[32];
        if (!GetConfigValueStr(ConfigValue_PeerIp, ip, sizeof(ip), "0.0.0.0")) {
            return SYNC_RC(Result_ConfigNotFound);
        }

        std::string peerIp(ip);

        u16 peerPort   = GetConfigValue(ConfigValue_PeerPort);
        u16 listenPort = GetConfigValue(ConfigValue_ListenPort);

        socket = ConnectOrListen(peerIp, peerPort, listenPort);

        if (socket < 0) {
            return SYNC_RC(Result_InvalidSocket);
        }

        R_SUCCEED();
    }

    Result PerformSync() {
        AccountUid uid{};
        char accountName[32]; /* Todo check max account length. */

        if (!GetConfigValueStr(ConfigValue_AccountName, accountName, sizeof(accountName), "")) {
            return SYNC_RC(Result_ConfigNotFound);
        }

        if (accountName[0] == '\0') {
            return SYNC_RC(Result_AccountNotFound);
        }

        R_TRY(GetAccountUid(accountName, uid));

        NsApplicationRecord *records = nullptr;
        s32 applicationCount         = 0;
        Result rc = ns::GetInstalledProgramList(records, applicationCount);
        if (R_FAILED(rc)) {
            if (records != nullptr) {
                free(records);
            }

            return rc;
        }
        /* This is safe because buffer must be a valid pointer if the function succeeds. */
        ON_SCOPE_EXIT{free(records); };

        fs::Log("applicationCount = %d", applicationCount);

        for (s32 i = 0; i < applicationCount; ++i) {
            const s64 programId = records[i].application_id;
            fs::Log("programId = 0%llX", programId);

            u64 localTs = 0;
            rc = fs::GetSaveDataArchiveTimestamp(uid, programId, localTs);
            if (rc == SYNC_RC(Result_ArchiveTimestampNotFound)) {
                continue;
            }
            R_TRY(rc);

            fs::Log("GetSaveDataArchiveTimestamp succeeded");

            int socket = 0;
            R_TRY(EstablishConnection(socket));
            ON_SCOPE_EXIT { close(socket); };

            fs::Log("Established connection");

            FsFileSystem fs{};
            std::string saveFilePath{};
            R_TRY(fs::MountSaveFile(uid, programId, fs, saveFilePath));
            ON_SCOPE_EXIT { fs::UnmountSaveFile(fs); };
            fs::Log("Mounted save file");

            char zipPathC[256];
            if (!GetConfigValueStr(ConfigValue_ZipPath, zipPathC, sizeof(zipPathC), "/config/nx-save-sync/")) {
                return SYNC_RC(Result_ConfigNotFound);
            }
            std::string zipPath(zipPathC);

            fs::Log("Ziped file");

            R_TRY(SynchronizeSaves(socket, localTs, saveFilePath, zipPath, programId));
            fs::Log("Synchronized save files");
        }

        R_SUCCEED();
    }
}
