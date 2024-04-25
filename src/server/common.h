#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

#include <cstdint>

#include "core/core.h"
#include "core/utils.h"

/**
 * Server symbols start from 1,000,000
 */
enum class Symbol : int32_t {
  kBegin = 1'000'000,
  // Add symbols after kBegin

  kHelpRequested,
  kPortArgNotFound,
  kPortValueNotFound,
  kPortParsingFailed,
  kUnknownArgument,

  kMailBoxAlreadyExists,
  kMailBoxNotFound,
  kMailBoxNameEmpty,
  kMailBoxNameTooLong,
  kMailBoxNameAll,

  kEventLoopLinuxEpollCreate1Failed,
  kEventLoopLinuxEpollCtlAddFailed,
  kEventLoopLinuxEpollCtlDeleteFailed,
  kEventLoopLinuxEpollWaitFailed,
  kEventLoopLinuxWriteFailed,
  kEventLoopLinuxWriteClosed,

  kMainEventLoopLinuxServerSocketFailed,
  kMainEventLoopLinuxServerSocketBindFailed,
  kMainEventLoopLinuxServerSocketListenFailed,
  kMainEventLoopLinuxServerSocketAcceptFailed,
  kMainEventLoopLinuxUnexpectedFd,

  kLobbyEventLoopLinuxClientFdConversionFailed,
  kLobbyEventLoopLinuxClientFdAlreadyExists,

  kBattleEventLoopLinuxMatchedClientFdsParseFailed,
  kBattleEventLoopLinuxMatchedClientFdsKeyNotFound,
  kBattleEventLoopLinuxClientFdConversionFailed,
  kBattleEventLoopLinuxClientFdAlreadyExists,
  kBattleEventLoopLinuxHandlerNotFound,

  kFileDescriptorLinuxGetStatusFailed,
  kFileDescriptorLinuxSetStatusFailed,
  kFileDescriptorLinuxCloseFailed,

  kLinuxSignalSetFailed,
  kLinuxSignalResetFailed,

  kThreadWorkerSessionAlreadyExists,

  // Add symbols before kEnd
  kEnd
};

auto operator<<(std::ostream &os, const Symbol symbol) noexcept
    -> std::ostream &;

using Error = ErrorBase<Symbol>;

template <typename T> using Result = ResultBase<T, Error>;

#endif // SERVER_COMMON_H