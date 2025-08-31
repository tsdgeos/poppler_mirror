/*
 * goostring-format-checker.cc
 *
 * This file is licensed under the GPLv2 or later
 *
 * Clang++ compiler plugin that checks usage of GooString::format-like functions
 *
 * Copyright (C) 2014 Fabio D'Urso <fabiodurso@hotmail.it>
 * Copyright (C) 2021, 2025 Albert Astals Cid <aacid@kde.org>
 */

#include <cctype>

#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Attr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>

using namespace clang;

namespace {

class GooStringFormatCheckerVisitor : public RecursiveASTVisitor<GooStringFormatCheckerVisitor>
{
public:
    explicit GooStringFormatCheckerVisitor(CompilerInstance *compInst);

    bool VisitFunctionDecl(FunctionDecl *funcDecl);
    bool VisitCallExpr(CallExpr *callExpr);

private:
    /* Returns the index of the format argument, or -1 if the function must
     * not be checked */
    int findFormatArgumentIndex(const FunctionDecl *funcDecl) const;

    /* Returns the SourceLocation of the n-th character */
    SourceLocation getLocationOfCharacter(const StringLiteral *strLiteral, unsigned n);

    /* Validates usage of a placeholder and returns the corresponding
     * argument index, or -1 in case of errors */
    int verifyPlaceholder(const CallExpr *callExpr, const SourceLocation &placeholderLocation, std::string &placeholderText, int baseArgIdx) const;

