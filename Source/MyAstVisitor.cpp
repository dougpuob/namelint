#include "MyAstVisitor.h"
#include "Common.h"
#include <iomanip>

using namespace namelint;

bool MyASTVisitor::_IsMainFile(Decl *pDecl) {
  if (this->m_pAstCxt->getSourceManager().isInMainFile(pDecl->getLocation())) {
    return true;
  }
  return false;
}

void MyASTVisitor::_KeepFileName(string &FilePath) {
  size_t nPos = FilePath.rfind("\\");
  if (nPos < 0) {
    nPos = FilePath.rfind("/");
  }

  if (nPos > 0) {
    FilePath = FilePath.substr(nPos + 1, FilePath.length() - nPos - 1);
  }
}

bool MyASTVisitor::_GetPosition(Decl *pDecl, string &FileName,
                                size_t &nLineNumb, size_t &nColNumb) {
  if (!this->m_pAstCxt) {
    this->m_pAstCxt = &pDecl->getASTContext();
    assert(false);
    return false;
  }

  bool bStatus = false;

  FullSourceLoc FullLocation =
      this->m_pAstCxt->getFullLoc(pDecl->getBeginLoc());
  if (FullLocation.isValid()) {
    FileName = FullLocation.getFileLoc().getFileEntry()->getName();

    if ((FileName != GetAppCxt()->FileName) || ("" == GetAppCxt()->FileName)) {
      APP_CONTEXT *pAppCxt = (APP_CONTEXT *)GetAppCxt();
      pAppCxt->FileName = FileName;

      string Path1;
      Path::NormPath(FileName.c_str(), Path1);

      String::Replace(Path1, "\\\\", "\\");
      String::Replace(Path1, "\"", "");
      FileName = Path1;
    }

    nLineNumb = FullLocation.getSpellingLineNumber();
    nColNumb = FullLocation.getSpellingColumnNumber();
    bStatus = true;
  }

  return bStatus;
}

bool MyASTVisitor::_PrintPosition(Decl *pDecl) {
  string FileName;
  size_t nLineNumb = 0;
  size_t nColNumb = 0;
  bool bStatus = _GetPosition(pDecl, FileName, nLineNumb, nColNumb);
  if (bStatus) {
    cout << "  <" << nLineNumb << "," << nColNumb << ">" << setw(12);
  }
  return bStatus;
}

bool MyASTVisitor::_ClassifyTypeName(string &TyeName) {
  bool bStatus = true;

  String::Replace(TyeName, "extern", "");
  String::Replace(TyeName, "static", "");
  String::Replace(TyeName, "struct", "");
  String::Replace(TyeName, "const", "");
  String::Replace(TyeName, "&", "");
  String::Replace(TyeName, "*", "");
  // String::Replace(TyeName, "* ", "*");
  // String::Replace(TyeName, " *", "*");
  String::Trim(TyeName);

  return bStatus;
}

bool MyASTVisitor::_GetFunctionInfo(FunctionDecl *pDecl, string &Name) {
  if (!pDecl->hasBody()) {
    return false;
  }
  if (!this->_IsMainFile(pDecl)) {
    return false;
  }

  Name = pDecl->getDeclName().getAsString();
  return true;
}

bool MyASTVisitor::_GetParmsInfo(ParmVarDecl *pDecl, string &VarType,
                                 string &VarName, bool &bIsPtr) {
  if (!pDecl) {
    return false;
  }
  if (!this->_IsMainFile(pDecl)) {
    return false;
  }

  SourceLocation MyBeginLoc = pDecl->getBeginLoc();
  SourceLocation MyLoc = pDecl->getLocation();
  string MyVarType =
      std::string(this->m_pSrcMgr->getCharacterData(MyBeginLoc),
                  this->m_pSrcMgr->getCharacterData(MyLoc) -
                      this->m_pSrcMgr->getCharacterData(MyBeginLoc));

  VarType = MyVarType;

  QualType MyQualType = pDecl->getType();

  VarName = pDecl->getName().data();
  bIsPtr = MyQualType->isPointerType();

  if (VarType.length() > 0) {
    this->_ClassifyTypeName(VarType);
  }

  String::Trim(VarType);
  String::Trim(VarName);

  return true;
}

