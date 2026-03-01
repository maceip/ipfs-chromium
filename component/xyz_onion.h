#ifndef IPFS_XYZ_ONION_H_
#define IPFS_XYZ_ONION_H_

#include "export.h"

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/timer/timer.h"

#include <string_view>
#include <vector>

namespace ipfs {

// A stub long-running service that simulates a dependency (like a Tor onion
// proxy or similar) which takes between 10 seconds and 2 minutes to become
// ready after startup. Follows the Chromium service management pattern:
//
//   - Owned per-BrowserContext via InterRequestState (SupportsUserData::Data)
//   - Asynchronous startup via base::OneShotTimer
//   - Observer interface so consumers (like XyzDomainPatch) can defer work
//     until the service is ready, rather than polling or blocking
//   - startup_pending_ flag (same pattern as CacheRequestor) for synchronous
//     readiness checks
//
class COMPONENT_EXPORT(IPFS) XyzOnion {
 public:
  // Observer interface — implemented by consumers that need to know when
  // XyzOnion transitions to ready.
  class COMPONENT_EXPORT(IPFS) Observer : public base::CheckedObserver {
   public:
    virtual void OnXyzOnionReady(XyzOnion* service) = 0;
  };

  XyzOnion();
  ~XyzOnion();

  XyzOnion(const XyzOnion&) = delete;
  XyzOnion& operator=(const XyzOnion&) = delete;

  // Kicks off the asynchronous startup. Safe to call multiple times; repeated
  // calls while startup is already pending are no-ops.
  void Start();

  // Returns true once the service has finished initializing and is ready to
  // handle requests.
  bool is_ready() const { return is_ready_; }

  // Returns true while the service is starting up but not yet ready.
  bool startup_pending() const { return startup_pending_; }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  // Called by the one-shot timer when the simulated startup delay expires.
  void OnStartupComplete();

  bool is_ready_ = false;
  bool startup_pending_ = false;
  base::OneShotTimer startup_timer_;
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<XyzOnion> weak_factory_{this};
};

}  // namespace ipfs

#endif  // IPFS_XYZ_ONION_H_
