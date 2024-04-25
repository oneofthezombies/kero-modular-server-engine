#ifndef SERVER_FILE_DESCRIPTOR_LINUX_H
#define SERVER_FILE_DESCRIPTOR_LINUX_H

#include "core/core.h"

#include "common.h"

class FileDescriptorLinux final {
public:
  using Raw = int;

  explicit FileDescriptorLinux(const Raw fd) noexcept;
  FileDescriptorLinux(FileDescriptorLinux &&other) noexcept;
  ~FileDescriptorLinux() noexcept;

  auto operator=(FileDescriptorLinux &&other) noexcept -> FileDescriptorLinux &;

  auto IsValid() const noexcept -> bool;
  auto AsRaw() const noexcept -> Raw;

  [[nodiscard]] auto UpdateNonBlocking() const noexcept -> Result<Void>;

  [[nodiscard]] static auto IsValid(const Raw fd) noexcept -> bool;
  [[nodiscard]] static auto UpdateNonBlocking(const Raw fd) noexcept
      -> Result<Void>;

private:
  Raw fd_{kInvalidFd};

  static constexpr int kInvalidFd{-1};
};

auto operator<<(std::ostream &os, const FileDescriptorLinux &fd)
    -> std::ostream &;

#endif // SERVER_FILE_DESCRIPTOR_LINUX_H