bool MyASTVisitor::_GetVarInfo(VarDecl *pDecl, string &VarType, string &VarName,
                               bool &bIsPtr, bool &bIsArray,
                               bool &bIsBuiltinType) {
  if (!pDecl) {
    return false;
  }

  if (!this->_IsMainFile(pDecl)) {
    return false;
  }
  // TODO:
  // This will get var type, but need to overcome some situation:
  // 1. Type "unsigned long long int" will get "unsigned long long"
  // 2. Dependency on include file, if can't get definition, it will get "int"
  // 3. Multi-array issue,we need to process string like "[8][]" "[][][]"
  //...
  // auto VarType = pDecl->getType().getAsString();

  SourceLocation MyBeginLoc = pDecl->getBeginLoc();
  SourceLocation MyLoc = pDecl->getLocation();
  string MyVarType =
      std::string(this->m_pSrcMgr->getCharacterData(MyBeginLoc),
                  this->m_pSrcMgr->getCharacterData(MyLoc) -
                      this->m_pSrcMgr->getCharacterData(MyBeginLoc));
  size_t nPos = MyVarType.find(",");

  if (std::string::npos != nPos) {
    nPos = MyVarType.find(" ");
    MyVarType = MyVarType.substr(0, nPos);
  }

  if (MyVarType.length() > 0) {
    this->_ClassifyTypeName(MyVarType);
  }

  QualType MyQualType = pDecl->getType();

  VarType = MyVarType;
  VarName = pDecl->getNameAsString();
  bIsArray = MyQualType->isArrayType();
  bIsBuiltinType = MyQualType->isBuiltinType();
  bIsPtr = MyQualType->isPointerType();

  String::Trim(VarType);
  String::Trim(VarName);

  return true;
}

ErrorDetail *MyASTVisitor::_CreateErrorDetail(const string &FileName,
                                              const string &Suggestion) {
  return new ErrorDetail(FileName, Suggestion);
}

ErrorDetail *MyASTVisitor::_CreateErrorDetail(
    Decl *pDecl, const CheckType &CheckType, const bool &bIsPtr,
    const bool &bIsArray, const string &TargetName, const string &Suggestion) {

  return this->_CreateErrorDetail(pDecl, CheckType, bIsPtr, bIsArray, "",
                                  TargetName, Suggestion);
}

ErrorDetail *MyASTVisitor::_CreateErrorDetail(
    Decl *pDecl, const CheckType &CheckType, const bool &bIsPtr,
    const bool &bIsArray, const string &TypeName, const string &TargetName,
    const string &Suggestion) {
  if (!pDecl) {
    return NULL;
  }

  ErrorDetail *pNew = NULL;

  string FileName;
  CodePos Pos = {0};
  if (this->_GetPosition(pDecl, FileName, Pos.nLine, Pos.nColumn)) {
    pNew = new ErrorDetail(Pos, CheckType, bIsPtr, bIsArray, TypeName,
                           TargetName, Suggestion);
  }
  return pNew;
}

MyASTVisitor::MyASTVisitor(const SourceManager *pSM, const ASTContext *pAstCxt,
                           const Config *pConfig) {
  this->m_pSrcMgr = pSM;
  this->m_pAstCxt = (ASTContext *)pAstCxt;
  this->m_pConfig = pConfig->GetData();
  APP_CONTEXT *pAppCxt = (APP_CONTEXT *)GetAppCxt();

  {
    RuleOfFunction Rule;
    Rule.bAllowedUnderscopeChar =
        this->m_pConfig->General.Options.bAllowedUnderscopeChar;
    Rule.IgnoreNames = this->m_pConfig->General.IgnoredList.FunctionName;
    Rule.IgnorePrefixs = this->m_pConfig->General.IgnoredList.FunctionPrefix;

    this->m_Detect.ApplyRuleForFunction(Rule);
  }

  {
    RuleOfVariable Rule;
    Rule.bAllowedUnderscopeChar =
        this->m_pConfig->General.Options.bAllowedUnderscopeChar;

    Rule.IgnorePrefixs = this->m_pConfig->General.IgnoredList.VariablePrefix;
    Rule.WordListMap = this->m_pConfig->Hungarian.WordList;
    Rule.ArrayNamingMap = this->m_pConfig->Hungarian.ArrayList;
    Rule.NullStringMap = this->m_pConfig->Hungarian.NullStringList;

    this->m_Detect.ApplyRuleForVariable(Rule);
  }
}

