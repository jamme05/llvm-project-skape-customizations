
#ifndef CLANG_REFLECTION_H
#define CLANG_REFLECTION_H

#include "llvm/ADT/StringRef.h"

namespace clang {
  class Decl;
}

namespace clang
{
  struct ReflectionInfo
  {
    Decl*       Declaration;
    std::string DisplayName = {};
    // TODO: More reflection settings.
  };
}

#endif