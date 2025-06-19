#pragma once
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <libmnl/libmnl.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <vector>

namespace monkas::monitor
{

class Attributes
{
  public:
    static auto parse(const nlmsghdr *n, size_t offset, uint16_t maxType, uint64_t &counter) -> Attributes;

    ~Attributes() = default;
    Attributes(const Attributes &) = default;
    Attributes(Attributes &&) = default;
    auto operator=(const Attributes &) -> Attributes & = default;
    auto operator=(Attributes &&) -> Attributes & = default;

    [[nodiscard]] auto has(uint16_t type) const -> bool;
    [[nodiscard]] auto getString(uint16_t type) const -> std::optional<std::string>;
    [[nodiscard]] auto getU64(uint16_t type) const -> std::optional<uint64_t>;
    [[nodiscard]] auto getU8(uint16_t type) const -> std::optional<uint8_t>;
    [[nodiscard]] auto getU16(uint16_t type) const -> std::optional<uint16_t>;
    [[nodiscard]] auto getU32(uint16_t type) const -> std::optional<uint32_t>;

    template <std::size_t N> [[nodiscard]] auto getPayload(uint16_t type) const -> std::optional<std::array<uint8_t, N>>
    {
        if (!has(type))
        {
            return std::nullopt;
        }

        if (mnl_attr_validate2(m_attributes[type], MNL_TYPE_UNSPEC, N) < 0)
        {
            spdlog::trace("payload of type {} has len {} != {}", type, mnl_attr_get_payload_len(m_attributes[type]), N);
            return std::nullopt;
        }

        const auto *payload = static_cast<const uint8_t *>(mnl_attr_get_payload(m_attributes[type]));
        std::array<uint8_t, N> arr;
        std::copy_n(payload, N, arr.data());
        return arr;
    }

  private:
    explicit Attributes(std::size_t toAlloc);

    struct CallbackArgs
    {
        Attributes *attrs;
        uint64_t *counter;
    };

    void parseAttribute(const nlattr *a, uint64_t &counter);
    static auto dispatchMnlAttributeCallback(const nlattr *attr, void *args) -> int;

    std::vector<const nlattr *> m_attributes;
};
} // namespace monkas::monitor
