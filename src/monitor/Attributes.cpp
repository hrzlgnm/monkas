#include <cstdint>

#include <libmnl/libmnl.h>
#include <linux/netlink.h>
#include <monitor/Attributes.hpp>
#include <spdlog/spdlog.h>
#include <sys/types.h>

namespace monkas::monitor
{
namespace
{

using Attrs = std::vector<const nlattr*>;

[[nodiscard]] auto has(const Attrs& attrs, const uint16_t type) -> bool
{
    return type < attrs.size() && attrs[type] != nullptr;
}

template<typename T>
[[nodiscard]] auto getTypedAttribute(const Attrs& attrs,
                                     uint16_t type,
                                     const mnl_attr_data_type mnlType,
                                     T (*getter)(const nlattr*)) -> std::optional<T>
{
    if (!has(attrs, type)) {
        return std::nullopt;
    }
    const auto* attr = attrs[type];
    if (mnl_attr_validate(attr, mnlType) < 0) {
        spdlog::warn("attribute of type {} is invalid", type);
        return std::nullopt;
    }
    return getter(attr);
}

template<std::size_t N>
[[nodiscard]] auto getPayload(const Attrs& attrs, uint16_t type) -> std::optional<std::array<uint8_t, N>>
{
    if (!has(attrs, type)) {
        return std::nullopt;
    }

    const auto* attr = attrs[type];

    if (mnl_attr_validate2(attr, MNL_TYPE_UNSPEC, N) < 0) {
        spdlog::trace("payload of type {} has len {} != {}", type, mnl_attr_get_payload_len(attr), N);
        return std::nullopt;
    }

    const auto* payload = static_cast<const uint8_t*>(mnl_attr_get_payload(attr));
    std::array<uint8_t, N> arr;
    std::copy_n(payload, N, arr.data());
    return arr;
}

}  // namespace

auto Attributes::parse(const nlmsghdr* n,
                       const uint32_t offset,
                       const uint16_t maxType,
                       uint64_t& seenCounter,
                       uint64_t& unknownCounter) -> Attributes
{
    Attributes attributes {maxType + 1U};
    CallbackArgs arg {.attrs = &attributes, .seenCounter = &seenCounter, .unknownCounter = &unknownCounter};
    mnl_attr_parse(n, offset, &Attributes::dispatchMnlAttributeCallback, &arg);
    return attributes;
}

Attributes::Attributes(const std::size_t toAlloc)
    : m_attributes(toAlloc, nullptr)
{
}

void Attributes::parseAttribute(const nlattr* a, uint64_t& seenCounter, uint64_t& unknownCounter)
{
    const auto type = mnl_attr_get_type(a);
    const auto maxType = static_cast<uint16_t>(m_attributes.size() - 1U);
    const auto typeValid = mnl_attr_type_valid(a, maxType);
    if (typeValid > 0) {
        seenCounter++;
        m_attributes[type] = a;
        return;
    }
    unknownCounter++;
    if (typeValid < 0) {
        const auto err = errno;
        spdlog::warn("failed to validate nlattr type 0x{:04x}: {}", type, std::strerror(err));
        return;
    }
    spdlog::warn("ignoring unexpected nlattr type {}", type);
}

auto Attributes::dispatchMnlAttributeCallback(const nlattr* attr, void* args) -> int
{
    const auto* cb = static_cast<CallbackArgs*>(args);
    cb->attrs->parseAttribute(attr, *cb->seenCounter, *cb->unknownCounter);
    return MNL_CB_OK;
}

auto Attributes::getString(const uint16_t type) const -> std::optional<std::string>
{
    return getTypedAttribute<const char*>(m_attributes, type, MNL_TYPE_STRING, mnl_attr_get_str)
        .transform([](const char* str) { return std::string(str); });
}

auto Attributes::getU8(const uint16_t type) const -> std::optional<uint8_t>
{
    return getTypedAttribute<uint8_t>(m_attributes, type, MNL_TYPE_U8, mnl_attr_get_u8);
}

auto Attributes::getU16(const uint16_t type) const -> std::optional<uint16_t>
{
    return getTypedAttribute<uint16_t>(m_attributes, type, MNL_TYPE_U16, mnl_attr_get_u16);
}

auto Attributes::getU32(const uint16_t type) const -> std::optional<uint32_t>
{
    return getTypedAttribute<uint32_t>(m_attributes, type, MNL_TYPE_U32, mnl_attr_get_u32);
}

auto Attributes::getU64(const uint16_t type) const -> std::optional<uint64_t>
{
    return getTypedAttribute<uint64_t>(m_attributes, type, MNL_TYPE_U64, mnl_attr_get_u64);
}

auto Attributes::getEthernetAddress(const uint16_t type) const -> std::optional<ethernet::Address>
{
    return getPayload<ethernet::ADDR_LEN>(m_attributes, type)
        .transform([](const auto& arr) { return ethernet::Address(arr); });
}

auto Attributes::getIpV6Address(const uint16_t type) const -> std::optional<ip::Address>
{
    return getPayload<ip::IPV6_ADDR_LEN>(m_attributes, type)
        .transform([](const auto& arr) { return ip::Address(arr); });
}

auto Attributes::getIpV4Address(const uint16_t type) const -> std::optional<ip::Address>
{
    return getPayload<ip::IPV4_ADDR_LEN>(m_attributes, type)
        .transform([](const auto& arr) { return ip::Address(arr); });
}
}  // namespace monkas::monitor
