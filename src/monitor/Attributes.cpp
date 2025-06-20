#include "ethernet/Address.hpp"
#include <libmnl/libmnl.h>
#include <monitor/Attributes.hpp>
#include <spdlog/spdlog.h>
#include <sys/types.h>

namespace monkas::monitor
{

auto Attributes::parse(const nlmsghdr *n, size_t offset, uint16_t maxType, u_int64_t &counter) -> Attributes
{
    Attributes attributes{maxType + 1U};
    CallbackArgs arg{.attrs = &attributes, .counter = &counter};
    mnl_attr_parse(n, offset, &Attributes::dispatchMnlAttributeCallback, &arg);
    return attributes;
}

Attributes::Attributes(std::size_t toAlloc)
    : m_attributes(toAlloc, nullptr)
{
}

void Attributes::parseAttribute(const nlattr *a, uint64_t &counter)
{
    const auto type = mnl_attr_get_type(a);
    if (mnl_attr_type_valid(a, m_attributes.size() - 1) > 0)
    {
        counter++;
        m_attributes[type] = a;
    }
    else
    {
        spdlog::warn("ignoring unexpected nlattr type {}", type);
    }
}

auto Attributes::dispatchMnlAttributeCallback(const nlattr *attr, void *args) -> int
{
    auto *cb = static_cast<CallbackArgs *>(args);
    cb->attrs->parseAttribute(attr, *cb->counter);
    return MNL_CB_OK;
}

auto Attributes::has(uint16_t type) const -> bool
{
    return type < m_attributes.size() && m_attributes[type] != nullptr;
}

auto Attributes::getString(uint16_t type) const -> std::optional<std::string>
{
    return getTypedAttribute<const char *>(type, MNL_TYPE_STRING, mnl_attr_get_str).transform([](const char *str) {
        return std::string(str);
    });
}

auto Attributes::getU8(uint16_t type) const -> std::optional<uint8_t>
{
    return getTypedAttribute<uint8_t>(type, MNL_TYPE_U8, mnl_attr_get_u8);
}

auto Attributes::getU16(uint16_t type) const -> std::optional<uint16_t>
{
    return getTypedAttribute<uint16_t>(type, MNL_TYPE_U16, mnl_attr_get_u16);
}

auto Attributes::getU32(uint16_t type) const -> std::optional<uint32_t>
{
    return getTypedAttribute<uint32_t>(type, MNL_TYPE_U32, mnl_attr_get_u32);
}

auto Attributes::getU64(uint16_t type) const -> std::optional<uint64_t>
{
    return getTypedAttribute<uint64_t>(type, MNL_TYPE_U64, mnl_attr_get_u64);
}

auto Attributes::getEthernetAddress(uint16_t type) const -> std::optional<ethernet::Address>
{
    return getPayload<ethernet::ADDR_LEN>(type).transform(
        [](const auto &arr) { return ethernet::Address::fromBytes(arr); });
}

auto Attributes::getIpV6Address(uint16_t type) const -> std::optional<ip::Address>
{
    return getPayload<ip::IPV6_ADDR_LEN>(type).transform([](const auto &arr) { return ip::Address::fromBytes(arr); });
}

auto Attributes::getIpV4Address(uint16_t type) const -> std::optional<ip::Address>
{
    return getPayload<ip::IPV4_ADDR_LEN>(type).transform([](const auto &arr) { return ip::Address::fromBytes(arr); });
}
} // namespace monkas::monitor
