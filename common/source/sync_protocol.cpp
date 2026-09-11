#include <sync/sync_protocol.hpp>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <algorithm>

#if !defined(__SWITCH__)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace {
    bool SendAll(int socket, const void *data, size_t length) {
        const char *bufferPointer = static_cast<const char *>(data);

        while (length > 0) {
            ssize_t bytesSend = send(socket, bufferPointer, length, 0);
            if (bytesSend <= 0) {
                return false;
            }

            bufferPointer += bytesSend;
            length        -= bytesSend;
        }

        return true;
    }

    bool ReceiveAll(int socket, void *data, size_t length) {
        char *bufferPointer = static_cast<char *>(data);

        while (length > 0) {
            ssize_t bytesReceived = recv(socket, bufferPointer, length, 0);

            if (bytesReceived <= 0) {
                return false;
            }

            bufferPointer += bytesReceived;
            length        -= bytesReceived;
        }

        return true;
    }
}

bool SendMessage(int socket, MsgType type, const void *payload, u32 length) {
    u32 netLength = htonl(length);
    u8 localType = static_cast<u8>(type);
    return SendAll(socket, &netLength, sizeof(netLength)) && SendAll(socket, &localType, sizeof(type)) && (length == 0 || SendAll(socket, payload, length));
}

bool ReceiveMessage(int socket, MsgType &type, std::vector<u8> &payload) {
    u32 netLength{};
    u8 typeLocal{};

    /* Get the length. */
    if (!ReceiveAll(socket, &netLength, sizeof(netLength))) {
        return false;
    }

    /* Get the message type. */
    if (!ReceiveAll(socket, &typeLocal, sizeof(typeLocal))) {
        return false;
    }

    payload.resize(ntohl(netLength));

    /* Receive the message. */
    if (!payload.empty() && !ReceiveAll(socket, payload.data(), payload.size())) {
        return false;
    }

    type = static_cast<MsgType>(typeLocal);
    return true;
}

static bool ExchangeDate(int socket, u64 myTs, uint64_t &peerTs) {
    if (!SendMessage(socket, MsgDate, &myTs, sizeof(myTs))) {
        return false;
    }

    MsgType type{};
    std::vector<u8> payload{};

    if (!ReceiveMessage(socket, type, payload) || type != MsgDate || payload.size() != sizeof(peerTs)) {
        return false;
    }

    std::memcpy(&peerTs, payload.data(), sizeof(peerTs));
    return true;
}

static bool SendFile(int socket, const std::string &path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return false;
    }

    u64 size = stream.tellg();
    stream.seekg(0);

    std::vector<uint8_t> meta(sizeof(size) + path.size());
    std::memcpy(meta.data(), &size, sizeof(size));
    std::memcpy(meta.data() + sizeof(size), path.data(), path.size());
    if (!SendMessage(socket, MsgFileMeta, meta.data(), meta.size())) {
        return false;
    }

    std::vector<u8> buf(32 * 1024);
    u64 remaining = size;

    while (remaining > 0) {
        size_t chunk = std::min<u64>(buf.size(), remaining);
        stream.read(buf.data(), chunk);

        if (!SendMessage(socket, MsgFileChunk, buf.data(), chunk)) {
            return false;
        }

        remaining -= chunk;
    }

    /* We're done. */
    return SendMessage(socket, MsgDone, nullptr, 0);
}

static bool ReceiveFile(int socket, const std::string &outDir) {
    MsgType type{};
    std::vector<u8> payload{};

    if (!ReceiveMessage(socket, type, payload) || type != MsgFileMeta) {
        return false;
    }

    u64 size{};
    std::memcpy(&size, payload.data(), sizeof(size));
    std::string fileName(reinterpret_cast<char *>(payload.data() + sizeof(u64)), payload.size() - sizeof(u64));

    std::ofstream out(outDir + "/" + fileName, std::ios::binary);

    if (!out) {
        return false;
    }

    u64 received{};
    while (received < size) {
        if (!ReceiveMessage(socket, type, payload) || type != MsgFileChunk) {
            return false;
        }

        out.write(reinterpret_cast<char *>(payload.data()), payload.size());
        received += payload.size();
    }

    /* Are we done? */
    return ReceiveMessage(socket, type, payload) && type == MsgDone;
}

bool SynchronizeSaves(int socket, u64 myTs, const std::string &savePath, const std::string &outDir) {
    u64 peerTs = 0;

    if (!ExchangeDate(socket, myTs, peerTs)) {
        return false;
    }

    if (myTs > peerTs) {
        return SendFile(socket, savePath);
    }

    if (myTs < peerTs) {
        return ReceiveFile(socket, outDir);
    }

    /* The dates match, there is nothing to do. */
    return true;
}

int ConnectOrListen(const std::string &peerIp, u16 peerPort, u16 listenPort) {
    /* Try to connect. */
    int sock        = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(peerPort);

    inet_pton(AF_INET, peerIp.c_str(), &addr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0) {
        /* Connected succesfully. */
        return sock;
    }
    close(sock);

    /* Fall back to listening. */
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    int opt          = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in listenAddr{};
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = INADDR_ANY;
    listenAddr.sin_port = htons(listenPort);

    if (bind(listenSocket, reinterpret_cast<sockaddr *>(&listenAddr), sizeof(listenAddr)) != 0) {
        return -1;
    }

    if (listen(listenSocket, 1) != 0) {
        return -1;
    }

    int clientSocket = accept(listenSocket, nullptr, nullptr);
    close(listenSocket);

    return clientSocket;
}
