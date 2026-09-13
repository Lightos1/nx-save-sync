#include <sync.hpp>
#include <switch.h>
#include <string>
#include <sync.hpp>

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

}
