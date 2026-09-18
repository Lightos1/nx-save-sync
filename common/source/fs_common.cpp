#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <mutex>
#include <cstdarg>
#include <string>
#include <vector>
#include <sync/config.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include "miniz.h"

namespace fs {

    namespace {
        constexpr size_t MaxPath       = 512;
        constexpr int CompressionLevel = MZ_BEST_SPEED;

        struct DirEntryInfo {
            std::string name;
            bool isDir;
        };

        bool AddDirectoryToZip(mz_zip_archive &zipArchive, const char *folderPath, size_t rootLen, bool &anyFileAdded) {
            std::vector<DirEntryInfo> entries;
            char fullPath[MaxPath];

            {
                DIR *dir = opendir(folderPath);
                if (!dir) {
                    return false;
                }

                struct dirent *entry;
                while ((entry = readdir(dir)) != nullptr) {
                    const char *name = entry->d_name;

                    if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
                        continue;
                    }

                    if (snprintf(fullPath, sizeof(fullPath), "%s/%s", folderPath, name) >= static_cast<int>(sizeof(fullPath))) {
                        continue;
                    }

                    struct stat st{};
                    if (stat(fullPath, &st) != 0) {
                        continue;
                    }

                    if (S_ISDIR(st.st_mode)) {
                        entries.push_back({name, true});
                    } else if (S_ISREG(st.st_mode)) {
                        entries.push_back({name, false});
                    }
                }

                closedir(dir);
            }

            for (const auto &entry : entries) {
                if (snprintf(fullPath, sizeof(fullPath), "%s/%s", folderPath, entry.name.c_str()) >= static_cast<int>(sizeof(fullPath))) {
                    continue;
                }

                if (entry.isDir) {
                    AddDirectoryToZip(zipArchive, fullPath, rootLen, anyFileAdded);
                    continue;
                }

                const char *archiveName = fullPath + rootLen + 1;

                if (mz_zip_writer_add_file(&zipArchive, archiveName, fullPath, nullptr, 0, CompressionLevel)) {
                    anyFileAdded = true;
                }
            }

            return true;
        }
    }

    int PackageZip(const char *savePath, const char *outPath) {
        if (!savePath || !*savePath || !outPath || !*outPath) {
            return -1;
        }

        char folderPath[MaxPath];
        size_t len = strlen(savePath);
        if (len == 0 || len >= sizeof(folderPath)) {
            return -1;
        }

        memcpy(folderPath, savePath, len + 1);
        while (len > 0 && (folderPath[len - 1] == '/' || folderPath[len - 1] == '\\')) {
            folderPath[--len] = '\0';
        }

        if (len == 0) {
            return -1;
        }

        if (strlen(outPath) >= MaxPath) {
            return -1;
        }

        remove(outPath);

        mz_zip_archive zipArchive;
        memset(&zipArchive, 0, sizeof(zipArchive));

        if (!mz_zip_writer_init_file(&zipArchive, outPath, 0)) {
            return -1;
        }

        bool anyFileAdded = false;
        if (!AddDirectoryToZip(zipArchive, folderPath, len, anyFileAdded)) {
            mz_zip_writer_end(&zipArchive);
            return -1;
        }

        const bool finalized = mz_zip_writer_finalize_archive(&zipArchive);
        mz_zip_writer_end(&zipArchive);

        if (!finalized || !anyFileAdded) {
            remove(outPath);
            return -1;
        }

        return 0;
    }

    void Log(const char *fmt, ...) {
        static std::mutex logMutex{};

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