bool MyASTVisitor::VisitFunctionDecl(clang::FunctionDecl *pDecl) {
  if (!this->m_pConfig->General.Options.bCheckFunctionName) {
    return true;
  }

  APP_CONTEXT *pAppCxt = ((APP_CONTEXT *)GetAppCxt());
  if (!pAppCxt) {
    assert(pAppCxt);
    return false;
  }

  this->m_LearnIt.LearnFunctionDecl(*pDecl);

  string FuncName;
  bool bResult = false;
  bool bIsPtr = false;
  bool bIsArray = false;
  bool bStatus = this->_GetFunctionInfo(pDecl, FuncName);
  if (bStatus) {
    bResult = this->m_Detect.CheckFunction(
        this->m_pConfig->General.Rules.FunctionName, FuncName);

    pAppCxt->TraceMemo.Checked.nFunction++;
    if (!bResult) {
      pAppCxt->TraceMemo.Error.nFunction++;

      pAppCxt->TraceMemo.ErrorDetailList.push_back(this->_CreateErrorDetail(
          pDecl, CheckType::CT_Function, bIsPtr, bIsArray, FuncName, ""));
    }

    const clang::ArrayRef<clang::ParmVarDecl *> MyRefArray =
        pDecl->parameters();
    for (size_t nIdx = 0; nIdx < MyRefArray.size(); nIdx++) {
      string VarType;
      string VarName;

      bool bIsPtr = false;
      bool bIsArray = false;

      ParmVarDecl *pParmVarDecl = MyRefArray[nIdx];

      bStatus = this->_GetParmsInfo(pParmVarDecl, VarType, VarName, bIsPtr);
      if (bStatus) {
        bResult = this->m_Detect.CheckVariable(
            this->m_pConfig->General.Rules.VariableName, VarType, VarName,
            this->m_pConfig->Hungarian.Others.PreferUpperCamelIfMissed, bIsPtr,
            bIsArray);

        pAppCxt->TraceMemo.Checked.nParameter++;
        if (!bResult) {
          pAppCxt->TraceMemo.Error.nParameter++;

          pAppCxt->TraceMemo.ErrorDetailList.push_back(
              this->_CreateErrorDetail(pParmVarDecl, CheckType::CT_Parameter,
                                       bIsPtr, bIsArray, VarType, VarName, ""));
        }
      }
    }
  }

  return bStatus;
}

bool MyASTVisitor::VisitCXXMethodDecl(CXXMethodDecl *pDecl) { return true; }

