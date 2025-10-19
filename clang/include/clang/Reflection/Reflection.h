
#ifndef CLANG_REFLECTION_H
#define CLANG_REFLECTION_H

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"

using namespace clang;

namespace clang {
  class Decl;
}

namespace clang
{
  // This is a struct representation of the additional members in SkapeReflectedAttr
  struct ReflectionInfoExample
  {
    SmallString<16> DisplayName = {};
    SmallString<64> Description = {};
  };

  class SkapeReflectedFunctionAttr : public SkapeReflectedAttr {
  public:
    // A container of smaller reflection info.
    struct ParamReflection {
      // Editor Only
      enum class Kind : uint8_t {
        // The parameter will only be used as an input node
        In,
        // The parameter will only be used as an output node
        Out,
        // The parameter will be both an input and output node
        InOut,
      };
      SmallString<16> DisplayName = {};
      SmallString<64> Description = {};
    };
    
    SkapeReflectedFunctionAttr( ASTContext& Ctx,
        const AttributeCommonInfo& CommonInfo )
      : SkapeReflectedAttr( Ctx, CommonInfo ) {
    }

    SmallVector<ParamReflection, 4> Params;
  };
} // clang::

#endif