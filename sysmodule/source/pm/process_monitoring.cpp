/*
 * Copyright (c) Souldbminer, Lightos_ and Horizon OC Contributors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include <switch.h>
#include <sync.hpp>
#include "../fs/fs.hpp"
#include "../tcp/sys_connection.hpp"

namespace pm {

    namespace {
        u64 currentProgramId = 0;
        constexpr u64 Qlaunch = 0x0100000000001000ULL;
        constexpr u32 ResultProcessNotFound = 0x20f;
    }

    Result Initialize() {
        R_TRY(pmdmntInitialize());
        R_TRY(pminfoInitialize());

        R_SUCCEED();
    }

    void WaitForQLaunch() {
        Result rc = 0;
        u64 pid = 0;
        do {
            rc = pmdmntGetProcessId(&pid, Qlaunch);
            svcSleepThread(50 * 1000000ULL);
        } while (R_FAILED(rc));
        currentProgramId = Qlaunch;
    }

    Result GetCurrentProgramId(u64 &programId) {
        u64 processId = 0;

        Result rc = pmdmntGetApplicationProcessId(&processId);
        if (rc == ResultProcessNotFound) {
            programId = Qlaunch;
            R_SUCCEED();
        }
        R_TRY(rc);

        rc = pminfoGetProgramId(&programId, processId);
        if (rc == ResultProcessNotFound) {
            programId = Qlaunch;
            R_SUCCEED();
        }
        R_TRY(rc);

        R_SUCCEED();
    }

    static bool IsSyncAllowed(u64 programId) {
        return programId == Qlaunch;
    }

    static bool IsExitingGame(u64 previousProgramId) {
        return previousProgramId != Qlaunch;
    }

    static void HandleSyncRc(Result result) {
        if (R_SUCCEEDED(result)) {
            return;
        }

        fs::Log("Failed to synchronize save files: %d", result);
    }

    void MonitorProcesses() {
        u64 programId          = 0;
        u64 previousProgramId  = 0;
        bool canSync           = false;
        bool startTimer        = false;
        u64 lastSyncTick       = 0;

        while (true) {
            u64 minutesUntilSync = GetConfigValue(ConfigValue_SyncIntervalMin);
            previousProgramId    = programId;

            Result rc = GetCurrentProgramId(programId);
            if (R_FAILED(rc)) {
                fs::Log("failed to get program id: %d", R_DESCRIPTION(rc));
            }

            if (previousProgramId == 0) {
                previousProgramId = programId;
            }

            canSync = IsSyncAllowed(programId);

            do {
                if (canSync && !startTimer) {
                    startTimer   = true;
                    lastSyncTick = armGetSystemTick();
                } else if (!canSync) {
                    startTimer = false;
                    break;
                }

                /* Exiting game. */
                if (IsExitingGame(previousProgramId)) {
                    HandleSyncRc(tcp::PerformSync());

                    startTimer = false;
                    break;
                }

                if (canSync && startTimer) {
                    u64 elapsedNs = armTicksToNs(armGetSystemTick() - lastSyncTick);
                    u64 targetNs  = minutesUntilSync * 60ULL * 1000000000ULL;

                    if (elapsedNs >= targetNs) {
                        HandleSyncRc(tcp::PerformSync());
                        lastSyncTick = armGetSystemTick();
                    }
                }
            } while (false);

            svcSleepThread(GetConfigValue(ConfigValue_ProcessRefreshIntervalSec) * 1'000'000'000ULL);
        }
    }

    void Exit() {
        pmdmntExit();
        pminfoExit();
    }

}
