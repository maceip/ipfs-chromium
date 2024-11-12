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
      std::pair<std::string_view, int>{"http://127.0.0.1:8080/", 1043},
      {"https://ipfs.io/", 1001},
      {"https://dweb.link/", 941},
      {"https://trustless-gateway.link/", 938},
      {"https://hardbin.com/", 910},
      {"https://ipfs.greyh.at/", 858},
      {"https://ipfs.joaoleitao.org/", 848},
      {"https://dlunar.net/", 689},
      {"https://flk-ipfs.io/", 675},
      {"https://ipfs.cyou/", 471},
      {"https://human.mypinata.cloud/", 412},
      {"https://jcsl.hopto.org/", 363},
      {"https://delegated-ipfs.dev/", 318},
      {"https://4everland.io/", 297},
      {"https://ipfs.runfission.com/", 262},
      {"https://gateway.pinata.cloud/", 141},
      {"https://dag.w3s.link/", 135},
      {"https://flk-ipfs.xyz/", 104},
      {"https://ipfs.eth.aragon.network/", 11},
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
