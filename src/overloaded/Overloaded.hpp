#pragma once

// helper type for std::visit
template<typename... T>
struct Overloaded : T...
{
    using T::operator()...;
};

template<class... T>
Overloaded(T...) -> Overloaded<T...>;
