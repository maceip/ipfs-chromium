#ifndef IPFS_XYZ_ONION_SERVICE_H_
#define IPFS_XYZ_ONION_SERVICE_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/time/time.h"
#include "base/timer/timer.h"

namespace ipfs {

class XyzOnionService {
 public:
  static XyzOnionService& Get();

  XyzOnionService(const XyzOnionService&) = delete;
  XyzOnionService& operator=(const XyzOnionService&) = delete;

  bool IsReady() const;
  void HandleXyzFetch(std::string_view url);

 private:
  friend class XyzOnionServiceForTesting;

  enum class State {
    kNotStarted,
    kStarting,
    kReady,
  };

  XyzOnionService();
  ~XyzOnionService();

  void EnsureStarted();
  void OnReady();
  void ProcessReadyFetch(std::string_view url);

  State state_ = State::kNotStarted;
  base::TimeDelta startup_delay_;
  base::OneShotTimer startup_timer_;
  std::vector<std::string> pending_urls_;
};

}  // namespace ipfs

#endif  // IPFS_XYZ_ONION_SERVICE_H_
