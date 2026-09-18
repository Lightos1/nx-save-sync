#include <cstdlib>

#include <switch.h>
#include <sync/result.hpp>
#include <sync/scope_exit.hpp>

namespace ns {

    Result GetInstalledProgramList(NsApplicationRecord *&outRecords, s32 &outCount) {
        R_TRY(nsInitialize());
        ON_SCOPE_EXIT{ nsExit(); };

        constexpr s32 maxRecords = 1024;
        outRecords = static_cast<NsApplicationRecord *>(malloc(sizeof(NsApplicationRecord) * maxRecords));
        R_UNLESS(outRecords != nullptr, SYNC_RC(Result_InvalidPointer));

        return nsListApplicationRecord(outRecords, maxRecords, 0, &outCount);
    }

}
