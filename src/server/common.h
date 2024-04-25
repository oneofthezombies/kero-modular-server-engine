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

  kEventLoopLinuxEpollCreate1Failed,
  kEventLoopLinuxEpollCtlAddFailed,
  kEventLoopLinuxEpollCtlDeleteFailed,
  kEventLoopLinuxEpollWaitFailed,

  kMainEventLoopLinuxServerSocketFailed,
  kMainEventLoopLinuxServerSocketBindFailed,
  kMainEventLoopLinuxServerSocketListenFailed,
  kMainEventLoopLinuxServerSocketAcceptFailed,
  kMainEventLoopLinuxUnexpectedFd,

  kLobbyEventLoopLinuxClientFdConversionFailed,
  kLobbyEventLoopLinuxClientFdAlreadyExists,

  kBattleEventLoopLinuxMatchedClientFdsNoComma,
  kBattleEventLoopLinuxClientFdConversionFailed,
  kBattleEventLoopLinuxClientFdAlreadyExists,
  kBattleEventLoopLinuxHandlerNotFound,

  kEngineLinuxClientSocketReadFailed,
  kEngineLinuxClientSocketWriteFailed,
  kEngineLinuxClientSocketClosed,
  kEngineLinuxClientSocketCloseFailed,

  kEngineLinuxSessionAlreadyExists,
  kEngineLinuxSessionNotFound,

  kEngineLinuxMessageParseFailed,

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

using Error = core::Error<Symbol>;

template <typename T> using Result = core::Result<T, Error>;

using SB = core::StringBuilder;

#endif // SERVER_COMMON_H
