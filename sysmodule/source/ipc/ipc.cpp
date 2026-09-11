#include <switch.h>
#include <mutex>

#include "ipc_server.h"

namespace ipc {

    bool gRunning = false;
    Thread gThread;
    std::mutex gThreadMutex;
    IpcServer gServer;

    #define IPC_SERVICE_NAME "sys:syn"

    Result ServiceHandlerFunc(void *arg, const IpcServerRequest *r, u8 *out_data, size_t *out_dataSize) {
        (void) arg;

        switch (r->data.cmdId) {
            default:
                break;
        }

        return 1;
    }

    void ProcessThreadFunc(void* arg) {
        (void)arg;
        Result rc;
        while (true) {
            rc = ipcServerProcess(&gServer, &ServiceHandlerFunc, nullptr);
            if (R_FAILED(rc)) {
                if (rc == KERNELRESULT(Cancelled)) {
                    return;
                }
                if (rc != KERNELRESULT(ConnectionClosed)) {
                    // fs::Log("[ipc] ipcServerProcess: [0x%x] %04d-%04d", rc, R_MODULE(rc), R_DESCRIPTION(rc));
                }
            }
        }
    }

    void Initialize() {
        Result rc;

        s32 priority;
        rc = svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
        if (R_FAILED(rc)) {
            // fs::Log("[ipc] svcGetThreadPriority failed: [0x%x] %04d-%04d", rc, R_MODULE(rc), R_DESCRIPTION(rc));
            return;
        }

        rc = ipcServerInit(&gServer, IPC_SERVICE_NAME, 42);
        if (R_FAILED(rc)) {
            // fs::Log("[ipc] ipcServerInit failed: [0x%x] %04d-%04d", rc, R_MODULE(rc), R_DESCRIPTION(rc));
            return;
        }

        rc = threadCreate(&gThread, &ProcessThreadFunc, nullptr, NULL, 0x4000, priority, -2);
        if (R_FAILED(rc)) {
            // fs::Log("[ipc] threadCreate failed: [0x%x] %04d-%04d", rc, R_MODULE(rc), R_DESCRIPTION(rc));
            ipcServerExit(&gServer);
            return;
        }

        gRunning = false;
    }

    void SetRunning(bool running) {
        std::scoped_lock lock{gThreadMutex};
        if (gRunning == running) {
            return;
        }

        gRunning = running;
        if (running) {
            Result rc = threadStart(&gThread);
            if (R_FAILED(rc)) {
                // fs::Log("[ipc] threadStart failed: [0x%x] %04d-%04d", rc, R_MODULE(rc), R_DESCRIPTION(rc));
                gRunning = false;
                return;
            }
        } else {
            // fs::Log("[ipc] Stopping thread...");
            svcCancelSynchronization(gThread.handle);
            threadWaitForExit(&gThread);
            // fs::Log("[ipc] Thread stopped");
        }
    }

    void Exit() {
        SetRunning(false);

        Result rc = threadClose(&gThread);
        if (R_FAILED(rc)) {
            // fs::Log("[ipc] threadClose failed: [0x%x] %04d-%04d", rc, R_MODULE(rc), R_DESCRIPTION(rc));
        }

        rc = ipcServerExit(&gServer);
        if (R_FAILED(rc)) {
            // fs::Log("[ipc] ipcServerExit failed: [0x%x] %04d-%04d", rc, R_MODULE(rc), R_DESCRIPTION(rc));
        }

        // fs::Log("[ipc] Exited");
    }

}