//===-- sanitizer_suppressions.cc -----------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Suppression parsing/matching code shared between TSan and LSan.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_suppressions.h"

#include "sanitizer_allocator_internal.h"
#include "sanitizer_common.h"
#include "sanitizer_flags.h"
#include "sanitizer_libc.h"
#include "sanitizer_placement_new.h"

namespace __sanitizer {

static const char *const kTypeStrings[SuppressionTypeCount] = {
    "none", "race", "mutex", "thread", "signal", "leak", "called_from_lib",
    "deadlock", "vptr_check", "interceptor_name", "interceptor_via_fun",
    "interceptor_via_lib"};

bool TemplateMatch(char *templ, const char *str) {
  if (str == 0 || str[0] == 0)
    return false;
  bool start = false;
  if (templ && templ[0] == '^') {
    start = true;
    templ++;
  }
  bool asterisk = false;
  while (templ && templ[0]) {
    if (templ[0] == '*') {
      templ++;
      start = false;
      asterisk = true;
      continue;
    }
    if (templ[0] == '$')
      return str[0] == 0 || asterisk;
    if (str[0] == 0)
      return false;
    char *tpos = (char*)internal_strchr(templ, '*');
    char *tpos1 = (char*)internal_strchr(templ, '$');
    if (tpos == 0 || (tpos1 && tpos1 < tpos))
      tpos = tpos1;
    if (tpos != 0)
      tpos[0] = 0;
    const char *str0 = str;
    const char *spos = internal_strstr(str, templ);
    str = spos + internal_strlen(templ);
    templ = tpos;
    if (tpos)
      tpos[0] = tpos == tpos1 ? '$' : '*';
    if (spos == 0)
      return false;
    if (start && spos != str0)
      return false;
    start = false;
    asterisk = false;
  }
  return true;
}

ALIGNED(64) static char placeholder[sizeof(SuppressionContext)];
static SuppressionContext *suppression_ctx = 0;

SuppressionContext::SuppressionContext() : suppressions_(1), can_parse_(true) {
  internal_memset(has_suppresson_type_, 0, sizeof(has_suppresson_type_));
}

SuppressionContext *SuppressionContext::Get() {
  CHECK(suppression_ctx);
  return suppression_ctx;
}

static bool GetPathAssumingFileIsRelativeToExec(const char *file_path,
                                                /*out*/char *new_file_path,
                                                uptr new_file_path_size) {
  InternalScopedString exec(kMaxPathLength);
  if (ReadBinaryName(exec.data(), exec.size())) {
    const char *file_name_pos = StripModuleName(exec.data());
    uptr path_to_exec_len = file_name_pos - exec.data();
    internal_strncat(new_file_path, exec.data(),
                     Min(path_to_exec_len, new_file_path_size - 1));
    internal_strncat(new_file_path, file_path,
                     new_file_path_size - internal_strlen(new_file_path) - 1);
    return true;
  }
  return false;
}

void SuppressionContext::InitIfNecessary() {
  if (suppression_ctx)
    return;
  suppression_ctx = new(placeholder) SuppressionContext;
  if (common_flags()->suppressions[0] == '\0')
    return;

  // If we cannot find the file, check if its location is relative to
  // the location of the executable.
  const char *filename = common_flags()->suppressions;
  InternalScopedString new_file_path(kMaxPathLength);
  if (!FileExists(filename) && !IsAbsolutePath(filename) &&
      GetPathAssumingFileIsRelativeToExec(filename, new_file_path.data(),
                                          new_file_path.size())) {
    filename = new_file_path.data();
  }

  // Read the file.
  char *suppressions_from_file;
  uptr buffer_size;
  const uptr max_len = 1 << 26;
  uptr contents_size =
    ReadFileToBuffer(filename, &suppressions_from_file,
                     &buffer_size, max_len);
  VPrintf(1, "%s: reading suppressions file at %s\n",
          SanitizerToolName, filename);

  if (contents_size == 0) {
    Printf("%s: failed to read suppressions file '%s'\n", SanitizerToolName,
           common_flags()->suppressions);
    Die();
  }

  suppression_ctx->Parse(suppressions_from_file);
}

bool SuppressionContext::Match(const char *str, SuppressionType type,
                               Suppression **s) {
  if (!has_suppresson_type_[type])
    return false;
  can_parse_ = false;
  uptr i;
  for (i = 0; i < suppressions_.size(); i++)
    if (type == suppressions_[i].type &&
        TemplateMatch(suppressions_[i].templ, str))
      break;
  if (i == suppressions_.size()) return false;
  *s = &suppressions_[i];
  return true;
}

static const char *StripPrefix(const char *str, const char *prefix) {
  while (str && *str == *prefix) {
    str++;
    prefix++;
  }
  if (!*prefix)
    return str;
  return 0;
}

void SuppressionContext::Parse(const char *str) {
  // Context must not mutate once Match has been called.
  CHECK(can_parse_);
  const char *line = str;
  while (line) {
    while (line[0] == ' ' || line[0] == '\t')
      line++;
    const char *end = internal_strchr(line, '\n');
    if (end == 0)
      end = line + internal_strlen(line);
    if (line != end && line[0] != '#') {
      const char *end2 = end;
      while (line != end2 && (end2[-1] == ' ' || end2[-1] == '\t'))
        end2--;
      int type;
      for (type = 0; type < SuppressionTypeCount; type++) {
        const char *next_char = StripPrefix(line, kTypeStrings[type]);
        if (next_char && *next_char == ':') {
          line = ++next_char;
          break;
        }
      }
      if (type == SuppressionTypeCount) {
        Printf("%s: failed to parse suppressions\n", SanitizerToolName);
        Die();
      }
      Suppression s;
      s.type = static_cast<SuppressionType>(type);
      s.templ = (char*)InternalAlloc(end2 - line + 1);
      internal_memcpy(s.templ, line, end2 - line);
      s.templ[end2 - line] = 0;
      s.hit_count = 0;
      s.weight = 0;
      suppressions_.push_back(s);
      has_suppresson_type_[s.type] = true;
    }
    if (end[0] == 0)
      break;
    line = end + 1;
  }
}

uptr SuppressionContext::SuppressionCount() const {
  return suppressions_.size();
}

bool SuppressionContext::HasSuppressionType(SuppressionType type) const {
  return has_suppresson_type_[type];
}

const Suppression *SuppressionContext::SuppressionAt(uptr i) const {
  CHECK_LT(i, suppressions_.size());
  return &suppressions_[i];
}

void SuppressionContext::GetMatched(
    InternalMmapVector<Suppression *> *matched) {
  for (uptr i = 0; i < suppressions_.size(); i++)
    if (suppressions_[i].hit_count)
      matched->push_back(&suppressions_[i]);
}

const char *SuppressionTypeString(SuppressionType t) {
  CHECK(t < SuppressionTypeCount);
  return kTypeStrings[t];
}

}  // namespace __sanitizer
