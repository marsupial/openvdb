/*
clang++ test.cpp -Xclang -load -Xclang factionplug.dylib -Xclang -plugin -Xclang plugin-name
clang++ -cc1 test.cpp  -load factionplug.dylib -std=c++11
clang++ -cc1 test.cpp -ast-dump
./faction test.cpp
*/

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

class Inspector;

class FunctionVisitor : public RecursiveASTVisitor<FunctionVisitor> {
  Inspector& Inspect;
public:
  FunctionVisitor(Inspector &I) : Inspect(I) {}

  bool VisitDeclRefExpr(DeclRefExpr *Expr);
};

class Inspector {
  ASTContext       &Context;
  FunctionVisitor  FuncVisitor;
public:
  Inspector(ASTContext& Ctx) : Context(Ctx), FuncVisitor(*this) {}

  void ShowLocation(SourceLocation Loc, StringRef What = {}) {
    // getFullLoc uses the ASTContext's SourceManager to resolve the source
    // location and break it up into its line and column parts.
    FullSourceLoc FullLocation = Context.getFullLoc(Loc);
    if (FullLocation.isValid()) {
      llvm::outs() << "Found declaration at "
                   << FullLocation.getSpellingLineNumber() << ":"
                   << FullLocation.getSpellingColumnNumber() << "\n";
    } else if (!What.empty()) {
      llvm::errs() << "WTF!" << What << "\n";
    }
  }

  void Inspect(Decl *D, SourceLocation Loc = {}) {
    const NamedDecl *ND = dyn_cast<NamedDecl>(D);
    if (!ND)
      return;
    const FunctionDecl* FD = dyn_cast<FunctionDecl>(ND);
    if (FD) {
      llvm::errs() << FD->getName() << "---> \n";
      FuncVisitor.TraverseDecl(D);
      llvm::errs() << "<---" << FD->getName() << "\n";;
      return;
    }

    const VarDecl* VD = dyn_cast<VarDecl>(ND);
    if (!VD)
      return;

    // VD->isFileVarDecl();
    if (!VD->hasGlobalStorage())
      return;

    QualType Qt = VD->getType();
    if (Qt.isConstant(Context)) {
      llvm::errs() << "Skipping2: \"" << VD->getNameAsString() << "\"\n";
      return;
    }
    if (Qt->isPointerType()) {
      clang::QualType QtPointee = Qt->getPointeeType().getUnqualifiedType();
      if (Context.hasSameType(QtPointee, Context.CharTy) ||
          Context.hasSameType(QtPointee, Context.WCharTy) ||
          Context.hasSameType(QtPointee, Context.Char16Ty) ||
          Context.hasSameType(QtPointee, Context.Char32Ty)) {
        llvm::errs() << "Rewrite String Type\n";
      }
    }

    llvm::errs() << "Non thread safe: \"" << VD->getNameAsString() << "\"\n";
    
    ShowLocation(Loc.isValid() ? Loc : D->getBeginLoc(), ND->getName());
  }
};

bool FunctionVisitor::VisitDeclRefExpr(DeclRefExpr *Expr) {
  //Expr->dump();
  ValueDecl* ValDecl  = Expr->getDecl();
  //ValDecl->dump();
  VarDecl* VD = dyn_cast<VarDecl>(ValDecl);
  if (!VD)
    return true;

  Inspect.Inspect(VD, Expr->getBeginLoc());
  return true;
}

class PluginConsumer : public ASTConsumer {
  CompilerInstance &Instance;
  Inspector        Inspector;

public:
  PluginConsumer(CompilerInstance &Instance)
      : Instance(Instance), Inspector(Instance.getASTContext()) {}

  bool shouldSkipFunctionBody(Decl *D) override {
    return false;
  }

  void HandleCXXStaticMemberVarInstantiation(VarDecl *D) override {
    llvm::errs() << "Instantioation" << D->getName() << "\n";
  }

  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    ASTContext* Context = nullptr;
    for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
      Inspector.Inspect(*i);
    }
    return true;
  }

  void HandleTranslationUnit(ASTContext& context) override {
    return;
  }
};

class AstPlugin : public PluginASTAction {
public:
  // Automatically run the plugin after the main AST action
  ActionType getActionType() override {
    return AddAfterMainAction;
  }

  bool
  ParseArgs(const CompilerInstance &CI,
            const std::vector<std::string> &args) override {
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
      llvm::errs() << "PrintFunctionNames arg = " << args[i] << "\n";

      // Example error handling.
      DiagnosticsEngine &D = CI.getDiagnostics();
      if (args[i] == "-an-error") {
        unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error,
                                            "invalid argument '%0'");
        D.Report(DiagID) << args[i];
        return false;
      } else if (args[i] == "-parse-template") {
        if (i + 1 >= e) {
          D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                     "missing -parse-template argument"));
          return false;
        }
        ++i;
      }
    }
    if (!args.empty() && args[0] == "help")
      PrintHelp(llvm::errs());

    return true;
  }

  void PrintHelp(llvm::raw_ostream& ros) {
    ros << "Help for AstPlugin plugin goes here\n";
  }

  std::unique_ptr<ASTConsumer>
  CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override {
    return std::unique_ptr<ASTConsumer>(new PluginConsumer(CI));
  }
};

class ExamplePragmaHandler : public PragmaHandler {
public:
  ExamplePragmaHandler() : PragmaHandler("example_pragma") { }
  void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer,
                    Token &PragmaTok) {
    // Handle the pragma
  }
};

#if !defined(FACTION_MAIN)

static PragmaHandlerRegistry::Add<ExamplePragmaHandler> Y("example_pragma","example pragma description");
static FrontendPluginRegistry::Add<AstPlugin> X("plugin-name", "plugin description");

#else

class FileBuf {
  FILE* mFile;
public:
  FileBuf(const char* path) : mFile(fopen(path, "r")) {}
  ~FileBuf() { if (mFile) fclose(mFile); }

  std::vector<char> contents(bool close = true) {
    std::vector<char> contents;
    fseek(mFile, 0, SEEK_END);
    auto size = ftell(mFile);
    if (size) {
        fseek(mFile, 0, SEEK_SET);
        contents.resize(size+1);
        fread(contents.data(), size, 1, mFile);
        contents.back() = 0;
    }
    if (close) {
      fclose(mFile);
      mFile = nullptr;
    }
    return contents;
  }
};


int main(int argc, char **argv) {
  if (argc < 1)
    return -1;

  FileBuf fb(argv[1]);
  auto contents = fb.contents();
  if (contents.empty())
    return -1;

  std::vector<std::string> args;
  args.reserve(argc-2);
  for (int i = 2; i < argc; ++i)
    args.emplace_back(argv[i]);

  bool result = clang::tooling::runToolOnCodeWithArgs(new AstPlugin,
                                                      contents.data(), args);

  return result ? 0 : -1;
}

#endif

