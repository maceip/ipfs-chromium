#include "xyz_domain_patch.h"

#include "tor_onion_service.h"

#include "base/logging.h"
#include "base/no_destructor.h"

namespace ipfs {

namespace {
std::shared_ptr<TorOnionService>& OnionServiceInstance() {
  static base::NoDestructor<std::shared_ptr<TorOnionService>> instance;
  return *instance;
}
}  // namespace

bool XyzDomainPatch::IsXyzDomain(std::string_view host) {
  constexpr std::string_view kSuffix = ".xyz";
  if (host.size() < kSuffix.size()) {
    return false;
  }
  return host.substr(host.size() - kSuffix.size()) == kSuffix;
}

bool XyzDomainPatch::IsOnionDomain(std::string_view host) {
  constexpr std::string_view kSuffix = ".onion";
  if (host.size() < kSuffix.size()) {
    return false;
  }
  return host.substr(host.size() - kSuffix.size()) == kSuffix;
}

void XyzDomainPatch::OnXyzFetch(std::string_view url) {
  auto& service = OnionServiceInstance();
  if (service && service->is_running()) {
    LOG(INFO) << "Routing .xyz fetch through Tor onion service (SOCKS5 port "
              << service->socks_port() << "): " << url;
    LOG(INFO) << "Onion address: " << service->onion_hostname();
  } else {
    LOG(WARNING) << "No active Tor onion service for .xyz fetch: " << url;
  }
}

void XyzDomainPatch::SetOnionService(
    std::shared_ptr<TorOnionService> service) {
  OnionServiceInstance() = std::move(service);
}

std::shared_ptr<TorOnionService> XyzDomainPatch::GetOnionService() {
  return OnionServiceInstance();
}

uint16_t XyzDomainPatch::GetSocksPort() {
  auto& service = OnionServiceInstance();
  if (service && service->is_running()) {
    return service->socks_port();
  }
  return 0;
}

std::string XyzDomainPatch::GetOnionHostname() {
  auto& service = OnionServiceInstance();
  if (service && service->is_running()) {
    return std::string(service->onion_hostname());
  }
  return {};
}

}  // namespace ipfs
