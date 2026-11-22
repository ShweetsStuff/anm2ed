#pragma once

#include "embed.h"

namespace anm2ed::resource::friends
{
  struct Info
  {
    const char* name{};
    const Embed* png{};
    const Embed* anm2{};
  };

#define FRIENDS_LIST                                                                                                   \
  X(MEAT_BOY, "Meat Boy", FRIENDS_MEAT_BOY)                                                                            \
  X(ISAAC, "Isaac", FRIENDS_ISAAC)                                                                                     \
  X(STACY, "Stacy", FRIENDS_STACY)                                                                                     \
  X(ASH, "Ash", FRIENDS_ASH)

  enum Type
  {
#define X(symbol, name, embed) symbol,
    FRIENDS_LIST
#undef X
        COUNT
  };

  constexpr Info FRIENDS[COUNT] = {
#define X(symbol, name, embed) {name, &embed##_PNG, &embed##_ANM2},
      FRIENDS_LIST
#undef X
  };
}
