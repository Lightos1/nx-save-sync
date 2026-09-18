#pragma once
#include <string>
#include <vector>

#if defined(__SWITCH__)
    #include <switch.h>
#else
    #include <cstdint>

    typedef uint8_t u8;
    typedef int8_t s8;
    typedef uint16_t u16;
    typedef int16_t s16;
    typedef uint32_t u32;
    typedef int32_t s32;
    typedef uint64_t u64;
    typedef int64_t s64;

    typedef u32 Result;
#endif

enum MsgType : u8 {
    MsgDate      = 1,
    MsgProgramId = 2,
    MsgFileMeta  = 3,
    MsgFileChunk = 4,
    MsgDone      = 5,
};

int ConnectOrListen(const std::string &peerIp, u16 peerPort, u16 listenPort);
Result SynchronizeSaves(int socket, u64 myTs, const std::string &savePath, const std::string &outPath, const u64 programId);
bool SendMessage(int socket, MsgType type, const void *payload, u32 length);
bool ReceiveMessage(int socket, MsgType &type, std::vector<u8> &payload);
bool ExchangeDate(int socket, u64 myTs, u64 &peerTs);
