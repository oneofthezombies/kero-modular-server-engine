#include "event_loop_linux.h"

#include <sys/epoll.h>

#include "core/tiny_json.h"

#include "event_loop_handler.h"
#include "mail_center.h"
#include "utils_linux.h"

using namespace engine;

engine::EventLoopLinux::EventLoopLinux(MailBox &&mail_box, std::string &&name,
                                       FileDescriptorLinux &&epoll_fd,
                                       EventLoopHandlerPtr &&handler) noexcept
    : mail_box_{std::move(mail_box)}, name_{std::move(name)},
      epoll_fd_{std::move(epoll_fd)}, handler_{std::move(handler)} {}

auto engine::EventLoopLinux::Builder::Build(
    std::string &&name, EventLoopHandlerPtr &&handler) const noexcept
    -> Result<EventLoopLinux> {
  using ResultT = Result<EventLoopLinux>;

  auto epoll_fd = FileDescriptorLinux{epoll_create1(0)};
  if (!epoll_fd.IsValid()) {
    return ResultT{Error{Symbol::kEventLoopLinuxEpollCreate1Failed,
                         core::TinyJson{}
                             .Set("linux_error", core::LinuxError::FromErrno())
                             .ToString()}};
  }

  auto mail_box_res = MailCenter::Global().Create(std::string{name});
  if (mail_box_res.IsErr()) {
    return ResultT{std::move(mail_box_res.Err())};
  }

  return ResultT{EventLoopLinux{std::move(mail_box_res.Ok()), std::move(name),
                                std::move(epoll_fd), std::move(handler)}};
}

auto engine::EventLoopLinux::Add(const FileDescriptorLinux::Raw fd,
                                 const uint32_t events) noexcept
    -> Result<Void> {
  using ResultT = Result<Void>;

  struct epoll_event ev {};
  ev.events = events;
  ev.data.fd = fd;
  if (epoll_ctl(epoll_fd_.AsRaw(), EPOLL_CTL_ADD, fd, &ev) == -1) {
    return ResultT{Error{Symbol::kEventLoopLinuxEpollCtlAddFailed,
                         core::TinyJson{}
                             .Set("linux_error", core::LinuxError::FromErrno())
                             .ToString()}};
  }

  return ResultT{Void{}};
}

auto engine::EventLoopLinux::Delete(const FileDescriptorLinux::Raw fd) noexcept
    -> Result<Void> {
  using ResultT = Result<Void>;

  if (epoll_ctl(epoll_fd_.AsRaw(), EPOLL_CTL_DEL, fd, nullptr) == -1) {
    return ResultT{Error{Symbol::kEventLoopLinuxEpollCtlDeleteFailed,
                         core::TinyJson{}
                             .Set("linux_error", core::LinuxError::FromErrno())
                             .ToString()}};
  }

  return ResultT{Void{}};
}

auto engine::EventLoopLinux::Write(const FileDescriptorLinux::Raw fd,
                                   const std::string_view data) noexcept
    -> Result<Void> {
  using ResultT = Result<Void>;

  const auto data_size = data.size();
  ssize_t written = 0;
  while (written < data_size) {
    const auto count = write(fd, data.data() + written, data_size - written);
    if (count == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }

      return ResultT{
          Error{Symbol::kEventLoopLinuxWriteFailed,
                core::TinyJson{}
                    .Set("linux_error", core::LinuxError::FromErrno())
                    .Set("fd", fd)
                    .ToString()}};
    } else {
      written += count;

      if (count == 0) {
        return ResultT{Error{Symbol::kEventLoopLinuxWriteClosed,
                             core::TinyJson{}.Set("fd", fd).ToString()}};
      }
    }
  }

  return ResultT{Void{}};
}

auto engine::EventLoopLinux::Run() noexcept -> Result<Void> {
  using ResultT = Result<Void>;

  struct epoll_event events[kMaxEvents]{};
  std::atomic<bool> shutdown{false};
  while (!shutdown) {
    auto mail = mail_box_.rx.TryReceive();
    if (mail) {
      if (auto value = mail->body.Get("shutdown"); value) {
        shutdown = true;
      }

      if (auto res = OnMailReceived(*mail); res.IsErr()) {
        return res;
      }
    }

    const auto fd_count = epoll_wait(epoll_fd_.AsRaw(), events, kMaxEvents, 0);
    if (fd_count == -1) {
      if (errno == EINTR) {
        continue;
      }

      return ResultT{
          Error{Symbol::kEventLoopLinuxEpollWaitFailed,
                core::TinyJson{}
                    .Set("linux_error", core::LinuxError::FromErrno())
                    .ToString()}};
    }

    for (int i = 0; i < fd_count; ++i) {
      const auto &event = events[i];
      if (auto res = OnEpollEventReceived(event); res.IsErr()) {
        return res;
      }
    }
  }

  return ResultT{Void{}};
}

auto engine::EventLoopLinux::OnMailReceived(const Mail &mail) noexcept
    -> Result<Void> {
  using ResultT = Result<Void>;

  return ResultT{Void{}};
}

auto engine::EventLoopLinux::OnEpollEventReceived(
    const struct epoll_event &event) noexcept -> Result<Void> {
  using ResultT = Result<Void>;

  return ResultT{Void{}};
}
