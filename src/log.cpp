#include "log.h"

#include <filesystem>
#include <iterator>
#include <print>

#include "sdl.h"
#include "time_.h"

using namespace anm2ed::util;

namespace anm2ed
{
  namespace
  {
    bool ensure_utf8_bom(const std::filesystem::path& path)
    {
      std::error_code ec{};
      if (!std::filesystem::exists(path, ec)) return false;

      std::ifstream existing(path, std::ios::binary);
      if (!existing) return false;

      char bom[3]{};
      existing.read(bom, sizeof(bom));
      auto bytesRead = existing.gcount();
      auto hasBom = bytesRead == sizeof(bom) && bom[0] == '\xEF' && bom[1] == '\xBB' && bom[2] == '\xBF';
      if (hasBom) return true;

      existing.clear();
      existing.seekg(0, std::ios::beg);
      std::string contents((std::istreambuf_iterator<char>(existing)), std::istreambuf_iterator<char>());
      existing.close();

      std::ofstream output(path, std::ios::binary | std::ios::trunc);
      if (!output) return false;
      output << "\xEF\xBB\xBF";
      output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
      output.close();

      return true;
    }
  }

  void Logger::write_raw(const std::string& message)
  {
    std::println("{}", message);
    if (file.is_open()) file << message << '\n' << std::flush;
  }

  void Logger::write(const Level level, const std::string& message)
  {
    std::string formatted = std::format("{} {} {}", time::get("(%d-%B-%Y %I:%M:%S)"), LEVEL_STRINGS[level], message);
    write_raw(formatted);
  }

  void Logger::info(const std::string& message) { write(INFO, message); }
  void Logger::warning(const std::string& message) { write(WARNING, message); }
  void Logger::error(const std::string& message) { write(ERROR, message); }
  void Logger::fatal(const std::string& message) { write(FATAL, message); }
  void Logger::command(const std::string& message) { write(COMMAND, message); }
  void Logger::open(const std::filesystem::path& path)
  {
    std::error_code ec{};
    auto exists = std::filesystem::exists(path, ec);
    if (exists) ensure_utf8_bom(path);

    file.open(path, std::ios::out | std::ios::app);
    if (!file.is_open()) return;

    if (!exists) file << "\xEF\xBB\xBF";
  }

  std::filesystem::path Logger::path() { return sdl::preferences_directory_get() / "log.txt"; }

  Logger::Logger()
  {
    open(path());
    info("Initializing Anm2Ed");
  }

  Logger::~Logger()
  {
    info("Exiting Anm2Ed");
    if (file.is_open()) file.close();
  }
}

anm2ed::Logger logger;
