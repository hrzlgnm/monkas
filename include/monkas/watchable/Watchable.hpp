#pragma once

#include <algorithm>
#include <exception>
#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <utility>

#include <spdlog/spdlog.h>

namespace monkas
{
/** NOT thread-safe; only call from one thread */
template<typename... Args>
class Watchable
{
  public:
    Watchable() = default;
    ~Watchable() = default;
    Watchable(const Watchable&) = delete;
    auto operator=(const Watchable&) -> Watchable& = delete;
    auto operator=(Watchable&&) -> Watchable& = delete;
    Watchable(Watchable&&) = delete;

    using Watcher = std::function<void(Args...)>;
    using Watchers = std::list<Watcher>;
    using Token = typename Watchers::const_iterator;

    [[nodiscard]] auto addWatcher(const Watcher& watcher) -> Token
    {
        return m_watchers.insert(m_watchers.end(), watcher);
    }

    /**
     * @brief Removes a watcher identified by the given token.
     *
     * If called during notification, the removal is deferred until after notification completes. If the token is
     * invalid or the watcher is already marked for removal, the operation is ignored.
     */
    void removeWatcher(const Token& token)
    {
        if (!tokenIsValid(token)) {
            spdlog::error("Attempted to remove a watcher with an invalid token, ignoring");
            return;
        }
        if (m_notifying) {
            spdlog::debug("Attempted to remove a watcher while notifying, will be removed after notifying");
            if (!toBeRemoved(token)) {
                m_tokensToRemove.push_front(token);
            } else {
                spdlog::warn("Watcher with token {} is already marked for removal, ignoring",
                             std::distance(m_watchers.cbegin(), token));
            }
            return;
        }
        m_watchers.erase(token);
    }

    [[nodiscard]] auto hasWatchers() const -> bool { return !m_watchers.empty(); }

    /**
     * @brief Notifies all registered watchers with the provided arguments.
     *
     * Calls each watcher callback with the given arguments. Watchers marked for removal are skipped. Any exceptions
     * thrown by watcher callbacks are caught and logged without interrupting the notification process. Deferred
     * removals are processed after all notifications are complete.
     */
    void notify(Args... args)
    {
        {
            struct Guard
            {
                bool& flag;

                Guard(const Guard&) = delete;
                Guard(Guard&&) = delete;
                auto operator=(const Guard&) -> Guard& = delete;
                auto operator=(Guard&&) -> Guard& = delete;

                explicit Guard(bool& f)
                    : flag(f)
                {
                    flag = true;
                }

                ~Guard() { flag = false; }
            } guard(m_notifying);

            for (auto itr {std::cbegin(m_watchers)}; itr != std::cend(m_watchers); ++itr) {
                if (toBeRemoved(itr)) {
                    spdlog::trace("Skipping watcher with token: {} as it is marked for removal",
                                  std::distance(m_watchers.cbegin(), itr));
                    continue;
                }
                try {
                    spdlog::trace("Calling watcher with token: {}", std::distance(m_watchers.cbegin(), itr));
                    (*itr)(std::forward<Args>(args)...);
                } catch (const std::exception& e) {
                    spdlog::error("Caught an unexpected exception from watcher: {}", e.what());
                } catch (...) {
                    spdlog::error("Caught an unexpected and unknown exception from watcher");
                }
            }
        }
        for (const auto token : m_tokensToRemove) {
            spdlog::trace("Removing marked for removal watcher with token: {}",
                          std::distance(m_watchers.cbegin(), token));
            removeWatcher(token);
        }
        m_tokensToRemove.clear();
    }

  private:
    auto toBeRemoved(const Token& token) const -> bool
    {
        return std::find(std::cbegin(m_tokensToRemove), std::cend(m_tokensToRemove), token)
            != std::cend(m_tokensToRemove);
    }

    auto tokenIsValid(const Token& token) const -> bool
    {
        return token != m_watchers.end()
            && std::any_of(m_watchers.cbegin(), m_watchers.cend(), [&](const auto& w) { return &w == &(*token); });
    }

    Watchers m_watchers;
    std::forward_list<Token> m_tokensToRemove;
    bool m_notifying {false};
};
}  // namespace monkas
