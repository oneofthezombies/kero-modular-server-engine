#include "socket_router_service.h"

#include "kero/core/utils.h"
#include "kero/engine/actor_service.h"
#include "kero/engine/common.h"
#include "kero/engine/runner_context.h"
#include "kero/log/log_builder.h"
#include "kero/middleware/common.h"

using namespace kero;

kero::SocketRouterService::SocketRouterService(
    const Borrow<RunnerContext> runner_context, std::string&& target) noexcept
    : Service{runner_context, {kServiceKindId_Actor}},
      target_{std::move(target)} {}

auto
kero::SocketRouterService::OnCreate() noexcept -> Result<Void> {
  using ResultT = Result<Void>;

  if (target_.empty()) {
    return ResultT::Err(Error::From(
        FlatJson{}.Set("message", std::string{"target must be set"}).Take()));
  }

  if (!SubscribeEvent(EventSocketOpen::kEvent)) {
    return ResultT::Err(Error::From(
        FlatJson{}
            .Set("message",
                 std::string{"Failed to subscribe to socket open event"})
            .Take()));
  }

  return OkVoid();
}

auto
kero::SocketRouterService::OnEvent(const std::string& event,
                                   const FlatJson& data) noexcept -> void {
  if (event != EventSocketOpen::kEvent) {
    return;
  }

  const auto socket_id_opt = data.TryGet<u64>(EventSocketOpen::kSocketId);
  if (!socket_id_opt) {
    log::Error("Failed to get socket id from data").Data("data", data).Log();
    return;
  }

  GetDependency<ActorService>()->SendMail(std::string{target_},
                                          EventSocketMove::kEvent,
                                          data.Clone());
}
