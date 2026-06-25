#include <filesystem>

#include <CLI/CLI.hpp>
#include <fmt/core.h>

#include "common/types.hpp"
#include "storage/page_store.hpp"

auto main(int argc, char** argv) -> int {
  std::filesystem::path path{"demo.db"};
  int page_id = 0;

  CLI::App app{"Inspect the provided flat-file page store"};
  app.add_option("--file", path, "Path to the page-store file");
  app.add_option("--page", page_id, "Page id to read");
  CLI11_PARSE(app, argc, argv);

  im110::FlatFilePageStore store(path);
  im110::PageBytes bytes{};
  store.ReadPage(page_id, std::span<std::byte, im110::PAGE_SIZE>{bytes});

  fmt::print("Read page {} from {}\n", page_id, path.string());
  fmt::print("First 16 bytes:");
  for (std::size_t i = 0; i < 16; ++i) {
    fmt::print(" {:02x}", std::to_integer<unsigned int>(bytes[i]));
  }
  fmt::print("\n");
}
