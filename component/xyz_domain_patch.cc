#include "xyz_domain_patch.h"

#include "tor_onion_service.h"

#include "base/logging.h"

using Self = ipfs::XyzDomainPatch;

Self::XyzDomainPatch(XyzOnion* onion) : onion_(onion) {
  DCHECK(onion_);
  onion_->AddObserver(this);
}

Self::~XyzDomainPatch() {
  if (onion_) {
    onion_->RemoveObserver(this);
  }
}

bool Self::IsXyzDomain(std::string_view host) {
  constexpr std::string_view kSuffix = ".xyz";
  if (host.size() < kSuffix.size()) {
    return false;
  }
  return host.substr(host.size() - kSuffix.size()) == kSuffix;
}

bool Self::IsOnionDomain(std::string_view host) {
  constexpr std::string_view kSuffix = ".onion";
  if (host.size() < kSuffix.size()) {
    return false;
  }
  return host.substr(host.size() - kSuffix.size()) == kSuffix;
}

void Self::OnXyzFetch(std::string_view url) {
  if (onion_->is_ready()) {
    // Happy path: service is up, handle immediately.
    ProcessFetch(std::string(url));
    return;
  }

  // Service is not ready yet — queue and make sure startup has been kicked off.
  LOG(WARNING) << "XyzOnion not ready; deferring fetch for " << url
               << " (pending queue size: " << pending_fetches_.size() << ")";
  pending_fetches_.emplace_back(url);

  if (!onion_->startup_pending()) {
    onion_->Start();
  }
}

void Self::SetTorOnionService(std::shared_ptr<ipfs::TorOnionService> service) {
  tor_service_ = std::move(service);
}

std::shared_ptr<ipfs::TorOnionService> Self::GetTorOnionService() const {
  return tor_service_;
}

void Self::OnXyzOnionReady(XyzOnion* /*service*/) {
  LOG(INFO) << "XyzDomainPatch: XyzOnion is ready, draining "
            << pending_fetches_.size() << " queued fetch(es)";
  DrainPendingFetches();
}

void Self::ProcessFetch(const std::string& url) {
  if (tor_service_ && tor_service_->is_running()) {
    LOG(INFO) << "Routing fetch through Tor onion service (SOCKS5 port "
              << tor_service_->socks_port() << "): " << url;
    LOG(INFO) << "Onion address: " << tor_service_->onion_hostname();
  } else {
    LOG(INFO) << "XyzDomainPatch: processing fetch for " << url
              << " (no active Tor service)";
  }
}

void Self::DrainPendingFetches() {
  // Move the vector so re-entrant OnXyzFetch() calls during processing
  // go into a fresh queue rather than invalidating our iterator.
  std::vector<std::string> to_process;
  to_process.swap(pending_fetches_);
  for (const auto& url : to_process) {
    ProcessFetch(url);
  }
}
