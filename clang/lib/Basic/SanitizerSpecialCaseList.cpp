//===--- SanitizerSpecialCaseList.cpp - SCL for sanitizers ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// An extension of SpecialCaseList to allowing querying sections by
// SanitizerMask.
//
//===----------------------------------------------------------------------===//
#include "clang/Basic/SanitizerSpecialCaseList.h"

using namespace clang;

std::unique_ptr<SanitizerSpecialCaseList>
SanitizerSpecialCaseList::create(const std::vector<std::string> &Paths,
                                 llvm::vfs::FileSystem &VFS,
                                 std::string &Error) {
  std::unique_ptr<clang::SanitizerSpecialCaseList> SSCL(
      new SanitizerSpecialCaseList());
  if (SSCL->createInternal(Paths, VFS, Error)) {
    SSCL->createSanitizerSections();
    return SSCL;
  }
  return nullptr;
}

std::unique_ptr<SanitizerSpecialCaseList>
SanitizerSpecialCaseList::createOrDie(const std::vector<std::string> &Paths,
                                      llvm::vfs::FileSystem &VFS) {
  std::string Error;
  if (auto SSCL = create(Paths, VFS, Error))
    return SSCL;
  llvm::report_fatal_error(StringRef(Error));
}

void SanitizerSpecialCaseList::createSanitizerSections() {
  for (auto &It : Sections) {
    auto &S = It.second;
    SanitizerMask Mask;

#define SANITIZER(NAME, ID)                                                    \
  if (S.SectionMatcher->match(NAME))                                           \
    Mask |= SanitizerKind::ID;
#define SANITIZER_GROUP(NAME, ID, ALIAS) SANITIZER(NAME, ID)

#include "clang/Basic/Sanitizers.def"
#undef SANITIZER
#undef SANITIZER_GROUP

    SanitizerSections.emplace_back(Mask, S.Entries);
  }
}

bool SanitizerSpecialCaseList::inSection(SanitizerMask Mask, StringRef Prefix,
                                         StringRef Query,
                                         StringRef Category) const {
  for (auto &S : SanitizerSections)
    if ((S.Mask & Mask) &&
        SpecialCaseList::inSectionBlame(S.Entries, Prefix, Query, Category))
      return true;

  return false;
}

bool SanitizerSpecialCaseList::containsGlobal(SanitizerMask Mask, StringRef GlobalName,
                    StringRef Category) const {
  return inSection(Mask, "global", GlobalName, Category);
}

bool SanitizerSpecialCaseList::containsType(SanitizerMask Mask, StringRef MangledTypeName,
                  StringRef Category) const {
  return inSection(Mask, "type", MangledTypeName, Category);
}

bool SanitizerSpecialCaseList::containsFunction(SanitizerMask Mask, StringRef FunctionName) const {
  return inSection(Mask, "fun", FunctionName);
}

bool SanitizerSpecialCaseList::containsFile(SanitizerMask Mask, StringRef FileName,
                  StringRef Category) const {
  return inSection(Mask, "src", FileName, Category);
}

bool SanitizerSpecialCaseList::containsMainFile(SanitizerMask Mask, StringRef FileName,
                      StringRef Category) const {
  return inSection(Mask, "mainfile", FileName, Category);
}

bool SanitizerSpecialCaseList::containsLocation(SanitizerMask Mask,
                                                SourceLocation Loc,
                                                SourceManager &SM,
                                                StringRef Category) const {
  return Loc.isValid() &&
          containsFile(Mask, SM.getFilename(SM.getFileLoc(Loc)), Category);
}
