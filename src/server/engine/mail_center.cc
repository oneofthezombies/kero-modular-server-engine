#include "mail_center.h"

#include <mutex>

#include "core/spsc_channel.h"
#include "core/tiny_json.h"

#include "common.h"

using namespace engine;

engine::Mail::Mail(std::string &&from, std::string &&to,
                   MailBody &&body) noexcept
    : from{std::move(from)}, to{std::move(to)}, body{std::move(body)} {}

auto engine::Mail::Clone() const noexcept -> Mail {
  auto from = this->from;
  auto to = this->to;
  auto body = this->body.Clone();
  return Mail{std::move(from), std::move(to), std::move(body)};
}

engine::MailBox::MailBox(core::Tx<Mail> &&tx, core::Rx<Mail> &&rx) noexcept
    : tx(std::move(tx)), rx(std::move(rx)) {}

engine::MailCenter::MailCenter(core::Tx<MailBody> &&run_tx) noexcept
    : run_tx_{std::move(run_tx)} {}

auto engine::MailCenter::Shutdown() noexcept -> void {
  run_tx_.Send(std::move(core::TinyJson{}.Set("shutdown", "")));
  run_thread_.join();
}

auto engine::MailCenter::Create(std::string &&name) noexcept
    -> Result<MailBox> {
  using ResultT = Result<MailBox>;

  if (auto res = ValidateName(name); res.IsErr()) {
    return ResultT{std::move(res.Err())};
  }

  {
    std::lock_guard lock{mutex_};
    if (const auto it = mail_boxes_.find(name); it != mail_boxes_.end()) {
      return ResultT{Error{Symbol::kMailBoxAlreadyExists,
                           core::TinyJson{}.Set("name", name).ToString()}};
    }

    auto [from_peer_tx, to_office_rx] = core::Channel<Mail>::Builder{}.Build();
    auto [from_office_tx, to_peer_rx] = core::Channel<Mail>::Builder{}.Build();

    mail_boxes_.emplace(std::move(name),
                        MailBox{core::Tx<Mail>{std::move(from_office_tx)},
                                core::Rx<Mail>{std::move(to_office_rx)}});

    return ResultT{MailBox{
        core::Tx<Mail>{std::move(from_peer_tx)},
        core::Rx<Mail>{std::move(to_peer_rx)},
    }};
  }
}

auto engine::MailCenter::Delete(std::string &&name) noexcept -> Result<Void> {
  using ResultT = Result<Void>;

  std::lock_guard lock{mutex_};
  if (const auto it = mail_boxes_.find(name); it == mail_boxes_.end()) {
    return ResultT{Error{Symbol::kMailBoxNotFound,
                         core::TinyJson{}.Set("name", name).ToString()}};
  }

  mail_boxes_.erase(name);
  return ResultT{Void{}};
}

auto engine::MailCenter::ValidateName(
    const std::string_view name) const noexcept -> Result<Void> {
  using ResultT = Result<Void>;

  if (name.empty()) {
    return ResultT{Error{Symbol::kMailBoxNameEmpty,
                         core::TinyJson{}.Set("name", name).ToString()}};
  }

  if (name.size() > 64) {
    return ResultT{Error{Symbol::kMailBoxNameTooLong,
                         core::TinyJson{}.Set("name", name).ToString()}};
  }

  // "all" is reserved for broadcast
  if (name == "all") {
    return ResultT{Error{Symbol::kMailBoxNameAllReserved,
                         core::TinyJson{}.Set("name", name).ToString()}};
  }

  return ResultT{Void{}};
}

auto engine::MailCenter::RunOnThread(core::Rx<MailBody> &&run_rx) noexcept
    -> void {
  while (true) {
    auto run_event = run_rx.TryReceive();
    if (run_event) {
      if (auto value = run_event->Get("shutdown"); value) {
        break;
      }
    }

    {
      std::lock_guard lock{mutex_};
      for (auto &[name, mail_box] : mail_boxes_) {
        auto mail = mail_box.rx.TryReceive();
        if (!mail) {
          continue;
        }

        // broadcast
        if (mail->to == "all") {
          for (auto &[name, other_mail_box] : mail_boxes_) {
            if (name == mail->from) {
              continue;
            }
            other_mail_box.tx.Send(mail->Clone());
          }
          continue;
        }

        // unicast
        const auto to = mail_boxes_.find(mail->to);
        if (to == mail_boxes_.end()) {
          core::TinyJson{}
              .Set("reason", "MailBox not found")
              .Set("to", mail->to)
              .LogLn();
          continue;
        }

        to->second.tx.Send(std::move(*mail));
      }
    }
  }
}

auto engine::MailCenter::StartRunThread(core::Rx<MailBody> &&run_rx) noexcept
    -> void {
  run_thread_ = std::thread{RunThreadMain, std::ref(*this), std::move(run_rx)};
}

auto engine::MailCenter::RunThreadMain(MailCenter &mail_center,
                                       core::Rx<MailBody> &&run_rx) noexcept
    -> void {
  mail_center.RunOnThread(std::move(run_rx));
}

auto engine::MailCenter::Global() noexcept -> MailCenter & {
  static std::unique_ptr<MailCenter> instance{nullptr};
  static std::once_flag flag;
  std::call_once(flag, [] {
    auto [tx, rx] = core::Channel<MailBody>::Builder{}.Build();
    instance.reset(new MailCenter{std::move(tx)});
    instance->StartRunThread(std::move(rx));
  });
  return *instance;
}