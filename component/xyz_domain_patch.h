#ifndef IPFS_XYZ_DOMAIN_PATCH_H_
#define IPFS_XYZ_DOMAIN_PATCH_H_

#include "xyz_onion.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ipfs {

class TorOnionService;

// Intercepts .xyz domain fetches and routes them through a Tor onion service.
//
// Chromium-ergonomic readiness pattern:
//   If XyzOnion is ready   -> process immediately via OnXyzFetch().
//   If XyzOnion is pending -> queue the URL and observe XyzOnion; when the
//                             observer fires, drain the queue.
//   If XyzOnion hasn't started -> kick off Start(), then queue.
//
// When a .xyz or .onion domain is detected, the request is proxied through
// the local Tor SOCKS5 endpoint so that the traffic exits via the onion
// network.
class XyzDomainPatch : public XyzOnion::Observer {
 public:
  explicit XyzDomainPatch(XyzOnion* onion);
  ~XyzDomainPatch() override;

  XyzDomainPatch(const XyzDomainPatch&) = delete;
  XyzDomainPatch& operator=(const XyzDomainPatch&) = delete;

  // Returns true if the given URL host ends with ".xyz".
  static bool IsXyzDomain(std::string_view host);

  // Returns true if the given URL host is a .onion address.
  static bool IsOnionDomain(std::string_view host);

  // Called from the interceptor when a .xyz or .onion domain fetch is
  // detected.  If XyzOnion is ready the fetch is processed immediately;
  // otherwise it is queued until the service signals readiness.
  void OnXyzFetch(std::string_view url);

  // Binds a TorOnionService instance so that subsequent fetches can be
  // routed through it.  Ownership is shared.
  void SetTorOnionService(std::shared_ptr<TorOnionService> service);

  // Returns the currently bound TorOnionService, or nullptr.
  std::shared_ptr<TorOnionService> GetTorOnionService() const;

  // XyzOnion::Observer -------------------------------------------------
  void OnXyzOnionReady(XyzOnion* service) override;

 private:
  // Actually process a single .xyz URL (called only when XyzOnion is ready).
  void ProcessFetch(const std::string& url);

  // Drain every queued URL through ProcessFetch().
  void DrainPendingFetches();

  raw_ptr<XyzOnion> onion_;
  std::vector<std::string> pending_fetches_;
  std::shared_ptr<TorOnionService> tor_service_;
};

}  // namespace ipfs

#endif  // IPFS_XYZ_DOMAIN_PATCH_H_
