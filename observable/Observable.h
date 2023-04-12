#ifndef BROADCASTER_H
#define BROADCASTER_H

#include <forward_list>
#include <functional>
#include <list>
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
        for (const auto &observer : m_observers)
        {
            observer(std::forward<Args>(args)...);
        }
        m_broadCasting = false;
        for (const auto token : m_tokensToRemove)
        {
            removeListener(token);
        }
        m_tokensToRemove.clear();
    }

  private:
    Observers m_observers;
    std::forward_list<Token> m_tokensToRemove;
    bool m_broadCasting{false};
};

#endif // BROADCASTER_H
