#include <switch.h>
#include <sync.hpp>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <fstream>
#include <cstdarg>

#include "../tcp/sys_connection.hpp"
#include "fs.hpp"

namespace fs {

    namespace {

        constexpr const char *BackupMount = "save";

        int socket = 0;

        std::mutex logMutex{};

        Result MountSaveFile(AccountUid &account, u64 programId, FsFileSystem &outFs, std::string &outPath) {
            const FsSaveDataAttribute saveAttribute = {
                .application_id = programId,
                .uid            = account,
                .save_data_type = FsSaveDataType_Account,
                .save_data_rank = FsSaveDataRank_Primary,
            };

            R_TRY(fsOpenSaveDataFileSystem(&outFs, FsSaveDataSpaceId_User, &saveAttribute));

            Result rc = fsdevMountDevice(BackupMount, outFs);
            if (R_FAILED(rc)) {
                fsFsClose(&outFs);
                return rc;
            }

            outPath = BackupMount;
            outPath += ":/";

            R_SUCCEED();
        }

        u64 GetTimeStamp(FsFileSystem &fs, const std::string &internalPath) {
            FsTimeStampRaw ts{};
            Result rc = fsFsGetFileTimeStampRaw(&fs, internalPath.c_str(), &ts);

            if (R_FAILED(rc) || !ts.is_valid) {
                return 0;
            }

            return ts.modified;
        }

        void Synchronize(u64 ts, const std::string &path) {
            if (SynchronizeSaves(socket, ts, path, path)) {
                /* todo */
            }
            /* todo */
        }

        void IterateRecursively(FsFileSystem &fs, const std::string &mountedDir, const std::string &internalDir) {
            DIR *dir = opendir(mountedDir.c_str());

            if (!dir) {
                return;
            }

            ON_SCOPE_EXIT { closedir(dir); };

            dirent *entry{};
            while ((entry = readdir(dir)) != nullptr) {
                if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
                    continue;
                }

                const std::string mountedPath  = mountedDir + "/" + entry->d_name;
                const std::string internalPath = internalDir + "/" + entry->d_name;

                struct stat st{};
                if (stat(mountedPath.c_str(), &st) != 0) {
                    continue;
                }

                if (S_ISDIR(st.st_mode)) {
                    IterateRecursively(fs, mountedPath, internalPath);
                    continue;
                }

                u64 ts = GetTimeStamp(fs, mountedPath);
                if (!ts) {
                    continue;
                }

                Synchronize(ts, internalPath);
            }
        }
    }

    Result IterateSavefile(AccountUid &account, u64 programId) {
        FsFileSystem saveFileSystem{};
        std::string path;

        R_TRY(MountSaveFile(account, programId, saveFileSystem, path));
        socket = tcp::EstablishConnection();

        ON_SCOPE_EXIT { close(socket); };
        ON_SCOPE_EXIT { fsdevUnmountDevice(BackupMount); };

        R_UNLESS(socket >= 0, SYNC_RC(Result_ConnectionFailed));

        IterateRecursively(saveFileSystem, path, "");

        R_SUCCEED();
    }

    void Log(const char *fmt, ...) {
        std::scoped_lock lock{logMutex};

        va_list args;
        va_start(args, fmt);
        FILE *file = fopen(FileLogPath, "a");

        if (file) {
            timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            tm *nowTm = localtime(&now.tv_sec);

            fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d.%03ld] ", nowTm->tm_year+1900, nowTm->tm_mon+1, nowTm->tm_mday, nowTm->tm_hour, nowTm->tm_min, nowTm->tm_sec, now.tv_nsec / 1000000UL);
            vfprintf(file, fmt, args);
            fprintf(file, "\n");
            fclose(file);
        }

        va_end(args);
    }

}