    CompilerInstance *compInst;
    DiagnosticsEngine *diag;
    unsigned diag_badFuncZeroArgs;
    unsigned diag_badFuncNonVariadic;
    unsigned diag_badFuncLastArgInvalidType;
    unsigned diag_notStringLiteral;
    unsigned diag_notPlainASCII;
    unsigned diag_wrongOrder;
    unsigned diag_unescapedBracket;
    unsigned diag_unterminatedPlaceholder;
    unsigned diag_unconsumedArgs;
    unsigned diag_missingColon;
    unsigned diag_missingArgNumber;
    unsigned diag_badArgNumber;
    unsigned diag_argumentNotPresent;
    unsigned diag_badPrecision;
    unsigned diag_badType;
    unsigned diag_wrongArgExprType;
};

GooStringFormatCheckerVisitor::GooStringFormatCheckerVisitor(CompilerInstance *compInst) : compInst(compInst)
{
    diag = &compInst->getDiagnostics();

    diag_badFuncZeroArgs = diag->getCustomDiagID(DiagnosticsEngine::Error, "Cannot enforce format string checks on a function that takes no arguments");
    diag_badFuncNonVariadic = diag->getCustomDiagID(DiagnosticsEngine::Error, "Cannot enforce format string checks on a non-variadic function");
    diag_badFuncLastArgInvalidType = diag->getCustomDiagID(DiagnosticsEngine::Error, "Cannot enforce format string checks if the last non-variadic argument is not const char *");
    diag_notStringLiteral = diag->getCustomDiagID(DiagnosticsEngine::Error, "Format string is not a string literal. Skipping format checks");
    diag_notPlainASCII = diag->getCustomDiagID(DiagnosticsEngine::Error, "Format string contains non-ASCII or NUL characters. Skipping format checks");
    diag_wrongOrder = diag->getCustomDiagID(DiagnosticsEngine::Error, "Argument %0 must be consumed before argument %1");
    diag_unescapedBracket = diag->getCustomDiagID(DiagnosticsEngine::Error, "Unescaped '}' character");
    diag_unterminatedPlaceholder = diag->getCustomDiagID(DiagnosticsEngine::Error, "Unterminated placeholder");
    diag_unconsumedArgs = diag->getCustomDiagID(DiagnosticsEngine::Error, "Unconsumed argument(s)");
    diag_missingColon = diag->getCustomDiagID(DiagnosticsEngine::Error, "Invalid placeholder '{%0}': missing colon character");
    diag_missingArgNumber = diag->getCustomDiagID(DiagnosticsEngine::Error, "Invalid placeholder '{%0}': missing <arg> number");
    diag_badArgNumber = diag->getCustomDiagID(DiagnosticsEngine::Error, "Invalid placeholder '{%0}': bad <arg> number");
    diag_argumentNotPresent = diag->getCustomDiagID(DiagnosticsEngine::Error, "Argument for placeholder '{%0}' is not present");
    diag_badPrecision = diag->getCustomDiagID(DiagnosticsEngine::Error, "Invalid placeholder '{%0}': bad <precision> value");
    diag_badType = diag->getCustomDiagID(DiagnosticsEngine::Error, "Invalid placeholder '{%0}': bad <type> specifier");
    diag_wrongArgExprType = diag->getCustomDiagID(DiagnosticsEngine::Error, "Expected %0 for placeholder '{%1}', found %2");
}

bool GooStringFormatCheckerVisitor::VisitFunctionDecl(FunctionDecl *funcDecl)
{
    findFormatArgumentIndex(funcDecl); // Spot misuse of the "gooformat" annotation
    return true;
}

bool GooStringFormatCheckerVisitor::VisitCallExpr(CallExpr *callExpr)
{
    /*** Locate format argument or skip calls that needn't be checked ***/

    const int formatArgIdx = findFormatArgumentIndex(callExpr->getDirectCallee());
    if (formatArgIdx == -1)
        return true;

    /*** Obtain format string value ***/

    const Expr *formatArgExpr = callExpr->getArg(formatArgIdx);
    while (formatArgExpr->getStmtClass() == Stmt::ImplicitCastExprClass) {
        formatArgExpr = static_cast<const ImplicitCastExpr *>(formatArgExpr)->getSubExpr();
    }
    if (formatArgExpr->getStmtClass() != Stmt::StringLiteralClass) {
        diag->Report(formatArgExpr->getExprLoc(), diag_notStringLiteral);
        return true;
    }
    const StringLiteral *formatArgStrLiteral = static_cast<const StringLiteral *>(formatArgExpr);
    if (formatArgStrLiteral->containsNonAsciiOrNull()) {
        diag->Report(formatArgExpr->getExprLoc(), diag_notPlainASCII);
        return true;
    }

    /*** Parse format string and verify arguments ***/

    const std::string format = formatArgStrLiteral->getString().str();

    /* Keeps track of whether we are currently parsing a character contained
     * within '{' ... '}'. If set, current_placeholder contains the contents
     * parsed so far (without brackets) */
    bool in_placeholder = false;
    std::string current_placeholder;

    // Source location of the current placeholder's opening bracket
    SourceLocation placeholderLoc;

    /* Keeps track of the next expected argument number, to check that
     * arguments are first consumed in order (eg {0:d}{2:d}{1:d} is wrong).
     * Note that it's possible to "look back" at already consumed
     * arguments (eg {0:d}{1:d}{0:d} is OK) */
    int nextExpectedArgNum = 0;

    for (unsigned i = 0; i < format.length(); i++) {
        if (in_placeholder) {
            // Have we reached the end of the placeholder?
            if (format[i] == '}') {
                in_placeholder = false;

                // Verifies the placeholder and returns the argument number
                const int foundArgNum = verifyPlaceholder(callExpr, placeholderLoc, current_placeholder, formatArgIdx + 1);

                // If the placeholder wasn't valid, disable argument order checks
                if (foundArgNum == -1) {
                    nextExpectedArgNum = -1;
                }

                // If argument order checks are enabled, let's check!
                if (nextExpectedArgNum != -1) {
                    if (foundArgNum == nextExpectedArgNum) {
                        nextExpectedArgNum++;
                    } else if (foundArgNum > nextExpectedArgNum) {
                        diag->Report(placeholderLoc, diag_wrongOrder) << nextExpectedArgNum << foundArgNum;
                        nextExpectedArgNum = -1; // disable further checks
                    }
                }
            } else {
                current_placeholder += format[i];
            }
        } else if (format[i] == '{') {
            // If we find a '{' then a placeholder is starting...
            in_placeholder = true;
            current_placeholder = "";
            placeholderLoc = getLocationOfCharacter(formatArgStrLiteral, i);

            // ...unless it's followed by another '{' (escape sequence)
            if (i + 1 < format.length() && format[i + 1] == '{') {
                i++; // skip next '{' character
                in_placeholder = false;
            }
        } else if (format[i] == '}') {
            /* If we have found a '}' and we're not in a placeholder,
             * then it *MUST* be followed by another '}' (escape sequence) */
            if (i + 1 >= format.length() || format[i + 1] != '}') {
                diag->Report(getLocationOfCharacter(formatArgStrLiteral, i), diag_unescapedBracket);
            } else {
                i++; // skip next '}' character
            }
        }
    }

    /* If we've reached the end of the format string and in_placeholder is
     * still set, then the last placeholder wasn't terminated properly */
    if (in_placeholder)
        diag->Report(placeholderLoc, diag_unterminatedPlaceholder);

    int unconsumedArgs = callExpr->getNumArgs() - (formatArgIdx + 1 + nextExpectedArgNum);
    if (unconsumedArgs > 0)
        diag->Report(callExpr->getArg(callExpr->getNumArgs() - unconsumedArgs)->getExprLoc(), diag_unconsumedArgs);

    return true;
}

int GooStringFormatCheckerVisitor::findFormatArgumentIndex(const FunctionDecl *funcDecl) const
{
    if (!funcDecl)
        return -1;

    AnnotateAttr *annotation = NULL;
    for (specific_attr_iterator<AnnotateAttr> it = funcDecl->specific_attr_begin<AnnotateAttr>(); it != funcDecl->specific_attr_end<AnnotateAttr>() && !annotation; ++it) {
        if (it->getAnnotation() == "gooformat")
            annotation = *it;
    }

    // If this function hasn't got the "gooformat" annotation on it
    if (!annotation)
        return -1;

    if (funcDecl->getNumParams() == 0) {
        diag->Report(annotation->getLocation(), diag_badFuncZeroArgs);
        return -1;
    }

    if (!funcDecl->isVariadic()) {
        diag->Report(annotation->getLocation(), diag_badFuncNonVariadic);
        return -1;
    }

    // Assume the last non-variadic argument is the format specifier
    const int formatArgIdx = funcDecl->getNumParams() - 1;
    const QualType formatArgType = funcDecl->getParamDecl(formatArgIdx)->getType();
    if (formatArgType.getAsString() != "const char *") {
        diag->Report(annotation->getLocation(), diag_badFuncLastArgInvalidType);
        return -1;
    }

    return formatArgIdx;
}

SourceLocation GooStringFormatCheckerVisitor::getLocationOfCharacter(const StringLiteral *strLiteral, unsigned n)
{
    return strLiteral->getLocationOfByte(n, compInst->getSourceManager(), compInst->getLangOpts(), compInst->getTarget());
}

int GooStringFormatCheckerVisitor::verifyPlaceholder(const CallExpr *callExpr, const SourceLocation &placeholderLocation, std::string &placeholderText, int baseArgIdx) const
{
    // Find the colon that separates the argument number and the format specifier
    const size_t delim = placeholderText.find(':');
    if (delim == std::string::npos) {
        diag->Report(placeholderLocation, diag_missingColon) << placeholderText;
        return -1;
    }
    if (delim == 0) {
        diag->Report(placeholderLocation, diag_missingArgNumber) << placeholderText;
        return -1;
    }
    for (unsigned int i = 0; i < delim; i++) {
        if (!isdigit(placeholderText[i])) {
            diag->Report(placeholderLocation, diag_badArgNumber) << placeholderText;
            return -1;
        }
    }

    // Extract argument number and its actual position in the call's argument list
    const int argNum = atoi(placeholderText.substr(0, delim).c_str());
    const int argIdx = baseArgIdx + argNum;
    if (argIdx >= callExpr->getNumArgs()) {
        diag->Report(placeholderLocation, diag_argumentNotPresent) << placeholderText;
        return argNum;
    }

    // Check and strip width/precision specifiers
    std::string format = placeholderText.substr(delim + 1);
    bool dot_found = false;
    while (isdigit(format[0]) || format[0] == '.') {
        if (format[0] == '.') {
            if (dot_found) {
                diag->Report(placeholderLocation, diag_badPrecision) << placeholderText;
                return argNum;
            }
            dot_found = true;
        }
        format = format.substr(1);
    }

    const Expr *argExpr = callExpr->getArg(argIdx);
    const QualType qualType = argExpr->getType();
    const Type *valueType = qualType->getUnqualifiedDesugaredType();

    if (format == "d" || format == "x" || format == "X" || format == "o" || format == "b" || format == "w") {
        if (!valueType->isSpecificBuiltinType(BuiltinType::Int)) {
            diag->Report(argExpr->getExprLoc(), diag_wrongArgExprType) << "int" << placeholderText << qualType.getAsString();
        }
    } else if (format == "ud" || format == "ux" || format == "uX" || format == "uo" || format == "ub") {
        if (!valueType->isSpecificBuiltinType(BuiltinType::UInt)) {
            diag->Report(argExpr->getExprLoc(), diag_wrongArgExprType) << "unsigned int" << placeholderText << qualType.getAsString();
        }
    } else if (format == "ld" || format == "lx" || format == "lX" || format == "lo" || format == "lb") {
        if (!valueType->isSpecificBuiltinType(BuiltinType::Long)) {
            diag->Report(argExpr->getExprLoc(), diag_wrongArgExprType) << "long" << placeholderText << qualType.getAsString();
        }
    } else if (format == "uld" || format == "ulx" || format == "ulX" || format == "ulo" || format == "ulb") {
        if (!valueType->isSpecificBuiltinType(BuiltinType::ULong)) {
            diag->Report(argExpr->getExprLoc(), diag_wrongArgExprType) << "unsigned long" << placeholderText << qualType.getAsString();
        }
    } else if (format == "lld" || format == "llx" || format == "llX" || format == "llo" || format == "llb") {
        if (!valueType->isSpecificBuiltinType(BuiltinType::LongLong)) {
            diag->Report(argExpr->getExprLoc(), diag_wrongArgExprType) << "long long" << placeholderText << qualType.getAsString();
        }
    } else if (format == "ulld" || format == "ullx" || format == "ullX" || format == "ullo" || format == "ullb") {
        if (!valueType->isSpecificBuiltinType(BuiltinType::ULongLong)) {
            diag->Report(argExpr->getExprLoc(), diag_wrongArgExprType) << "unsigned long long" << placeholderText << qualType.getAsString();
        }
    } else if (format == "f" || format == "g" || format == "gs") {
        if (!valueType->isSpecificBuiltinType(BuiltinType::Double)) {
            diag->Report(argExpr->getExprLoc(), diag_wrongArgExprType) << "float or double" << placeholderText << qualType.getAsString();
        }
    } else if (format == "c") {
        if (!valueType->isSpecificBuiltinType(BuiltinType::UInt) && !valueType->isSpecificBuiltinType(BuiltinType::Int)) {
            diag->Report(argExpr->getExprLoc(), diag_wrongArgExprType) << "char, short or int" << placeholderText << qualType.getAsString();
        }
    } else if (format == "s") {
        if (!valueType->isPointerType() || !valueType->getPointeeType()->getUnqualifiedDesugaredType()->isCharType()) {
            diag->Report(argExpr->getExprLoc(), diag_wrongArgExprType) << "char *" << placeholderText << qualType.getAsString();
        }
    } else if (format == "t") {
        const CXXRecordDecl *pointeeType = valueType->isPointerType() ? valueType->getPointeeType()->getAsCXXRecordDecl() : 0;
        if (pointeeType == 0 || pointeeType->getQualifiedNameAsString() != "GooString") {
            diag->Report(argExpr->getExprLoc(), diag_wrongArgExprType) << "GooString *" << placeholderText << qualType.getAsString();
        }
    } else {
        diag->Report(placeholderLocation, diag_badType) << placeholderText;
        return argNum;
    }

    return argNum;
}

class GooStringFormatCheckerConsumer : public clang::ASTConsumer
{
public:
    GooStringFormatCheckerConsumer(CompilerInstance *compInst) : visitor(compInst) { }

    virtual void HandleTranslationUnit(clang::ASTContext &ctx) { visitor.TraverseDecl(ctx.getTranslationUnitDecl()); }

private:
    GooStringFormatCheckerVisitor visitor;
};

class GooStringFormatCheckerAction : public PluginASTAction
{
protected:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &compInst, llvm::StringRef inFile) { return std::make_unique<GooStringFormatCheckerConsumer>(&compInst); }

    bool ParseArgs(const CompilerInstance &compInst, const std::vector<std::string> &args)
    {
        if (args.size() != 0) {
            DiagnosticsEngine &D = compInst.getDiagnostics();
            D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "goostring-format-checker takes no arguments"));
            return false;
        } else {
            return true;
        }
    }
};

}

static FrontendPluginRegistry::Add<GooStringFormatCheckerAction> X("goostring-format-checker", "Checks usage of GooString::format-like functions");
