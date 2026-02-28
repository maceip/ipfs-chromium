#include "xyz_domain_patch.h"

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

void XyzDomainPatch::OnXyzFetch(std::string_view url) {
  // Stub: just prints BURP. Replace with real ROG logic later.
  LOG(WARNING) << "BURP";
}

}  // namespace ipfs
