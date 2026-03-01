#ifndef IPFS_TOR_ONION_SERVICE_H_
#define IPFS_TOR_ONION_SERVICE_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/process/process.h"
#include "base/sequence_checker.h"

namespace ipfs {

// Manages a Tor process and its associated onion (hidden) service.
//
// Lifecycle:
//   1. Construct with a data directory path.
//   2. Call Start() to write the torrc, launch the tor binary, and wait for
//      the .onion hostname to become available.
//   3. Use onion_hostname() to retrieve the v3 .onion address.
//   4. Use socks_port() to obtain the local SOCKS5 proxy port for routing
//      outbound connections through the Tor network.
//   5. Destruction (or Stop()) tears down the tor process.
//
// The onion service forwards traffic arriving on its virtual port to a
// configurable local target (address:port).
class TorOnionService {
 public:
  struct Config {
    // Directory used by Tor for its DataDirectory (keys, cached state, etc.).
    base::FilePath data_dir;

    // Port on which the Tor SOCKS5 proxy listens (default: 9050).
    uint16_t socks_port = 9050;

    // The virtual port exposed on the .onion address.
    uint16_t virtual_port = 80;

    // Target address:port that the hidden service forwards to.
    std::string target_addr = "127.0.0.1";
    uint16_t target_port = 8080;

    // Absolute path to the tor binary.  If empty, "tor" is resolved via PATH.
    base::FilePath tor_binary;
  };

  using ReadyCallback = std::function<void(bool success)>;

  explicit TorOnionService(Config config);
  ~TorOnionService();

  TorOnionService(const TorOnionService&) = delete;
  TorOnionService& operator=(const TorOnionService&) = delete;

  // Starts the Tor process and hidden service.  |callback| is invoked once
  // the .onion hostname is available (or on failure).
  void Start(ReadyCallback callback);

  // Gracefully terminates the running Tor process.
  void Stop();

  // Returns true when the Tor process is running and the .onion address has
  // been read.
  bool is_running() const;

  // The v3 .onion hostname (e.g. "abcdef…xyz.onion").
  // Empty until Start() succeeds.
  std::string_view onion_hostname() const;

  // Local SOCKS5 proxy port for connecting through Tor.
  uint16_t socks_port() const;

  // Returns the HiddenServiceDir path.
  base::FilePath hidden_service_dir() const;

 private:
  Config config_;
  base::Process tor_process_;
  std::string onion_hostname_;
  bool running_ = false;

  // Writes the torrc configuration file into the data directory.
  base::FilePath WriteTorrc();

  // Reads the hostname file produced by Tor after bootstrapping.
  bool ReadOnionHostname();
};

}  // namespace ipfs

#endif  // IPFS_TOR_ONION_SERVICE_H_
