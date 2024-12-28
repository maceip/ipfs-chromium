#include <ipfs_client/ctx/default_gateways.h>

#include <ipfs_client/ctx/gateway_config.h>

#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <string_view>

namespace ctx = ipfs::ctx;

auto ctx::LoadGatewaysFromEnvironmentVariable(ipfs::ctx::GatewayConfig& cfg) -> bool {
  auto* ovr = std::getenv("IPFS_GATEWAY");
  if (ovr == nullptr) {
    return false;
  }
  std::istringstream user_override{ovr};
  std::string gw_pfx;
  bool at_least_one = false;
  while (user_override >> gw_pfx) {
    if (gw_pfx.empty()) {
      continue;
    }
    if (gw_pfx.back() != '/') {
      gw_pfx.push_back('/');
    }
    cfg.AddGateway(gw_pfx, cfg.RoutingApiDiscoveryDefaultRate());
    at_least_one = true;
  }
  return at_least_one;
}
// NOLINTBEGIN(readability-magic-numbers)
void ctx::LoadStaticGatewayList(ipfs::ctx::GatewayConfig& cfg) {
  auto static_list = {
      std::pair<std::string_view, int>{"http://127.0.0.1:8080/", 1033},
      {"https://ipfs.io/", 990},
      {"https://dweb.link/", 931},
      {"https://trustless-gateway.link/", 928},
      {"https://hardbin.com/", 893},
      {"https://ipfs.greyh.at/", 849},
      {"https://ipfs.joaoleitao.org/", 831},
      {"https://dlunar.net/", 672},
      {"https://flk-ipfs.io/", 657},
      {"https://ipfs.cyou/", 457},
      {"https://human.mypinata.cloud/", 402},
      {"https://jcsl.hopto.org/", 346},
      {"https://delegated-ipfs.dev/", 300},
      {"https://4everland.io/", 288},
      {"https://ipfs.runfission.com/", 244},
      {"https://gateway.pinata.cloud/", 128},
      {"https://dag.w3s.link/", 124},
      {"https://flk-ipfs.xyz/", 108},
      {"https://ipfs.eth.aragon.network/", 20},
      {"https://data.filstorage.io/", 10},
      {"https://storry.tv/", 9},

      //Currently redirects to https://ipfs.io
      {"https://cloudflare-ipfs.com/", 8},
      {"https://cf-ipfs.com/", 7},
      {"https://fleek.ipfs.io/", 6},
      {"https://ipfs.fleek.co/", 5},
      {"https://permaweb.eu.org/", 4},
      {"https://gateway.ipfs.io/", 3},

      //Currently redirects to https://dweb.link/
      {"https://nftstorage.link/", 2},
      {"https://w3s.link/", 1}
    };
  for (auto [gw, rt] : static_list) {
    cfg.AddGateway(gw, rt);
  }
}
// NOLINTEND(readability-magic-numbers)
