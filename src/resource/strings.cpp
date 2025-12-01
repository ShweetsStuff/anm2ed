#include "strings.h"

namespace anm2ed
{
  Language language{ENGLISH};

  const char* localize(StringType type)
  {
    if ((language > LANGUAGE_COUNT || language < 0) || (type > STRING_TYPE_COUNT || type < 0)) return "undefined";
    return LANGUAGE_STRING_COLLECTIONS[language][type];
  }
}
