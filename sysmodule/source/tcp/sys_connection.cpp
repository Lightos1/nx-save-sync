#include <sync.hpp>
#include <switch.h>
#include <string>

namespace tcp {

    int EstablishConnection() {
        std::string peerIP = "192.168.0.50";
        u16 peerPort       = 9000;
        u16 listenPort     = 9000;

        int socket = ConnectOrListen(peerIP, peerPort, listenPort);

        if (socket < 0) {
            return -1;
        }

        return socket;
    }

}
