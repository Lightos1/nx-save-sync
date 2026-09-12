#pragma once
#include <switch.h>

constexpr u32 SysSyncModule = 0x359;

enum SysSyncResult {
    Result_AccountNotFound = 0,
    Result_ConnectionFailed,
    Result_SynchronizationFailed,
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
