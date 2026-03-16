#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/ADT/RewriteBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <set>

using namespace clang;

namespace {
class PotashnikChecker : public RecursiveASTVisitor<PotashnikChecker> {
  bool changed_data = false;
  bool changed_pointer = false;
  VarDecl *decl;

public:
  PotashnikChecker(VarDecl *D) : decl(D) {}

  bool VisitUnaryOperator(UnaryOperator *Node) {
    if (!Node->isIncrementDecrementOp()) {
      return true;
    }
    Expr *exp = Node->getSubExpr()->IgnoreParenImpCasts();

    if (auto *DRE = dyn_cast<DeclRefExpr>(exp)) {
      if (DRE->getDecl() == decl) {
        if (decl->getType()->isReferenceType()) {
          changed_data = true;
        } else {
          changed_pointer = true;
        }
      }
    }

    if (auto *UO = dyn_cast<UnaryOperator>(exp)) {
      if (UO->getOpcode() == UO_Deref) {
        if (auto *DRE = dyn_cast<DeclRefExpr>(
                UO->getSubExpr()->IgnoreParenImpCasts())) {
          if (DRE->getDecl() == decl) {
            changed_data = true;
          }
        }
      }
    }

    return true;
  }

  bool VisitBinaryOperator(BinaryOperator *Node) {
    if (!Node->isAssignmentOp()) {
      return true;
    }

    Expr *exp = Node->getLHS()->IgnoreParenImpCasts();

    if (auto *DRE = dyn_cast<DeclRefExpr>(exp)) {
      if (DRE->getDecl() == decl) {
        if (decl->getType()->isReferenceType()) {
          changed_data = true;
        } else {
          changed_pointer = true;
        }
      }
    }

    if (auto *UO = dyn_cast<UnaryOperator>(exp)) {
      if (UO->getOpcode() == UO_Deref) {
        if (auto *DRE = dyn_cast<DeclRefExpr>(
                UO->getSubExpr()->IgnoreParenImpCasts())) {
          if (DRE->getDecl() == decl) {
            changed_data = true;
          }
        }
      }
    }

    return true;
  }

  bool isDataChanged() const { return changed_data; }

  bool isPointerChanged() const { return changed_pointer; }
};

class PotashnikVisitor final
    : public clang::RecursiveASTVisitor<PotashnikVisitor> {
  Rewriter &rewriter;

public:
  PotashnikVisitor(Rewriter &R) : rewriter(R) {}
  bool VisitVarDecl(VarDecl *D) {
    if (isa<ParmVarDecl>(D))
      return true;

    QualType type = D->getType();

    if (!type->isPointerType() && !type->isReferenceType())
      return true;

    if (auto *FD = dyn_cast<FunctionDecl>(D->getDeclContext())) {
      if (Stmt *body = FD->getBody()) {
        if (type->isPointerType()) {
          processPointer(D, body, type);
        } else if (type->isReferenceType()) {
          processReference(D, body, type);
        }
      }
    }

    return true;
  }

  bool VisitParmVarDecl(ParmVarDecl *D) {
    QualType type = D->getType();

    if (!type->isPointerType() && !type->isReferenceType())
      return true;

    if (auto *FD = dyn_cast<FunctionDecl>(D->getDeclContext())) {
      if (Stmt *body = FD->getBody()) {
        if (type->isPointerType()) {
          processPointer(D, body, type);
        } else if (type->isReferenceType()) {
          processReference(D, body, type);
        }
      }
    }

    return true;
  }

  void processPointer(VarDecl *D, Stmt *body, QualType type) {
    PotashnikChecker Checker(D);
    Checker.TraverseStmt(body);

    SourceLocation start_loc = D->getBeginLoc();
    SourceLocation var_name_loc = D->getLocation();

    bool already_const_data = type->getPointeeType().isConstQualified();
    bool already_const_pointer = type.isConstQualified();
    bool data_changed = Checker.isDataChanged();
    bool pointer_changed = Checker.isPointerChanged();
    bool const_data = !already_const_data && !data_changed;
    bool const_pointer = !already_const_pointer && !pointer_changed;

    if (const_data) {
      rewriter.InsertText(start_loc, "const ", true, true);
    }
    if (const_pointer) {
      rewriter.InsertText(var_name_loc, "const ");
    }
  }

  void processReference(VarDecl *D, Stmt *body, QualType type) {
    PotashnikChecker Checker(D);
    Checker.TraverseStmt(body);

    SourceLocation start_loc = D->getBeginLoc();

    bool already_const_ref = type.getNonReferenceType().isConstQualified();
    bool ref_changed = Checker.isDataChanged();
    bool const_ref = !already_const_ref && !ref_changed;

    if (const_ref) {
      rewriter.InsertText(start_loc, "const ");
    }
  }
};

class PotashnikConsumer final : public clang::ASTConsumer {
  Rewriter rewriter;

public:
  void HandleTranslationUnit(ASTContext &Context) override {
    rewriter.setSourceMgr(Context.getSourceManager(), Context.getLangOpts());
    PotashnikVisitor Visitor(rewriter);
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    const llvm::RewriteBuffer *RewriteBuf = rewriter.getRewriteBufferFor(
        Context.getSourceManager().getMainFileID());
    if (RewriteBuf) {
      llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());
    }
  }
};

class PotashnikAction : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef) override {
    return std::make_unique<PotashnikConsumer>();
  }
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
};

} // namespace

static clang::FrontendPluginRegistry::Add<PotashnikAction>
    X("potashnik_m_lab1_plugin", "Replaces pointers and references to const "
                                 "pointers and references when it is possible");
