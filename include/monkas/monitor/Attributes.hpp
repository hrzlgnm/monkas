#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <ethernet/Address.hpp>
#include <ip/Address.hpp>
#include <libmnl/libmnl.h>
#include <spdlog/spdlog.h>

namespace monkas::monitor
{

class Attributes
{
  public:
    static auto parse(const nlmsghdr* n, size_t offset, uint16_t maxType, uint64_t& counter) -> Attributes;

    ~Attributes() = default;
    Attributes(const Attributes&) = default;
    Attributes(Attributes&&) = default;
    auto operator=(const Attributes&) -> Attributes& = default;
    auto operator=(Attributes&&) -> Attributes& = default;

    [[nodiscard]] auto has(uint16_t type) const -> bool;
    [[nodiscard]] auto getString(uint16_t type) const -> std::optional<std::string>;
    [[nodiscard]] auto getU8(uint16_t type) const -> std::optional<uint8_t>;
    [[nodiscard]] auto getU16(uint16_t type) const -> std::optional<uint16_t>;
    [[nodiscard]] auto getU32(uint16_t type) const -> std::optional<uint32_t>;
    [[nodiscard]] auto getU64(uint16_t type) const -> std::optional<uint64_t>;
    [[nodiscard]] auto getEthernetAddress(uint16_t type) const -> std::optional<ethernet::Address>;
    [[nodiscard]] auto getIpV4Address(uint16_t type) const -> std::optional<ip::Address>;
    [[nodiscard]] auto getIpV6Address(uint16_t type) const -> std::optional<ip::Address>;

  private:
    explicit Attributes(std::size_t toAlloc);

    template<typename T>
    auto getTypedAttribute(uint16_t type, mnl_attr_data_type mnlType, T (*getter)(const nlattr*)) const
        -> std::optional<T>
    {
        if (!has(type)) {
            return std::nullopt;
        }
        if (mnl_attr_validate(m_attributes[type], mnlType) < 0) {
            spdlog::warn("attribute of type {} is invalid", type);
            return std::nullopt;
        }
        return getter(m_attributes[type]);
    }

    template<std::size_t N>
    [[nodiscard]] auto getPayload(uint16_t type) const -> std::optional<std::array<uint8_t, N>>
    {
        if (!has(type)) {
            return std::nullopt;
        }

        if (mnl_attr_validate2(m_attributes[type], MNL_TYPE_UNSPEC, N) < 0) {
            spdlog::trace("payload of type {} has len {} != {}", type, mnl_attr_get_payload_len(m_attributes[type]), N);
            return std::nullopt;
        }

        const auto* payload = static_cast<const uint8_t*>(mnl_attr_get_payload(m_attributes[type]));
        std::array<uint8_t, N> arr;
        std::copy_n(payload, N, arr.data());
        return arr;
    }

    struct CallbackArgs
    {
        Attributes* attrs;
        uint64_t* counter;
    };

    void parseAttribute(const nlattr* a, uint64_t& counter);
    static auto dispatchMnlAttributeCallback(const nlattr* attr, void* args) -> int;

    std::vector<const nlattr*> m_attributes;
};
}  // namespace monkas::monitor
