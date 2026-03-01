#include "inter_request_state.h"

#include "chromium_ipfs_context.h"
#include "json_parser_adapter.h"
#include "preferences.h"

#include <base/logging.h>
#include <content/public/browser/browser_context.h>
#include <content/browser/child_process_security_policy_impl.h>
#include <third_party/blink/renderer/platform/weborigin/scheme_registry.h>

#include <ipfs_client/gw/default_requestor.h>
#include <ipfs_client/ipfs_request.h>
#include <ipfs_client/response.h>
#include "ipfs_client/ctx/default_gateways.h"

using Self = ipfs::InterRequestState;

namespace {
constexpr char user_data_key[] = "ipfs_request_userdata";
}

void Self::CreateForBrowserContext(content::BrowserContext* c, PrefService* p) {
  DCHECK(c);
  DCHECK(p);
  auto owned = std::make_unique<ipfs::InterRequestState>(c->GetPath(), p);
  c->SetUserData(user_data_key, std::move(owned));
  auto* cpsp = content::ChildProcessSecurityPolicy::GetInstance();
  for (std::string scheme : {"ipfs", "ipns"}) {
    cpsp->RegisterWebSafeScheme(scheme);
  }
}
auto Self::FromBrowserContext(content::BrowserContext* context)
    -> InterRequestState& {
  auto* cpsp = content::ChildProcessSecurityPolicy::GetInstance();
  for (std::string scheme : {"ipfs", "ipns"}) {
    if (!(cpsp->IsWebSafeScheme(scheme))) {
      cpsp->RegisterWebSafeScheme(scheme);
    }
    // auto s = WTF::String::FromUTF8(scheme);
    // blink::SchemeRegistry::RegisterURLSchemeAsAllowedForReferrer(s);
    // blink::SchemeRegistry::RegisterURLSchemeAsSupportingFetchAPI(s);
  }
  if (!context) {
    LOG(WARNING) << "No browser context! Using a default IPFS state.";
    static ipfs::InterRequestState static_state({}, {});
    return static_state;
  }
  base::SupportsUserData::Data* existing = context->GetUserData(user_data_key);
  if (existing) {
    return *static_cast<ipfs::InterRequestState*>(existing);
  } else {
    LOG(ERROR) << "Browser context has no IPFS state! It must be set earlier!";
    static ipfs::InterRequestState static_state({}, {});
    return static_state;
  }
}
std::shared_ptr<ipfs::Client> Self::api() {
  return api_;
}
auto Self::cache() -> std::shared_ptr<CacheRequestor>& {
  if (!cache_) {
    cache_ = std::make_shared<CacheRequestor>(*this, disk_path_);
  }
  return cache_;
}
auto Self::orchestrator() -> Partition& {
  if (!cache_) {
    auto rtor = gw::default_requestor(cache(), api());
    api()->with(rtor);
  }
  // TODO - use origin
  return *api()->partition({});
}
void Self::network_context(network::mojom::NetworkContext* val) {
  network_context_ = val;
}
network::mojom::NetworkContext* Self::network_context() const {
  return network_context_;
}
Self::InterRequestState(base::FilePath p, PrefService* prefs)
    : api_{CreateContext(*this, prefs)}, disk_path_{p} {
  api_->with(std::make_unique<JsonParserAdapter>());
  DCHECK(prefs);

  // Boot the XyzOnion service eagerly — it takes 10s–2min to become ready,
  // so we want the clock ticking as soon as the profile is created.
  xyz_onion_ = std::make_unique<XyzOnion>();
  xyz_onion_->Start();

  // XyzDomainPatch observes XyzOnion readiness and queues deferred fetches.
  xyz_domain_patch_ = std::make_unique<XyzDomainPatch>(xyz_onion_.get());
}
Self::~InterRequestState() noexcept {
  // Tear down observers before the service they observe.
  xyz_domain_patch_.reset();
  xyz_onion_.reset();
  network_context_ = nullptr;
  cache_.reset();
}
ipfs::XyzOnion& Self::xyz_onion() {
  return *xyz_onion_;
}
ipfs::XyzDomainPatch& Self::xyz_domain_patch() {
  return *xyz_domain_patch_;
}
