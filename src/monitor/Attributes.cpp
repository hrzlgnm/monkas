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
        m_attributes.at(type) = a;
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
    if (!has(type))
    {
        return std::nullopt;
    }
    if (mnl_attr_validate(m_attributes[type], MNL_TYPE_STRING) < 0)
    {
        spdlog::warn("attribute of type {} is invalid", type);
        return std::nullopt;
    }
    return mnl_attr_get_str(m_attributes[type]);
}

auto Attributes::getU64(uint16_t type) const -> std::optional<uint64_t>
{
    if (!has(type))
    {
        return std::nullopt;
    }
    if (mnl_attr_validate(m_attributes[type], MNL_TYPE_U64) < 0)
    {
        spdlog::warn("attribute of type {} is invalid", type);
        return std::nullopt;
    }
    return mnl_attr_get_u64(m_attributes[type]);
}

auto Attributes::getU8(uint16_t type) const -> std::optional<uint8_t>
{
    if (!has(type))
    {
        return std::nullopt;
    }
    if (mnl_attr_validate(m_attributes[type], MNL_TYPE_U8) < 0)
    {
        spdlog::warn("attribute of type {} is invalid", type);
        return std::nullopt;
    }
    return mnl_attr_get_u8(m_attributes[type]);
}

auto Attributes::getU16(uint16_t type) const -> std::optional<uint16_t>
{
    if (!has(type))
    {
        return std::nullopt;
    }
    if (mnl_attr_validate(m_attributes[type], MNL_TYPE_U16) < 0)
    {
        spdlog::warn("attribute of type {} is invalid", type);
        return std::nullopt;
    }
    return mnl_attr_get_u16(m_attributes[type]);
}

auto Attributes::getU32(uint16_t type) const -> std::optional<uint32_t>
{
    if (!has(type))
    {
        return std::nullopt;
    }
    if (mnl_attr_validate(m_attributes[type], MNL_TYPE_U32) < 0)
    {
        spdlog::warn("attribute of type {} is invalid", type);
        return std::nullopt;
    }
    return mnl_attr_get_u32(m_attributes[type]);
}

} // namespace monkas::monitor
