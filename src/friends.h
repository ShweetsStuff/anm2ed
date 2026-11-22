#pragma once

#include "embed.h"

struct FriendInfo
{
    const char* name = nullptr;
    const Embed* png = nullptr;
    const Embed* anm2 = nullptr;
};

#define FRIENDS_LIST                                                                                                   \
    X(FRIEND_MEAT_BOY, "Meat Boy", FRIENDS_MEAT_BOY)                                                                   \
    X(FRIEND_ISAAC, "Isaac", FRIENDS_ISAAC)                                                                            \
    X(FRIEND_STACY, "Stacy", FRIENDS_STACY)                                                                            \
    X(FRIEND_ASH, "Ash", FRIENDS_ASH)

enum Friend
{
#define X(symbol, name, embed) symbol,
    FRIENDS_LIST
#undef X
        FRIEND_COUNT
};

constexpr FriendInfo FRIENDS[FRIEND_COUNT] = {
#define X(symbol, name, embed) {name, &embed##_PNG, &embed##_ANM2},
    FRIENDS_LIST
#undef X
};
