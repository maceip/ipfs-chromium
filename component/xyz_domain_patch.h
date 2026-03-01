#ifndef IPFS_XYZ_DOMAIN_PATCH_H_
#define IPFS_XYZ_DOMAIN_PATCH_H_

#include "xyz_onion.h"

#include <string>
#include <string_view>
#include <vector>

namespace ipfs {

// Patch module for intercepting .xyz domain fetches. When a .xyz domain is
// fetched the request is routed through this module, which depends on
// XyzOnion being ready.
//
// Chromium-ergonomic readiness pattern:
//   If XyzOnion is ready   → process immediately via OnXyzFetch().
//   If XyzOnion is pending → queue the URL and observe XyzOnion; when the
//                            observer fires, drain the queue.
//   If XyzOnion hasn't started → kick off Start(), then queue.
//
// This avoids blocking the UI/IO thread and matches how CacheRequestor handles
// its own startup_pending_ state.
class XyzDomainPatch : public XyzOnion::Observer {
 public:
  explicit XyzDomainPatch(XyzOnion* onion);
  ~XyzDomainPatch() override;

  XyzDomainPatch(const XyzDomainPatch&) = delete;
  XyzDomainPatch& operator=(const XyzDomainPatch&) = delete;

  // Returns true if the given URL host ends with ".xyz".
  static bool IsXyzDomain(std::string_view host);

  // Called from the interceptor when a .xyz domain fetch is detected.
  // If XyzOnion is ready the fetch is processed immediately; otherwise it is
  // queued until the service signals readiness.
  void OnXyzFetch(std::string_view url);

  // XyzOnion::Observer -------------------------------------------------
  void OnXyzOnionReady(XyzOnion* service) override;

 private:
  // Actually process a single .xyz URL (called only when XyzOnion is ready).
  void ProcessFetch(const std::string& url);

  // Drain every queued URL through ProcessFetch().
  void DrainPendingFetches();

  raw_ptr<XyzOnion> onion_;
  std::vector<std::string> pending_fetches_;
};

}  // namespace ipfs

#endif  // IPFS_XYZ_DOMAIN_PATCH_H_
