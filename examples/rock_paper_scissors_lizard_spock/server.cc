#include <memory>

#include "battle_service.cc"
#include "kero/core/utils.h"
#include "kero/engine/actor_service.h"
#include "kero/engine/engine.h"
#include "kero/engine/runner_builder.h"
#include "kero/engine/signal_service.h"
#include "kero/log/center.h"
#include "kero/log/core.h"
#include "kero/log/transport.h"
#include "kero/middleware/config_service.h"
#include "kero/middleware/io_event_loop_service.h"
#include "kero/middleware/socket_router_service.h"
#include "kero/middleware/tcp_server_service.h"
#include "match_service.cc"

/**
 * `using namespace kero` is used because it is an example program.
 */
using namespace kero;

auto
main(int argc, char** argv) -> int {
  Center{}.UseStreamForLoggingSystemError();
  auto transport = std::make_unique<ConsolePlainTextTransport>();
  transport->SetLevel(Level::kDebug);
  Center{}.AddTransport(std::move(transport));
  Defer defer_log_system{[] { Center{}.Shutdown(); }};

  auto engine = std::make_unique<Engine>();
  auto main_runner =
      engine->CreateRunnerBuilder("main")
          .AddServiceFactory(std::make_unique<ConfigServiceFactory>(argc, argv))
          .AddServiceFactory(
              std::make_unique<DefaultServiceFactory<SignalService>>())
          .AddServiceFactory(std::make_unique<ActorServiceFactory>(engine))
          .AddServiceFactory(
              std::make_unique<DefaultServiceFactory<IoEventLoopService>>())
          .AddServiceFactory(
              std::make_unique<DefaultServiceFactory<TcpServerService>>())
          .AddServiceFactory([](const Pin<RunnerContext> runner_context) {
            return Result<Own<Service>>{
                std::make_unique<SocketRouterService>(runner_context, "match")};
          })
          .BuildRunner();

  auto match_runner =
      engine->CreateRunnerBuilder("match")
          .AddServiceFactory(std::make_unique<ActorServiceFactory>(engine))
          .AddServiceFactory(
              std::make_unique<DefaultServiceFactory<IoEventLoopService>>())
          .AddServiceFactory([](const Pin<RunnerContext> runner_context) {
            return Result<Own<Service>>{
                std::make_unique<MatchService>(runner_context)};
          })
          .BuildThreadRunner();
  if (match_runner.IsErr()) {
    log::Error("Failed to create match runner")
        .Data("error", match_runner.TakeErr())
        .Log();
    return 1;
  }

  if (auto res = match_runner.Ok()->Start(); res.IsErr()) {
    log::Error("Failed to start match runner")
        .Data("error", res.TakeErr())
        .Log();
    return 1;
  }

  auto battle_runner =
      engine->CreateRunnerBuilder("battle")
          .AddServiceFactory(std::make_unique<ActorServiceFactory>(engine))
          .AddServiceFactory(
              std::make_unique<DefaultServiceFactory<IoEventLoopService>>())
          .AddServiceFactory([](const Pin<RunnerContext> runner_context) {
            return Result<Own<Service>>{
                std::make_unique<MatchService>(runner_context)};
          })
          .BuildThreadRunner();

  if (battle_runner.IsErr()) {
    log::Error("Failed to create battle runner")
        .Data("error", battle_runner.TakeErr())
        .Log();
    return 1;
  }

  if (auto res = battle_runner.Ok()->Start(); res.IsErr()) {
    log::Error("Failed to start battle runner")
        .Data("error", res.TakeErr())
        .Log();
    return 1;
  }

  if (auto res = main_runner.Ok()->Run(); res.IsErr()) {
    log::Error("Failed to start main runner")
        .Data("error", res.TakeErr())
        .Log();
    return 1;
  }

  return 0;
}
