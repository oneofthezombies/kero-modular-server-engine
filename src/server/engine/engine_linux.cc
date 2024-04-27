#include "engine_linux.h"

#include <atomic>

#include <netinet/in.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "core/tiny_json.h"
#include "core/utils.h"
#include "core/utils_linux.h"

#include "common.h"
#include "event_loop.h"
#include "mail_center.h"

using namespace engine;

std::atomic<const MailBox *> signal_mail_box_ptr{nullptr};

auto OnSignal(int signal) -> void {
  if (signal == SIGINT) {
    if (signal_mail_box_ptr != nullptr) {
      (*signal_mail_box_ptr)
          .tx.Send(Mail{"signal", "all",
                        std::move(core::TinyJson{}.Set("shutdown", ""))});
    }
  }

  core::TinyJson{}
      .Set("reason", "signal_received")
      .Set("signal", signal)
      .LogLn();
}

auto engine::EngineLinux::Builder::Build(Config &&config) const noexcept
    -> Result<EngineLinux> {
  using ResultT = Result<EngineLinux>;

  return ResultT{EngineLinux{std::move(config)}};
}

engine::EngineLinux::EngineLinux(Config &&config) noexcept
    : config_{std::move(config)} {}

auto engine::EngineLinux::AddEventLoop(std::string &&name,
                                       EventLoopHandlerPtr &&handler) noexcept
    -> Result<Void> {
  using ResultT = Result<Void>;

  if (auto it = event_loop_threads_.find(name);
      it != event_loop_threads_.end()) {
    return ResultT{Error{Symbol::kEngineEventLoopAlreadyExists,
                         core::TinyJson{}.Set("name", name).ToString()}};
  }

  auto event_loop_res =
      EventLoop::Builder{}.Build(std::string{name}, std::move(handler));
  if (event_loop_res.IsErr()) {
    return ResultT{std::move(event_loop_res.Err())};
  }

  auto &event_loop = event_loop_res.Ok();
  if (auto res = event_loop.Init(config_); res.IsErr()) {
    return ResultT{std::move(res.Err())};
  }

  event_loop_threads_.emplace(
      std::move(name),
      std::thread{EngineLinux::EventLoopThreadMain, std::move(event_loop)});
  return ResultT{Void{}};
}

auto engine::EngineLinux::Run() noexcept -> Result<Void> {
  using ResultT = Result<Void>;

  auto signal_mail_box_res = MailCenter::Global().Create("signal");
  if (signal_mail_box_res.IsErr()) {
    return ResultT{std::move(signal_mail_box_res.Err())};
  }

  const auto &signal_mail_box = signal_mail_box_res.Ok();
  signal_mail_box_ptr.store(&signal_mail_box);

  {
    core::Defer reset_signal_mail_box_ptr{
        []() { signal_mail_box_ptr.store(nullptr); }};
    if (signal(SIGINT, OnSignal) == SIG_ERR) {
      return ResultT{
          Error{Symbol::kLinuxSignalSetFailed,
                core::TinyJson{}
                    .Set("linux_error", core::LinuxError::FromErrno())
                    .ToString()}};
    }

    if (auto res = main_event_loop_.Run(); res.IsErr()) {
      return res;
    }

    if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
      return ResultT{
          Error{Symbol::kLinuxSignalResetFailed,
                core::TinyJson{}
                    .Set("linux_error", core::LinuxError::FromErrno())
                    .ToString()}};
    }
  }

  MailCenter::Global().Shutdown();
  return ResultT{Void{}};
}

auto engine::EngineLinux::EventLoopThreadMain(EventLoop &event_loop) noexcept
    -> void {
  if (auto res = event_loop.Run(); res.IsErr()) {
    core::TinyJson{}
        .Set("reason", "event_loop_thread_main_failed")
        .Set("name", event_loop.Name())
        .Set("error", res.Err())
        .LogLn();
  }
}