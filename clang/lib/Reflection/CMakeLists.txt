set(LLVM_LINK_COMPONENTS
  Core
  Demangle
  FrontendHLSL
  FrontendOpenMP
  MC
  Support
  TargetParser
  )

add_clang_library(clangReflection
  Reflection.cpp

  DEPENDS
  ClangOpenCLBuiltinsImpl
  acc_gen
  omp_gen
  ClangDriverOptions

  LINK_LIBS
  clangAPINotes
  clangAST
  clangAnalysis
  clangBasic
  clangEdit
  clangLex
  clangSupport
  )
