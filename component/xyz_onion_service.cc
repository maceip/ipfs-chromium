#include "xyz_onion_service.h"

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/rand_util.h"
#include "base/time/time.h"

namespace ipfs {

XyzOnionService& XyzOnionService::Get() {
  static base::NoDestructor<XyzOnionService> service;
  return *service;
}

XyzOnionService::XyzOnionService() = default;

XyzOnionService::~XyzOnionService() = default;

bool XyzOnionService::IsReady() const {
  return state_ == State::kReady;
}

void XyzOnionService::HandleXyzFetch(std::string_view url) {
  EnsureStarted();
  if (IsReady()) {
    ProcessReadyFetch(url);
    return;
  }

  pending_urls_.emplace_back(url);
  LOG(WARNING) << "XyzOnion is still warming up; deferred xyz fetch for "
               << url;
}

void XyzOnionService::EnsureStarted() {
  if (state_ != State::kNotStarted) {
    return;
  }

  state_ = State::kStarting;
  startup_delay_ = base::Seconds(base::RandInt(10, 120));
  LOG(INFO) << "Starting XyzOnion service. Expected readiness in "
            << startup_delay_.InSeconds() << "s.";
  startup_timer_.Start(FROM_HERE, startup_delay_,
                       base::BindOnce(&XyzOnionService::OnReady,
                                      base::Unretained(this)));
}

void XyzOnionService::OnReady() {
  state_ = State::kReady;
  LOG(INFO) << "XyzOnion is ready. Flushing " << pending_urls_.size()
            << " deferred xyz fetch(es).";

  for (const std::string& url : pending_urls_) {
    ProcessReadyFetch(url);
  }
  pending_urls_.clear();
}

void XyzOnionService::ProcessReadyFetch(std::string_view url) {
  LOG(INFO) << "BURP (XyzOnion-ready fetch): " << url;
}

}  // namespace ipfs
