#include <switch.h>
#include <stdlib.h>

#include <sync.hpp>

#include "ipc/ipc.hpp"

/* Rip. */
#define INNER_HEAP_SIZE 0x100000

extern "C" {

    void *__real___cxa_throw(void *thrown_exception, void *pvar, void (*dest)(void *));
    void *__real__Unwind_Resume();
    void *__real___gxx_personality_v0();
    void __wrap___cxa_throw(void *thrown_exception, void *pvar, void (*dest)(void *)) {abort();}
    void __wrap__Unwind_Resume() {}
    void __wrap___gxx_personality_v0() {}

    u32 __nx_applet_type     = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    void __libnx_initheap(void) {
        static u8 innearHeap[INNER_HEAP_SIZE];
        extern void *fake_heap_start;
        extern void *fake_heap_end;

        fake_heap_start = innearHeap;
        fake_heap_end   = innearHeap + sizeof(innearHeap);
    }

    void __appInit(void) {
        Result rc;

        rc = smInitialize();
        if (R_FAILED(rc)) {
            diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
        }

        rc = setsysInitialize();
        if (R_SUCCEEDED(rc)) {
            SetSysFirmwareVersion fw;
            rc = setsysGetFirmwareVersion(&fw);
            if (R_SUCCEEDED(rc)) {
                hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
            }

            setsysExit();
        }

        rc = fsInitialize();
        if (R_FAILED(rc)) {
            diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
        }

        rc = fsdevMountSdmc();
        if (R_FAILED(rc)) {
            diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
        }

        rc = socketInitializeDefault();
        if (R_FAILED(rc)) {
            fatalThrow(rc);
        }
    }

    void __appExit(void) {
        fsExit();
        fsdevUnmountAll();
        socketExit();
    }
}

int main(int argc, char *argv[]) {
    ipc::Initialize();
    ipc::SetRunning(true);

    while (true) {
        svcSleepThread(10'000'000);
    }

    __builtin_unreachable();
}
