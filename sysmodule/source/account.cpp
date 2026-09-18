#include <switch.h>
#include <sync/result.hpp>
#include <sync/fs_common.hpp>
#include <cstring>
#include "fs/fs.hpp"

Result GetAccountUid(char *accountName, AccountUid &outUid) {
    AccountUid uids[ACC_USER_LIST_SIZE];
    s32 count = 0;

    R_TRY(accountInitialize(AccountServiceType_System));
    R_TRY(accountListAllUsers(uids, ACC_USER_LIST_SIZE, &count));
    fs::Log("%s", accountName);

    for (s32 i = 0; i < count; i++) {
        AccountProfile profile;

        R_TRY(accountGetProfile(&profile, uids[i]));

        AccountProfileBase base;
        R_TRY(accountProfileGet(&profile, nullptr, &base));
        fs::Log("base.nickname %s", base.nickname);

        if (strcmp(base.nickname, accountName) == 0) {
            outUid = uids[i];
            R_SUCCEED();
        }
    }

    return SYNC_RC(Result_AccountNotFound);
}
