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
#endif

enum MsgType : u8 {
    MsgDate      = 1,
    MsgFileMeta  = 2,
    MsgFileChunk = 3,
    MsgDone      = 4,
};

int ConnectOrListen(const std::string &peerIp, u16 peerPort, u16 listenPort);
bool SynchronizeSaves(int socket, u64 myTs, const std::string &savePath, const std::string &outdir);
bool SendMessage(int socket, MsgType type, const void *payload, u32 length);
bool ReceiveMessage(int socket, MsgType &type, std::vector<u8> &payload);
