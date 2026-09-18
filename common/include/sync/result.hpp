#pragma once

#if !defined(__SWITCH__)
    #define MAKERESULT(module, desc) ((((module) & 0x1FF)) | (((desc) & 0x1FFF) << 9))
    #define R_FAILED(res) ((res) != 0)
    #define R_SUCCEEDED(res) ((res) == 0)
#endif

constexpr u32 SysSyncModule = 350;

enum SysSyncResult {
    Result_AccountNotFound = 0,
    Result_InvalidSocket,
    Result_SynchronizationFailed,
    Result_ConfigNotFound,
    Result_ArchiveTimestampNotFound,
    Result_TsExchangeFailed,
    Result_ZippingFileFailed,
    Result_FileTransferFailed,
    Result_SendFileFailed,
    Result_ReceiveFileFailed,
    Result_InvalidPointer,
    Result_InvalidApplicationCount,
    Result_ProgramIdExchangeFailed,
};

#define SYNC_RC(x) MAKERESULT(SysSyncModule, x)

#define R_RETURN(rc) \
    do {             \
        return (rc); \
    } while (0)

#define R_SUCCEED()  \
    do {             \
        return 0;    \
    } while (0)

#define R_TRY(rc)            \
    do {                     \
        Result _r = (rc);    \
        if (R_FAILED(_r)) {  \
            return _r;       \
        }                    \
    } while (0)

#define R_UNLESS(cond, rc) \
    do {                   \
        if (!(cond)) {     \
            return (rc);   \
        }                  \
    } while (0)

#define R_SUCCEED_IF(cond) \
    do {                   \
        if ((cond)) {      \
            return 0;      \
        }                  \
    } while (0)

#define R_THROW(rc) \
    do {            \
        return (rc);\
    } while (0)
