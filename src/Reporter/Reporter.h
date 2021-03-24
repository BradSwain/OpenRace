/* Copyright 2021 Coderrect Inc. All Rights Reserved.
Licensed under the GNU Affero General Public License, version 3 or later (“AGPL”), as published by the Free Software
Foundation. You may not use this file except in compliance with the License. You may obtain a copy of the License at
https://www.gnu.org/licenses/agpl-3.0.en.html
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an “AS IS” BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#pragma once

#include <optional>

#include "Trace/ProgramTrace.h"

namespace race {

struct SourceLoc {
  llvm::StringRef filename;
  unsigned int line;
  unsigned int col;

  SourceLoc() = delete;
  SourceLoc(llvm::StringRef filename, unsigned int line, unsigned int col) : filename(filename), line(line), col(col) {}
  explicit SourceLoc(const llvm::DILocation *loc)
      : filename(loc->getFilename()), line(loc->getLine()), col(loc->getColumn()) {}

  inline bool operator==(const SourceLoc &other) const {
    return filename == other.filename && line == other.line && col == other.col;
  }
  inline bool operator!=(const SourceLoc &other) const { return !(*this == other); }

  bool operator<(const SourceLoc &other) const;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const SourceLoc &loc);

struct RaceAccess {
  // source location cannot always be identified
  std::optional<SourceLoc> location;
  const llvm::Instruction *inst;

  RaceAccess(const MemAccessEvent *event);

  bool sameLocation(const RaceAccess &other) const { return location == other.location; }

  bool operator==(const RaceAccess &other) const;
  bool operator!=(const RaceAccess &other) const;
  bool operator<(const RaceAccess &other) const;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const std::optional<SourceLoc> &location);
llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const RaceAccess &acc);

struct Race {
 public:
  RaceAccess first;
  RaceAccess second;
  Race(RaceAccess first, RaceAccess second) : first(first), second(second) {
    if (second < first) std::swap(first, second);
  }

  // Check if either access is missing source location
  bool missingLocation() const { return !first.location.has_value() || !second.location.has_value(); }

  inline bool operator==(const Race &other) const { return first == other.first && second == other.second; }
  inline bool operator<(const Race &other) const {
    if (first != other.first) return first < other.first;
    return second < other.second;
  }
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Race &race);

using Report = std::set<Race>;
class Reporter {
  std::vector<std::pair<const WriteEvent *, const MemAccessEvent *>> races;

 public:
  void collect(const WriteEvent *e1, const MemAccessEvent *e2);

  [[nodiscard]] Report getReport() const;
};

}  // namespace race