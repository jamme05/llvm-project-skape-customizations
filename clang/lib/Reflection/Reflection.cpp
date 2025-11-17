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

#include "clang/AST/Comment.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Sema/Sema.h"

namespace clang {
struct LateParsedTemplate;
}


class SkapeReflectionPluginAction : public PluginASTAction {
public:
  SmallVector<Decl*> ReflectionDecls = {};
  VarDecl*           OutputDecl      = nullptr;

  std::unordered_map<std::string, TagDecl*> ReflectionStructDecls = {
    { "sBaseInfo",       nullptr },
    { "sBase",           nullptr },
    { "sParamInfo",      nullptr },
    { "sFunctionInfo",   nullptr },
    { "sVariableInfo",   nullptr },
    { "sMember",         nullptr },
    { "sPureStruct",     nullptr },
    { "sMemberVariable", nullptr },
    { "sMemberFunction", nullptr },
    { "sPureStruct",     nullptr },
    { "sClassInfo",      nullptr },
    { "sEnumInfo",       nullptr },
    { "sBaseTypeInfo",   nullptr },
  };

  inline static std::unique_ptr<SkapeReflectionPluginAction> instance = nullptr;
  
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override;
protected:
  ActionType getActionType() override {
    return CmdlineAfterMainAction;
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }

};


class HandleReflectionConsumer : public ASTConsumer {
  CompilerInstance &CI;
  SkapeReflectionPluginAction& Act;
public:
  HandleReflectionConsumer(CompilerInstance &Instance, SkapeReflectionPluginAction &Action )
  : CI(Instance), Act(Action) {
  }

  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    // TODO: Create APValues with the reflection info.
    for (Decl *D : DG) {
      if (!D->hasAttr<SkapeReflectedAttr>())
        continue;

      // TODO: Add validations
      if (auto VD = dyn_cast_if_present<VarDecl>(D); VD != nullptr)
        Act.OutputDecl = VD;
      
      Act.ReflectionDecls.emplace_back(D);
    }

