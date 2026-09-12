#include <switch.h>
#include <sync/result.hpp>
#include <cstring>

Result GetAccountUid(const char *accountName, AccountUid &outUid) {
    AccountUid uids[ACC_USER_LIST_SIZE];
    s32 count = 0;

    R_TRY(accountInitialize(AccountServiceType_Application));

    TRY(accountListAllUsers(uids, ACC_USER_LIST_SIZE, &count));

    for (s32 i = 0; i < count; i++) {
        AccountProfile profile;

        R_TRY(accountGetProfile(&profile, uids[i]));

        AccountProfileBase base;
        R_TRY(accountProfileGet(&profile, &userdata, &base));

        if (strcmp(base->nickname, accountName) == 0) {
            outUid = uids[i];
            R_SUCCEED();
        }
    }

    return SYNC_RC(Result_AccountNotFound);
}