bool MyASTVisitor::VisitRecordDecl(RecordDecl *pDecl) {
    if (!this->_IsMainFile(pDecl)) {
        return false;
    }

    // clang-format OFF
    printf("---------------------------------------------------------------------------------\n");
    printf("pDecl->getName()                            = %s\n", pDecl->getName().str().c_str());
    printf("pDecl->getNameAsString()                    = %s\n", pDecl->getNameAsString().c_str());
    printf("pDecl->getDeclName()                        = %s\n", pDecl->getDeclName().getAsString().c_str());
    printf("pDecl->getKindName()                        = %s\n", pDecl->getKindName().str().c_str());
    // printf("pDecl->getDeclKindName()                    = %s\n", pDecl->getDeclKindName());
    printf("pDecl->getQualifiedNameAsString()           = %s\n", pDecl->getQualifiedNameAsString().c_str());
    printf("---------------------------------------------------------------------------------\n");
    // printf("pDecl->InEnclosingNamespaceSetOf()          = %d\n", pDecl->InEnclosingNamespaceSetOf());
    printf("pDecl->isAnonymousStructOrUnion()           = %d\n", pDecl->isAnonymousStructOrUnion());
    printf("pDecl->isBeingDefined()                     = %d\n", pDecl->isBeingDefined());
    printf("pDecl->isCanonicalDecl()                    = %d\n", pDecl->isCanonicalDecl());
    printf("pDecl->isCapturedRecord()                   = %d\n", pDecl->isCapturedRecord());
    printf("pDecl->isClass()                            = %d\n", pDecl->isClass());
    printf("pDecl->isClosure()                          = %d\n", pDecl->isClosure());
    printf("pDecl->isCompleteDefinition()               = %d\n", pDecl->isCompleteDefinition());
    printf("pDecl->isCompleteDefinitionRequired()       = %d\n", pDecl->isCompleteDefinitionRequired());
    printf("pDecl->isCXXClassMember()                   = %d\n", pDecl->isCXXClassMember());
    printf("pDecl->isCXXInstanceMember()                = %d\n", pDecl->isCXXInstanceMember());
    printf("pDecl->isExternCXXContext()                 = %d\n", pDecl->isExternCXXContext());
    // printf("pDecl->isDeclInLexicalTraversal()           = %d\n", pDecl->isDeclInLexicalTraversal());
    printf("pDecl->isDefinedOutsideFunctionOrMethod()   = %d\n", pDecl->isDefinedOutsideFunctionOrMethod());
    printf("pDecl->isDependentContext()                 = %d\n", pDecl->isDependentContext());
    printf("pDecl->isDependentType()                    = %d\n", pDecl->isDependentType());
    printf("pDecl->isDeprecated()                       = %d\n", pDecl->isDeprecated());
    printf("pDecl->isEmbeddedInDeclarator()             = %d\n", pDecl->isEmbeddedInDeclarator());
    printf("pDecl->isEnum()                             = %d\n", pDecl->isEnum());
    printf("pDecl->isExported()                         = %d\n", pDecl->isExported());
    printf("pDecl->isExternallyDeclarable()             = %d\n", pDecl->isExternallyDeclarable());
    printf("pDecl->isExternallyVisible()                = %d\n", pDecl->isExternallyVisible());
    printf("pDecl->isExternCContext()                   = %d\n", pDecl->isExternCContext());
    printf("pDecl->isExternCXXContext()                 = %d\n", pDecl->isExternCXXContext());
    printf("pDecl->isFileContext()                      = %d\n", pDecl->isFileContext());
    printf("pDecl->isFirstDecl()                        = %d\n", pDecl->isFirstDecl());
    printf("pDecl->isFreeStanding()                     = %d\n", pDecl->isFreeStanding());
    printf("pDecl->isFromASTFile()                      = %d\n", pDecl->isFromASTFile());
    printf("pDecl->isFunctionOrFunctionTemplate()       = %d\n", pDecl->isFunctionOrFunctionTemplate());
    printf("pDecl->isFunctionOrMethod()                 = %d\n", pDecl->isFunctionOrMethod());
    printf("pDecl->isHidden()                           = %d\n", pDecl->isHidden());
    printf("pDecl->isImplicit()                         = %d\n", pDecl->isImplicit());
    printf("pDecl->isInAnonymousNamespace()             = %d\n", pDecl->isInAnonymousNamespace());
    // printf("pDecl->isInIdentifierNamespace()            = %d\n", pDecl->isInIdentifierNamespace());
    printf("pDecl->isInjectedClassName()                = %d\n", pDecl->isInjectedClassName());
    printf("pDecl->isInlineNamespace()                  = %d\n", pDecl->isInlineNamespace());
    printf("pDecl->isInStdNamespace()                   = %d\n", pDecl->isInStdNamespace());
    printf("pDecl->isInterface()                        = %d\n", pDecl->isInterface());
    printf("pDecl->isInvalidDecl()                      = %d\n", pDecl->isInvalidDecl());
    printf("pDecl->isLambda()                           = %d\n", pDecl->isLambda());
    printf("pDecl->isLexicallyWithinFunctionOrMethod()  = %d\n", pDecl->isLexicallyWithinFunctionOrMethod());
    printf("pDecl->isLinkageValid()                     = %d\n", pDecl->isLinkageValid());
    printf("pDecl->isLocalExternDecl()                  = %d\n", pDecl->isLocalExternDecl());
    printf("pDecl->isLookupContext()                    = %d\n", pDecl->isLookupContext());
    printf("pDecl->isModulePrivate()                    = %d\n", pDecl->isModulePrivate());
    // printf("pDecl->isMsStruct()                         = %d\n", pDecl->isMsStruct());
    printf("pDecl->isCompleteDefinitisNamespaceion()    = %d\n", pDecl->isNamespace());
    printf("pDecl->isNonTrivialToPrimitiveCopy()        = %d\n", pDecl->isNonTrivialToPrimitiveCopy());
    printf("pDecl->isNonTrivialToPrimitiveDefaultInit() = %d\n", pDecl->isNonTrivialToPrimitiveDefaultInitialize());
    printf("pDecl->isNonTrivialToPrimitiveDestroy()     = %d\n", pDecl->isNonTrivialToPrimitiveDestroy());
    printf("pDecl->isObjCContainer()                    = %d\n", pDecl->isObjCContainer());
    printf("pDecl->isOutOfLine()                        = %d\n", pDecl->isOutOfLine());
    printf("pDecl->isParamDestroyedInCallee()           = %d\n", pDecl->isParamDestroyedInCallee());
    printf("pDecl->isParameterPack()                    = %d\n", pDecl->isParameterPack());
    printf("pDecl->isRecord()                           = %d\n", pDecl->isRecord());
    printf("pDecl->isReferenced()                       = %d\n", pDecl->isReferenced());
    printf("pDecl->isStdNamespace()                     = %d\n", pDecl->isStdNamespace());
    printf("pDecl->isStruct()                           = %d\n", pDecl->isStruct());
    // printf("pDecl->isTagIdentifierNamespace()           = %d\n", pDecl->isTagIdentifierNamespace());
    printf("pDecl->isTemplated()                        = %d\n", pDecl->isTemplated());
    printf("pDecl->isTemplateDecl()                     = %d\n", pDecl->isTemplateDecl());
    printf("pDecl->isTemplateParameter()                = %d\n", pDecl->isTemplateParameter());
    printf("pDecl->isTemplateParameterPack()            = %d\n", pDecl->isTemplateParameterPack());
    printf("pDecl->isThisDeclarationADefinition()       = %d\n", pDecl->isThisDeclarationADefinition());
    printf("pDecl->isThisDeclarationReferenced()        = %d\n", pDecl->isThisDeclarationReferenced());
    printf("pDecl->isTopLevelDeclInObjCContainer()      = %d\n", pDecl->isTopLevelDeclInObjCContainer());
    printf("pDecl->isTranslationUnit()                  = %d\n", pDecl->isTranslationUnit());
    printf("pDecl->isTransparentContext()               = %d\n", pDecl->isTransparentContext());
    printf("pDecl->isUnavailable()                      = %d\n", pDecl->isUnavailable());
    printf("pDecl->isCompleteDefiniisUniontion()        = %d\n", pDecl->isUnion());
    printf("pDecl->isUsed()                             = %d\n", pDecl->isUsed());
    printf("pDecl->isWeakImported()                     = %d\n", pDecl->isWeakImported());
    printf("---------------------------------------------------------------------------------\n");
    printf("pDecl->isCompleteDefinition()               = %d\n", pDecl->canPassInRegisters());
    printf("pDecl->isCompleteDefinition()               = %d\n", pDecl->mayInsertExtraPadding());
    printf("---------------------------------------------------------------------------------\n");

    // clang-format ON
    return true;
}

