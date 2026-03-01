#include "tor_onion_service.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"

namespace ipfs {

TorOnionService::TorOnionService(Config config)
    : config_(std::move(config)) {}

TorOnionService::~TorOnionService() {
  Stop();
}

void TorOnionService::Start(ReadyCallback callback) {
  if (running_) {
    LOG(WARNING) << "TorOnionService::Start called while already running";
    if (callback) {
      callback(true);
    }
    return;
  }

  // Ensure data directory exists.
  if (!base::CreateDirectory(config_.data_dir)) {
    LOG(ERROR) << "Failed to create Tor data directory: "
               << config_.data_dir.value();
    if (callback) {
      callback(false);
    }
    return;
  }

  // Ensure hidden service directory exists with correct permissions.
  auto hs_dir = hidden_service_dir();
  if (!base::CreateDirectory(hs_dir)) {
    LOG(ERROR) << "Failed to create hidden service directory: "
               << hs_dir.value();
    if (callback) {
      callback(false);
    }
    return;
  }

#if !BUILDFLAG(IS_WIN)
  // Tor requires the HiddenServiceDir to have mode 0700.
  if (chmod(hs_dir.value().c_str(), 0700) != 0) {
    LOG(ERROR) << "Failed to chmod hidden service directory";
    if (callback) {
      callback(false);
    }
    return;
  }
#endif

  // Write torrc.
  base::FilePath torrc_path = WriteTorrc();
  if (torrc_path.empty()) {
    if (callback) {
      callback(false);
    }
    return;
  }

  // Build command line.
  base::FilePath tor_bin = config_.tor_binary;
  if (tor_bin.empty()) {
    tor_bin = base::FilePath(FILE_PATH_LITERAL("tor"));
  }

  base::CommandLine cmd(tor_bin);
  cmd.AppendSwitchPath("-f", torrc_path);

  // Launch Tor.
  base::LaunchOptions options;
  options.start_hidden = true;
  tor_process_ = base::LaunchProcess(cmd, options);

  if (!tor_process_.IsValid()) {
    LOG(ERROR) << "Failed to launch Tor process";
    if (callback) {
      callback(false);
    }
    return;
  }

  LOG(INFO) << "Tor process launched (pid " << tor_process_.Pid() << ")";

  // Poll for the hostname file.  Tor takes a moment to bootstrap and create
  // the hidden service keys + hostname.
  constexpr int kMaxAttempts = 60;
  constexpr base::TimeDelta kPollInterval = base::Seconds(1);

  for (int i = 0; i < kMaxAttempts; ++i) {
    if (ReadOnionHostname()) {
      running_ = true;
      LOG(INFO) << "Onion service ready: " << onion_hostname_;
      if (callback) {
        callback(true);
      }
      return;
    }
    base::PlatformThread::Sleep(kPollInterval);
  }

  LOG(ERROR) << "Timed out waiting for Tor to produce a hostname file";
  Stop();
  if (callback) {
    callback(false);
  }
}

void TorOnionService::Stop() {
  if (tor_process_.IsValid()) {
    LOG(INFO) << "Stopping Tor process (pid " << tor_process_.Pid() << ")";
    tor_process_.Terminate(/*exit_code=*/0, /*wait=*/true);
    tor_process_.Close();
  }
  running_ = false;
  onion_hostname_.clear();
}

bool TorOnionService::is_running() const {
  return running_;
}

std::string_view TorOnionService::onion_hostname() const {
  return onion_hostname_;
}

uint16_t TorOnionService::socks_port() const {
  return config_.socks_port;
}

base::FilePath TorOnionService::hidden_service_dir() const {
  return config_.data_dir.Append(FILE_PATH_LITERAL("hidden_service"));
}

base::FilePath TorOnionService::WriteTorrc() {
  auto hs_dir = hidden_service_dir();

  std::string torrc;
  torrc += "DataDirectory " + config_.data_dir.value() + "\n";
  torrc += "SocksPort " + base::NumberToString(config_.socks_port) + "\n";
  torrc += "HiddenServiceDir " + hs_dir.value() + "\n";
  torrc += "HiddenServicePort " +
           base::NumberToString(config_.virtual_port) + " " +
           config_.target_addr + ":" +
           base::NumberToString(config_.target_port) + "\n";
  // Reduce Tor log noise by default.
  torrc += "Log notice file " +
           config_.data_dir.Append(FILE_PATH_LITERAL("tor.log")).value() +
           "\n";

  base::FilePath torrc_path =
      config_.data_dir.Append(FILE_PATH_LITERAL("torrc"));

  if (!base::WriteFile(torrc_path, torrc)) {
    LOG(ERROR) << "Failed to write torrc to " << torrc_path.value();
    return {};
  }

  LOG(INFO) << "Wrote torrc: " << torrc_path.value();
  return torrc_path;
}

bool TorOnionService::ReadOnionHostname() {
  base::FilePath hostname_path =
      hidden_service_dir().Append(FILE_PATH_LITERAL("hostname"));

  std::string contents;
  if (!base::ReadFileToString(hostname_path, &contents)) {
    return false;
  }

  onion_hostname_ = base::TrimWhitespaceASCII(contents, base::TRIM_ALL);
  return !onion_hostname_.empty();
}

}  // namespace ipfs
