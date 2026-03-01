#ifndef IPFS_XYZ_DOMAIN_PATCH_H_
#define IPFS_XYZ_DOMAIN_PATCH_H_

#include <memory>
#include <string>
#include <string_view>

namespace ipfs {

class TorOnionService;

// Intercepts .xyz domain fetches and routes them through a Tor onion service.
//
// When a .xyz domain is detected, the request is proxied through the local
// Tor SOCKS5 endpoint so that the traffic exits via the onion network.
class XyzDomainPatch {
 public:
  // Returns true if the given URL host ends with ".xyz".
  static bool IsXyzDomain(std::string_view host);

  // Returns true if the given URL host is a .onion address.
  static bool IsOnionDomain(std::string_view host);

  // Called when a .xyz domain fetch is intercepted.
  // Routes the request through the Tor onion service if one is active.
  static void OnXyzFetch(std::string_view url);

  // Binds a TorOnionService instance so that subsequent .xyz fetches can
  // be routed through it.  Ownership is shared.
  static void SetOnionService(std::shared_ptr<TorOnionService> service);

  // Returns the currently bound onion service, or nullptr.
  static std::shared_ptr<TorOnionService> GetOnionService();

  // Returns the local SOCKS5 proxy port if a Tor service is active, or 0.
  static uint16_t GetSocksPort();

  // Returns the .onion hostname if the service is running, or empty.
  static std::string GetOnionHostname();
};

}  // namespace ipfs

#endif  // IPFS_XYZ_DOMAIN_PATCH_H_
