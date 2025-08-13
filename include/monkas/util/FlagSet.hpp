#pragma once
#include <bitset>
#include <compare>
#include <sstream>
#include <type_traits>
#include <utility>

namespace monkas::util
{
template<typename Enum>
class FlagSet
{
    static_assert(std::is_scoped_enum_v<Enum>, "Template parameter must be a scoped enum");
    static_assert(std::is_integral_v<std::underlying_type_t<Enum>>, "Enum must have an integral underlying type");
    static_assert(requires { Enum::FlagsCount; }, "Enum must define FlagsCount enumerator");
    static_assert(std::to_underlying(Enum::FlagsCount) <= 32, "FlagsCount must not exceed 32");
    static constexpr size_t FLAG_COUNT = static_cast<size_t>(Enum::FlagsCount);

  public:
    using EnumType = Enum;

    FlagSet() = default;

    explicit FlagSet(uint32_t bits)
        : m_flags(bits)
    {
    }

    [[nodiscard]] constexpr auto toU32() const -> uint32_t { return m_flags.to_ulong(); }

    [[nodiscard]] constexpr static auto size() -> size_t { return FLAG_COUNT; }

    [[nodiscard]] constexpr auto count() const -> size_t { return m_flags.count(); }

    [[nodiscard]] constexpr auto none() const -> bool { return m_flags.none(); }

    [[nodiscard]] constexpr auto any() const -> bool { return m_flags.any(); }

    auto set(EnumType flag) -> void { m_flags.set(std::to_underlying(flag)); }

    auto reset(EnumType flag) -> void { m_flags.reset(std::to_underlying(flag)); }

    auto reset() -> void { m_flags.reset(); }

    [[nodiscard]] auto test(EnumType flag) const -> bool { return m_flags.test(std::to_underlying(flag)); }

    [[nodiscard]] auto toString() const -> std::string
    {
        std::ostringstream oss;
        bool first = true;
        for (size_t i = 0; i < FLAG_COUNT; ++i) {
            if (m_flags.test(i)) {
                if (!first) {
                    oss << "|";
                }
                first = false;
                oss << static_cast<Enum>(i);
            }
        }
        if (first) {
            return "None";
        }
        return oss.str();
    }

    [[nodiscard]] auto operator<=>(const FlagSet& other) const -> std::strong_ordering
    {
        return m_flags.to_ulong() <=> other.m_flags.to_ulong();
    }

    [[nodiscard]] auto operator==(const FlagSet& other) const -> bool = default;

  private:
    std::bitset<FLAG_COUNT> m_flags {};
};
}  // namespace monkas::util
