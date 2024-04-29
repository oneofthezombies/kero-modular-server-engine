#include "socket_pool_service.h"

#include "io_event_loop_service.h"
#include "kero/engine/agent.h"
#include "kero/engine/constants.h"

using namespace kero;

kero::SocketPoolService::SocketPoolService() noexcept
    : Service{ServiceKind::kSocket} {}

auto
kero::SocketPoolService::OnCreate(Agent& agent) noexcept -> Result<Void> {
  using ResultT = Result<Void>;

  if (!agent.SubscribeEvent(EventSocketOpen::kEvent, GetKind())) {
    return ResultT::Err(Error::From(
        Dict{}
            .Set("message",
                 std::string{"Failed to subscribe to socket open event"})
            .Take()));
  }

  if (!agent.SubscribeEvent(EventSocketClose::kEvent, GetKind())) {
    return ResultT::Err(Error::From(
        Dict{}
            .Set("message",
                 std::string{"Failed to subscribe to socket close event"})
            .Take()));
  }

  if (!agent.HasServiceIs<IoEventLoopService>(ServiceKind::kIoEventLoop)) {
    return ResultT::Err(Error::From(
        Dict{}
            .Set("message", std::string{"IoEventLoopService not found"})
            .Take()));
  }

  return ResultT::Ok(Void{});
}

auto
kero::SocketPoolService::OnEvent(Agent& agent,
                                 const std::string& event,
                                 const Dict& data) noexcept -> void {
  if (event == EventSocketOpen::kEvent) {
    OnSocketOpen(agent, data);
  } else if (event == EventSocketClose::kEvent) {
    OnSocketClose(agent, data);
  } else {
    // TODO: log error
  }
}

auto
kero::SocketPoolService::OnSocketOpen(Agent& agent, const Dict& data) noexcept
    -> void {
  auto fd = data.GetOrDefault<int64_t>(EventSocketOpen::kFd, -1);
  if (fd == -1) {
    // TODO: log error
    return;
  }

  auto io_event_loop =
      agent.GetServiceAs<IoEventLoopService>(ServiceKind::kIoEventLoop);
  if (!io_event_loop) {
    // TODO: log error
    return;
  }

  if (auto res =
          io_event_loop.Unwrap().AddFd(fd, {.in = true, .edge_trigger = true});
      res.IsErr()) {
    // TODO: log error
    return;
  }

  sockets_.insert(fd);
}

auto
kero::SocketPoolService::OnSocketClose(Agent& agent, const Dict& data) noexcept
    -> void {
  auto fd = data.GetOrDefault<int64_t>(EventSocketClose::kFd, -1);
  if (fd == -1) {
    // TODO: log error
    return;
  }

  sockets_.erase(fd);

  auto io_event_loop =
      agent.GetServiceAs<IoEventLoopService>(ServiceKind::kIoEventLoop);
  if (!io_event_loop) {
    // TODO: log error
    return;
  }

  if (auto res = io_event_loop.Unwrap().RemoveFd(fd); res.IsErr()) {
    // TODO: log error
    return;
  }
}