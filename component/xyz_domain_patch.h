#ifndef IPFS_XYZ_DOMAIN_PATCH_H_
#define IPFS_XYZ_DOMAIN_PATCH_H_

#include <string>
#include <string_view>

namespace ipfs {

// Patch stub for intercepting .xyz domain fetches.
// When a .xyz domain is fetched, the request is routed through this module.
// TODO: Replace stub implementation with actual ROG source once provided.
class XyzDomainPatch {
 public:
  // Returns true if the given URL host ends with ".xyz".
  static bool IsXyzDomain(std::string_view host);

  // Called when a .xyz domain fetch is intercepted.
  // Stub: prints "BURP" to console. Replace with real logic later.
  static void OnXyzFetch(std::string_view url);
};

}  // namespace ipfs

#endif  // IPFS_XYZ_DOMAIN_PATCH_H_
