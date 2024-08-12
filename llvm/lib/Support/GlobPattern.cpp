//===-- GlobPattern.cpp - Glob pattern matcher implementation -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements a glob pattern matcher.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/GlobPattern.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Errc.h"

using namespace llvm;

// Expands character ranges and returns a bitmap.
// For example, "a-cf-hz" is expanded to "abcfghz".
static Expected<BitVector> expand(StringRef S, StringRef Original) {
  BitVector BV(256, false);

  // Expand X-Y.
  for (;;) {
    if (S.size() < 3)
      break;

    uint8_t Start = S[0];
    uint8_t End = S[2];

    // If it doesn't start with something like X-Y,
    // consume the first character and proceed.
    if (S[1] != '-') {
      BV[Start] = true;
      S = S.substr(1);
      continue;
    }

    // It must be in the form of X-Y.
    // Validate it and then interpret the range.
    if (Start > End)
      return make_error<StringError>("invalid glob pattern: " + Original,
                                     errc::invalid_argument);

    for (int C = Start; C <= End; ++C)
      BV[(uint8_t)C] = true;
    S = S.substr(3);
  }

  for (char C : S)
    BV[(uint8_t)C] = true;
  return BV;
}

/// There are sub patterns and then there are inverted sub patterns. If the
/// string being matched against contains an inverted sub pattern then the
/// match will return false.
using MaybeInvertedPatternTy = std::pair<std::string, bool>;

// Identify brace expansions in S and return the list of patterns they expand
// into.
static Expected<SmallVector<MaybeInvertedPatternTy, 1>>
parseBraceExpansions(StringRef S, std::optional<size_t> MaxSubPatterns) {
  SmallVector<MaybeInvertedPatternTy, 1> SubPatterns = {
      std::make_pair(S.str(), false)};

  if (!MaxSubPatterns || !S.contains('{')) {
    return std::move(SubPatterns);
  }

  struct BraceExpansion {
    size_t Start;
    size_t Length;
    SmallVector<StringRef, 2> Terms;
    bool Inverted = false;
  };
  SmallVector<BraceExpansion, 0> BraceExpansions;

  BraceExpansion *CurrentBE = nullptr;
  size_t TermBegin;
  for (size_t I = 0, E = S.size(); I != E; ++I) {
    if (S[I] == '[') {
      I = S.find(']', I + 2);
      if (I == std::string::npos)
        return make_error<StringError>("invalid glob pattern, unmatched '['",
                                       errc::invalid_argument);
    } else if (S[I] == '{') {
      if (CurrentBE)
        return make_error<StringError>(
            "nested brace expansions are not supported",
            errc::invalid_argument);
      CurrentBE = &BraceExpansions.emplace_back();
      CurrentBE->Start = I;
      TermBegin = I + 1;
    } else if (S[I] == ',') {
      if (!CurrentBE)
        continue;
      if (CurrentBE->Inverted && S[TermBegin] != '!')
        return make_error<StringError>(
            "all terms in an inverted match must start with '!'",
            errc::invalid_argument);
      CurrentBE->Terms.push_back(S.substr(TermBegin + CurrentBE->Inverted,
                                          I - TermBegin - CurrentBE->Inverted));
      TermBegin = I + 1;
    } else if (S[I] == '}') {
      if (!CurrentBE)
        continue;
      if (CurrentBE->Terms.empty() && !CurrentBE->Inverted)
        return make_error<StringError>(
            "empty or singleton brace expansions are not supported",
            errc::invalid_argument);
      if (CurrentBE->Inverted && S[TermBegin] != '!')
        return make_error<StringError>(
            "inverted matcher term must start with '!'",
            errc::invalid_argument);
      CurrentBE->Terms.push_back(S.substr(TermBegin + CurrentBE->Inverted,
                                          I - TermBegin - CurrentBE->Inverted));
      CurrentBE->Length = I - CurrentBE->Start + 1;
      if (CurrentBE->Inverted && CurrentBE->Length < 4)
        return make_error<StringError>("inverted matcher is missing a term",
                                       errc::invalid_argument);
      CurrentBE = nullptr;
    } else if (S[I] == '!') {
      if (!CurrentBE)
        continue;
      if (I != TermBegin)
        return make_error<StringError>(
            "must use '!' at the beginning of the term", errc::invalid_argument);
      if (CurrentBE->Terms.size() && !CurrentBE->Inverted)
        return make_error<StringError>(
            "cannot mix inverted match terms with normal terms",
            errc::invalid_argument);
      CurrentBE->Inverted = true;
    } else if (S[I] == '*') {
      if (!CurrentBE)
        continue;
      if (CurrentBE->Inverted)
        return make_error<StringError>(
            "cannot use a wildcard '*' inside an inverted match expression",
            errc::invalid_argument);
    } else if (S[I] == '\\') {
      if (++I == E)
        return make_error<StringError>("invalid glob pattern, stray '\\'",
                                       errc::invalid_argument);
    }
  }
  if (CurrentBE)
    return make_error<StringError>("incomplete brace expansion",
                                   errc::invalid_argument);

  size_t NumSubPatterns = 1;
  for (auto &BE : BraceExpansions) {
    if (NumSubPatterns > std::numeric_limits<size_t>::max() / BE.Terms.size()) {
      NumSubPatterns = std::numeric_limits<size_t>::max();
      break;
    }
    NumSubPatterns *= BE.Terms.size();
  }
  if (NumSubPatterns > *MaxSubPatterns)
    return make_error<StringError>("too many brace expansions",
                                   errc::invalid_argument);
  // Replace brace expansions in reverse order so that we don't invalidate
  // earlier start indices
  for (auto &BE : reverse(BraceExpansions)) {
    SmallVector<MaybeInvertedPatternTy> OrigSubPatterns;
    std::swap(SubPatterns, OrigSubPatterns);
    for (StringRef Term : BE.Terms) {
      for (MaybeInvertedPatternTy &SubPat : OrigSubPatterns) {
        SubPatterns.emplace_back(std::make_pair(SubPat.first, BE.Inverted))
            .first.replace(BE.Start, BE.Length, Term);
        if (BE.Inverted)
          SubPatterns.emplace_back(std::make_pair(SubPat.first, false))
              .first.replace(BE.Start, BE.Length, "*");
      }
    }
  }
  return std::move(SubPatterns);
}

