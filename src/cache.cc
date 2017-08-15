#include "cache.h"

#include "indexer.h"
#include "platform.h"
#include "language_server_api.h"

#include <loguru/loguru.hpp>

#include <algorithm>

namespace {

std::string GetCachedBaseFileName(const std::string& cache_directory,
                                  std::string source_file) {
  assert(!cache_directory.empty());
  std::replace(source_file.begin(), source_file.end(), '\\', '_');
  std::replace(source_file.begin(), source_file.end(), '/', '_');
  std::replace(source_file.begin(), source_file.end(), ':', '_');

  return cache_directory + source_file;
}

}  // namespace

std::unique_ptr<IndexFile> LoadCachedIndex(Config* config,
                                           const std::string& filename) {
  if (!config->enableCacheRead)
    return nullptr;

  optional<std::string> file_content = ReadContent(
      GetCachedBaseFileName(config->cacheDirectory, filename) + ".json");
  if (!file_content)
    return nullptr;

  auto result = Deserialize(filename, *file_content, IndexFile::kCurrentVersion);
  result->is_loaded_from_cache_ = true;
  return result;
}

optional<std::string> LoadCachedFileContents(Config* config,
                                             const std::string& filename) {
  if (!config->enableCacheRead)
    return nullopt;

  return ReadContent(GetCachedBaseFileName(config->cacheDirectory, filename));
}

void WriteToCache(Config* config,
                  IndexFile& file) {
  if (!config->enableCacheWrite)
    return;

  std::string cache_basename =
      GetCachedBaseFileName(config->cacheDirectory, file.path);

  LOG_IF_S(ERROR, file.file_contents_.empty()) << "Writing " << file.path << " to cache but it has no contents";

  assert(!file.file_contents_.empty());
  std::ofstream cache_content;
  cache_content.open(cache_basename);
  assert(cache_content.good());
  cache_content << file.file_contents_;
  cache_content.close();

  std::string indexed_content = Serialize(file);
  std::ofstream cache;
  cache.open(cache_basename + ".json");
  assert(cache.good());
  cache << indexed_content;
  cache.close();
}
