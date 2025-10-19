//===- PrintFunctionNames.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which simply prints the names of all the top-level decls
// in the input file.
//
//===----------------------------------------------------------------------===//

#include "clang/Reflection/Reflection.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Sema/Sema.h"

namespace clang {
struct LateParsedTemplate;
}

class HandleReflectionConsumer : public ASTConsumer {
  CompilerInstance &Instance;
public:
  HandleReflectionConsumer(CompilerInstance &Instance)
  : Instance(Instance) {}

  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    for ( Decl *D : DG ) {
      if (!D->hasAttr<SkapeReflectedAttr>())
        continue;

      if (const auto TD = dyn_cast_or_null<TagDecl>(D))
        HandleTagDecl( TD );
      else if (const auto FD = dyn_cast_or_null<FunctionDecl>(D))
        HandleFunctionDecl(FD);
    }

    return true;
  }

  // class/struct/union/enum
  void HandleTagDecl(const TagDecl* TD) {
    // At this point we will want to check 
    llvm::outs() << "Reflected " << TD->getKindName() << " with name: " << TD->getName() << "\n";
    if (auto DisplayNameAttr = TD->getAttr<SkapeReflectedDisplayNameAttr>())
      llvm::outs() << "  Display name: " << DisplayNameAttr->getName() << "\n";
    if (auto DescriptionAttr = TD->getAttr<SkapeReflectedDescriptionAttr>())
      llvm::outs() << "  Description:  " << DescriptionAttr->getDescription() << "\n";
    llvm::outs() << "\n";
  }

  // functions. We also parse the params here
  void HandleFunctionDecl(FunctionDecl *FD) {
    llvm::outs() << "Reflected function with name: " << FD->getName() << "\n";
    if (auto DisplayNameAttr = FD->getAttr<SkapeReflectedDisplayNameAttr>())
      llvm::outs() << "  Display name: " << DisplayNameAttr->getName() << "\n";
    if (auto DescriptionAttr = FD->getAttr<SkapeReflectedDescriptionAttr>())
      llvm::outs() << "  Description:  " << DescriptionAttr->getDescription() << "\n";

    llvm::outs() << "  Params:\n";
    for (unsigned i = 0; i < FD->getNumParams(); ++i) {
      auto P = FD->getParamDecl(i);
      llvm::outs() << "    " << P->getName() << "\n";
      if (auto DisplayNameAttr = P->getAttr<SkapeReflectedDisplayNameAttr>())
        llvm::outs() << "      Display name: " << DisplayNameAttr->getName() << "\n";
      if (auto DescriptionAttr = P->getAttr<SkapeReflectedDescriptionAttr>())
        llvm::outs() << "      Description:  " << DescriptionAttr->getDescription() << "\n";
      if (auto KindAttr = P->getAttr<SkapeReflectedParamKindAttr>()) {
        switch (KindAttr->getParamKind()) {
        case SkapeReflectedParamKindAttr::Kind::In:    llvm::outs() << "      Kind: In   \n"; break;
        case SkapeReflectedParamKindAttr::Kind::Out:   llvm::outs() << "      Kind: Out  \n"; break;
        case SkapeReflectedParamKindAttr::Kind::InOut: llvm::outs() << "      Kind: InOut\n"; break;
        }
        
      }
    }
  }
  
  void HandleTranslationUnit(ASTContext& context) override {
    // This demonstrates how to force instantiation of some templates in
    // -fdelayed-template-parsing mode. (Note: Doing this unconditionally for
    // all templates is similar to not using -fdelayed-template-parsig in the
    // first place.)
    // The advantage of doing this in HandleTranslationUnit() is that all
    // codegen (when using -add-plugin) is completely finished and this can't
    // affect the compiler output.
  }
};

class PrintFunctionNamesAction : public PluginASTAction {
protected:
  ActionType getActionType() override {
    return CmdlineAfterMainAction;
  }
  
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    return std::make_unique<HandleReflectionConsumer>(CI);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }

};

void register_skape_plugin() {
  static const FrontendPluginRegistry::Add<PrintFunctionNamesAction> p{ "skape-reflection", "Experimental" };
}