Expected<GlobPattern>
GlobPattern::create(StringRef S, std::optional<size_t> MaxSubPatterns) {
  GlobPattern Pat;

  // Store the prefix that does not contain any metacharacter.
  size_t PrefixSize = S.find_first_of("?*[{\\");
  Pat.Prefix = S.substr(0, PrefixSize);
  if (PrefixSize == std::string::npos)
    return Pat;
  S = S.substr(PrefixSize);

  SmallVector<MaybeInvertedPatternTy, 1> SubPats;
  if (auto Err = parseBraceExpansions(S, MaxSubPatterns).moveInto(SubPats))
    return std::move(Err);

  for (auto [SubPat, Inverted] : SubPats) {
    auto SubGlobOrErr = SubGlobPattern::create(SubPat, Inverted);
    if (!SubGlobOrErr)
      return SubGlobOrErr.takeError();
    if (Inverted)
      Pat.InvertedSubGlobs.push_back(*SubGlobOrErr);
    else
      Pat.SubGlobs.push_back(*SubGlobOrErr);
  }

  return Pat;
}

Expected<GlobPattern::SubGlobPattern>
GlobPattern::SubGlobPattern::create(StringRef S, bool Inverted) {
  SubGlobPattern Pat;
  Pat.Inverted = Inverted;

  // Parse brackets.
  Pat.Pat.assign(S.begin(), S.end());
  for (size_t I = 0, E = S.size(); I != E; ++I) {
    if (S[I] == '[') {
      // ']' is allowed as the first character of a character class. '[]' is
      // invalid. So, just skip the first character.
      ++I;
      size_t J = S.find(']', I + 1);
      if (J == StringRef::npos)
        return make_error<StringError>("invalid glob pattern, unmatched '['",
                                       errc::invalid_argument);
      StringRef Chars = S.substr(I, J - I);
      bool Invert = S[I] == '^' || S[I] == '!';
      Expected<BitVector> BV =
          Invert ? expand(Chars.substr(1), S) : expand(Chars, S);
      if (!BV)
        return BV.takeError();
      if (Invert)
        BV->flip();
      Pat.Brackets.push_back(Bracket{J + 1, std::move(*BV)});
      I = J;
    } else if (S[I] == '\\') {
      if (++I == E)
        return make_error<StringError>("invalid glob pattern, stray '\\'",
                                       errc::invalid_argument);
    }
  }
  return Pat;
}

bool GlobPattern::match(StringRef S) const {
  if (!S.consume_front(Prefix))
    return false;
  if (SubGlobs.empty() && S.empty())
    return true;

  for (auto &Glob : InvertedSubGlobs)
    if (Glob.match(S))
      return false;

  for (auto &Glob : SubGlobs)
    if (Glob.match(S))
      return true;

  return false;
}

// Factor the pattern into segments split by '*'. The segment is matched
// sequentianlly by finding the first occurrence past the end of the previous
// match.
bool GlobPattern::SubGlobPattern::match(StringRef Str) const {
  const char *P = Pat.data(), *SegmentBegin = nullptr, *S = Str.data(),
             *SavedS = S;
  const char *const PEnd = P + Pat.size(), *const End = S + Str.size();
  size_t B = 0, SavedB = 0;
  while (S != End) {
    if (P == PEnd)
      ;
    else if (*P == '*') {
      // The non-* substring on the left of '*' matches the tail of S. Save the
      // positions to be used by backtracking if we see a mismatch later.
      SegmentBegin = ++P;
      SavedS = S;
      SavedB = B;
      continue;
    } else if (*P == '[') {
      if (Brackets[B].Bytes[uint8_t(*S)]) {
        P = Pat.data() + Brackets[B++].NextOffset;
        ++S;
        continue;
      }
    } else if (*P == '\\') {
      if (*++P == *S) {
        ++P;
        ++S;
        continue;
      }
    } else if (*P == *S || *P == '?') {
      ++P;
      ++S;
      continue;
    }
    if (!SegmentBegin)
      return false;
    // We have seen a '*'. Backtrack to the saved positions. Shift the S
    // position to probe the next starting position in the segment.
    P = SegmentBegin;
    S = ++SavedS;
    B = SavedB;
  }
  // All bytes in Str have been matched. Return true if the rest part of Pat is
  // empty or contains only '*'.
  return getPat().find_first_not_of('*', P - Pat.data()) == std::string::npos;
}
