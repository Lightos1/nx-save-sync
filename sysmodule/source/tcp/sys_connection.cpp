#include <sync.hpp>
#include <switch.h>
#include <string>
#include <sync.hpp>

namespace tcp {

    int EstablishConnection() {
        char ip[32];
        if (!GetConfigValueStr(ConfigValue_PeerIp, ip, sizeof(ip), "0.0.0.0")) {
            return -1; /* todo return result. */
        }

        std::string peerIp(ip);

        u16 peerPort       = GetConfigValue(ConfigValue_PeerPort);
        u16 listenPort     = GetConfigValue(ConfigValue_ListenPort);

        int socket = ConnectOrListen(peerIp, peerPort, listenPort);

        if (socket < 0) {
            return -1;
        }

        return socket;
    }

}
