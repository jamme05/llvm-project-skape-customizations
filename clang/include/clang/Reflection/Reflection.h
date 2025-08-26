
#ifndef CLANG_REFLECTION_H
#define CLANG_REFLECTION_H

#include "llvm/ADT/StringRef.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

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