#include "xyz_domain_patch.h"

#include "inter_request_state.h"
#include "base/logging.h"

namespace ipfs {

bool XyzDomainPatch::IsXyzDomain(std::string_view host) {
  // Match any host ending in ".xyz" (e.g. "example.xyz", "foo.bar.xyz")
  constexpr std::string_view kSuffix = ".xyz";
  if (host.size() < kSuffix.size()) {
    return false;
  }
  return host.substr(host.size() - kSuffix.size()) == kSuffix;
}

XyzFetchStatus XyzDomainPatch::OnXyzFetch(InterRequestState& state,
                                          std::string_view url) {
  auto& xyz_onion = state.xyz_onion_service();
  const bool was_ready = xyz_onion.IsReady();
  xyz_onion.HandleXyzFetch(url);

  if (!was_ready) {
    LOG(INFO) << "XyzDomainPatch deferred fetch while XyzOnion starts.";
    return XyzFetchStatus::kDeferredUntilXyzOnionReady;
  }
  return XyzFetchStatus::kHandled;
}

}  // namespace ipfs