bool MyASTVisitor::VisitVarDecl(VarDecl *pDecl) {
  if (!this->m_pConfig->General.Options.bCheckVariableName) {
    return true;
  }

  APP_CONTEXT *pAppCxt = ((APP_CONTEXT *)GetAppCxt());
  if (!pAppCxt) {
    assert(pAppCxt);
    return false;
  }

  string VarType;
  string VarName;

  bool bIsPtr = false;
  bool bIsArray = false;
  bool bIsBuiltinType = false;

  bool bStauts = this->_GetVarInfo(pDecl, VarType, VarName, bIsPtr, bIsArray,
                                   bIsBuiltinType);
  if (bStauts) {
    bool bResult = this->m_Detect.CheckVariable(
        this->m_pConfig->General.Rules.VariableName, VarType, VarName,
        this->m_pConfig->Hungarian.Others.PreferUpperCamelIfMissed, bIsPtr,
        bIsArray);

    pAppCxt->TraceMemo.Checked.nVariable++;
    if (!bResult) {
      pAppCxt->TraceMemo.Error.nParameter++;

      pAppCxt->TraceMemo.ErrorDetailList.push_back(
          this->_CreateErrorDetail(pDecl, CheckType::CT_Variable, bIsPtr,
                                   bIsArray, VarType, VarName, ""));
    }
  }

  return bStauts;
}

bool MyASTVisitor::VisitReturnStmt(ReturnStmt *pRetStmt) {
  assert(pRetStmt);

  const Expr *pExpr = pRetStmt->getRetValue();
  if (pExpr) {
    clang::QualType MyQualType = pExpr->getType();
    std::string MyTypeStr = MyQualType.getAsString();
    return true;
  }

  return false;
}
