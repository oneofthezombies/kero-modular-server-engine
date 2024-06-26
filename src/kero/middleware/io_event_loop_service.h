#ifndef KERO_MIDDLEWARE_IO_EVENT_LOOP_SERVICE_H
#define KERO_MIDDLEWARE_IO_EVENT_LOOP_SERVICE_H

#include "kero/core/utils_linux.h"
#include "kero/engine/service.h"
#include "kero/middleware/common.h"

struct epoll_event;

namespace kero {

class IoEventLoopService final : public Service {
 public:
  enum : Error::Code { kInvalidEpollFd = 1, kSocketClosed };

  struct AddOptions {
    bool in{false};
    bool out{false};
    bool edge_trigger{false};
  };

  explicit IoEventLoopService(
      const Borrow<RunnerContext> runner_context) noexcept;
  virtual ~IoEventLoopService() noexcept override = default;
  KERO_CLASS_KIND_MOVABLE(IoEventLoopService);
  KERO_SERVICE_KIND(kServiceKindId_IoEventLoop, "io_event_loop");

  [[nodiscard]] virtual auto
  OnCreate() noexcept -> Result<Void> override;

  virtual auto
  OnDestroy() noexcept -> void override;

  virtual auto
  OnUpdate() noexcept -> void override;

  [[nodiscard]] auto
  AddFd(const Fd::Value fd,
        const AddOptions options) const noexcept -> Result<Void>;

  [[nodiscard]] auto
  RemoveFd(const Fd::Value fd) const noexcept -> Result<Void>;

  [[nodiscard]] auto
  WriteToFd(const Fd::Value fd,
            const std::string_view data) const noexcept -> Result<Void>;

  [[nodiscard]] auto
  ReadFromFd(const Fd::Value fd) noexcept -> Result<std::string>;

 private:
  auto
  OnUpdateEpollEvent(const struct ::epoll_event& event) noexcept
      -> Result<Void>;

  Fd::Value epoll_fd_{Fd::kUnspecifiedInitialValue};

  static constexpr size_t kMaxEvents = 1024;
};

}  // namespace kero

#endif  // KERO_MIDDLEWARE_IO_EVENT_LOOP_SERVICE_H
