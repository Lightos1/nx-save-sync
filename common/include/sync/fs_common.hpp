#pragma once
#include <string>

namespace fs {

    int PackageZip(const char *savePath, const char *outPath);
    void Log(const char *fmt, ...);

}
