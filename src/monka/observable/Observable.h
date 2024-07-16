#pragma once

#include <algorithm>
#include <exception>
#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <spdlog/spdlog.h>
#include <utility>

template <typename... Args> class Observable
{
  public:
    Observable() = default;
    using Observer = std::function<void(Args...)>;
    using Observers = std::list<Observer>;
    using Token = typename Observers::const_iterator;

    Token addListener(const Observer &observer)
    {
        return m_observers.insert(m_observers.end(), observer);
    }
    void removeListener(const Token &token)
    {
        if (m_broadCasting)
        {
            m_tokensToRemove.push_front(token);
            return;
        }
        m_observers.erase(token);
    }

    void broadcast(Args... args)
    {
        m_broadCasting = true;
        for (auto itr{std::cbegin(m_observers)}; itr != std::cend(m_observers); ++itr)
        {
            if (std::find(std::cbegin(m_tokensToRemove), std::cend(m_tokensToRemove), itr) ==
                std::cend(m_tokensToRemove))
            {
                try
                {
                    (*itr)(std::forward<Args>(args)...);
                }
                catch (const std::exception &e)
                {
                    spdlog::error("Caught an unexpected exception from observer: {}", e.what());
                }
            }
        }
        m_broadCasting = false;
        for (const auto token : m_tokensToRemove)
        {
            removeListener(token);
        }
        m_tokensToRemove.clear();
    }

  private:
    Observable(const Observable &) = delete;
    Observable &operator=(const Observable &) = delete;
    Observable(Observable &&) = delete;

    Observers m_observers;
    std::forward_list<Token> m_tokensToRemove;
    bool m_broadCasting{false};
};
