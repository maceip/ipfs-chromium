#include "xyz_onion.h"

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/time/time.h"

using Self = ipfs::XyzOnion;

Self::XyzOnion() = default;
Self::~XyzOnion() = default;

void Self::Start() {
  if (is_ready_ || startup_pending_) {
    return;
  }
  startup_pending_ = true;

  // Simulate a service that takes 10s–2min to become ready (e.g. establishing
  // onion circuits, bootstrapping a DHT, etc.).
  int delay_seconds = base::RandInt(10, 120);
  LOG(INFO) << "XyzOnion: starting up, estimated ready in " << delay_seconds
            << "s";

  startup_timer_.Start(FROM_HERE, base::Seconds(delay_seconds),
                       base::BindOnce(&Self::OnStartupComplete,
                                      weak_factory_.GetWeakPtr()));
}

void Self::OnStartupComplete() {
  DCHECK(startup_pending_);
  startup_pending_ = false;
  is_ready_ = true;
  LOG(INFO) << "XyzOnion: service is now ready";

  for (auto& observer : observers_) {
    observer.OnXyzOnionReady(this);
  }
}

void Self::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void Self::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}