    return true;
  }

  void HandleTagDeclDefinition(TagDecl *D) override {
    if (!D->hasAttr<SkapeReflectedAttr>())
      return;
    
    if (auto RD = dyn_cast_or_null<CXXRecordDecl>(D); RD != nullptr) {
      // Handle Classes/Structs
      if (RD->getTagKind() != TagTypeKind::Struct && RD->getTagKind() != TagTypeKind::Class) {
        // TODO: Print error due to Unions being unsupported CXX types.
        llvm::outs() << "What: " << RD->getQualifiedNameAsString() << "\n";
        return;
      }
      HandleRecordDecl(RD);
    } else {
      // TODO: Handle Enums
    }
  }

  void HandleRecordDecl(CXXRecordDecl* D) {
    if (const auto itr = Act.ReflectionStructDecls.find(D->getName().data());
      itr != Act.ReflectionStructDecls.end() ) {
      itr->second = D;
      llvm::outs() << "Found: " << D->getName() << " As: " << D->getQualifiedNameAsString() << "\n";
    }
    Act.ReflectionDecls.emplace_back(D);
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
    auto& Ctx = CI.getASTContext();
    
    APValue ReflectionInfo{APValue::UninitStruct{}, 1, 2};
    auto&   BaseInfo = ReflectionInfo.getStructBase(0);

    
    handleBaseInfo(FD, BaseInfo);

    BaseInfo.getStructField(3).setInt(llvm::APSInt{llvm::APInt{32, 0}});

    uint32_t flags = 0;

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

    // FD->isStatic();
    
    // Ctx.getUnnamedGlobalConstantDecl()
    

    // ReflectionDecls.emplace_back()
  }

  // Fills Name, DisplayName and Description values in the BaseInfo
  void handleBaseInfo(const NamedDecl* ND, APValue& Value) {
    auto& Ctx = CI.getASTContext();

    Value = APValue{APValue::UninitStruct{}, 0, 5};
    
    auto MakeStringLiteral = [&](StringRef Tmp) {
      using LValuePathEntry = APValue::LValuePathEntry;
      llvm::outs() << "Here\n";
      auto ArrType       = Ctx.getStringLiteralArrayType(Ctx.CharTy, static_cast<uint32_t>(Tmp.size()));
      llvm::outs() << "Here\n";
      StringLiteral *Res = StringLiteral::Create(Ctx, Tmp, StringLiteralKind::Ordinary, false, ArrType, {});
      llvm::outs() << "Here\n";
      
      // Decay the string to a pointer to the first character.
      LValuePathEntry Path[1] = {LValuePathEntry::ArrayIndex(0)};
      llvm::outs() << "Here\n";
      return APValue(Res, CharUnits::Zero(), Path, /*OnePastTheEnd=*/false);
    };

    auto MakeNullLiteral = [&] {
      using LValuePathEntry = APValue::LValuePathEntry;
      // Decay the string to a pointer to the first character.
      LValuePathEntry Path[1] = {LValuePathEntry::ArrayIndex(0)};
      return APValue(APValue::LValueBase{}, CharUnits::Zero(), Path, /*OnePastTheEnd=*/false, true);
    };

    llvm::outs() << "Name\n";
    // Name
    Value.getStructField(0) = MakeStringLiteral(ND->getName());
    llvm::outs() << "Name\n";

    llvm::outs() << "Display Name\n";
    // Display Name
    if (const auto DisplayNameAttr = ND->getAttr<SkapeReflectedDisplayNameAttr>())
      Value.getStructField(1) = MakeStringLiteral(DisplayNameAttr->getName());
    else
      Value.getStructField(1) = MakeNullLiteral();
    llvm::outs() << "Display Name\n";

    llvm::outs() << "Description\n";
    // Description
    if (const auto DescriptionAttr = ND->getAttr<SkapeReflectedDescriptionAttr>())
      Value.getStructField(2) = MakeStringLiteral(DescriptionAttr->getDescription());
    else if (const auto Comment = Ctx.getRawCommentForAnyRedecl(ND)) {
      const auto CommentString = Comment->getFormattedText(CI.getSourceManager(),CI.getDiagnostics());
      Value.getStructField(2) = MakeStringLiteral(CommentString);
    }
    else
        Value.getStructField(2) = MakeNullLiteral();
  }

  bool ValidateParmVarDecl( const ParmVarDecl* PVD ) {
    if (const auto FD = dyn_cast_or_null<FunctionDecl>(PVD->getParentFunctionOrMethod())) {
      return FD->hasAttr<SkapeReflectedAttr>();
    }
    
    llvm::errs() << "Unable to find function for param " << PVD->getName() << "\n";
    return false;
  }
  
  void HandleTranslationUnit(ASTContext& context) override {
    // This demonstrates how to force instantiation of some templates in
    // -fdelayed-template-parsing mode. (Note: Doing this unconditionally for
    // all templates is similar to not using -fdelayed-template-parsig in the
    // first place.)
    // The advantage of doing this in HandleTranslationUnit() is that all
    // codegen (when using -add-plugin) is completely finished and this can't
    // affect the compiler output.
    // context.getTranslationUnitDecl()
  }

  void PerformReflectionPass() {
    // TODO: Feed all the reflection data into a final array

    auto& Ctx = CI.getASTContext();

    for (auto D : Act.ReflectionDecls) {
      if (auto ND = dyn_cast_or_null<NamedDecl>(D))
        llvm::outs() << "Reflecting " << ND->getName() << "\n";
      if (const auto TD = dyn_cast_or_null<TagDecl>(D))
        HandleTagDecl(TD);
      if (const auto FD = dyn_cast_or_null<FunctionDecl>(D))
        HandleFunctionDecl(FD);
    }
    
    auto MakeStringLiteral = [&](StringRef Tmp) {
      using LValuePathEntry = APValue::LValuePathEntry;
      StringLiteral *Res = Ctx.getPredefinedStringLiteralFromCache(Tmp);
      // Decay the string to a pointer to the first character.
      LValuePathEntry Path[1] = {LValuePathEntry::ArrayIndex(0)};
      return APValue(Res, CharUnits::Zero(), Path, /*OnePastTheEnd=*/false);
    };
    
    // Ctx.getUnnamedGlobalConstantDecl(  )

    llvm::outs() << "Reflection Complete. " << Act.ReflectionDecls.size() << " types reflected \n";
  }
};

std::unique_ptr<ASTConsumer> SkapeReflectionPluginAction::CreateASTConsumer(
    CompilerInstance &CI, llvm::StringRef) {
  instance = std::make_unique<SkapeReflectionPluginAction>();
  return std::make_unique<HandleReflectionConsumer>(CI, *instance);
}

void reflection_pass(CompilerInstance& CI) {
  const auto consumer = SkapeReflectionPluginAction::instance->CreateASTConsumer(CI, "");
  static_cast<HandleReflectionConsumer&>(*consumer).PerformReflectionPass();
}

// Command line: clang++ -Xclang -add-plugin -Xclang skape-reflection -std=c++23 ReflectionTesting.cpp
void register_skape_plugin() {
  static const FrontendPluginRegistry::Add<SkapeReflectionPluginAction> p{ "skape-reflection", "Experimental" };
}

