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
    if (mnl_attr_type_valid(a, m_attributes.size()) > 0)
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

auto Attributes::getStr(uint16_t type) const -> const char *
{
    if (type >= m_attributes.size() || m_attributes[type] == nullptr)
    {
        spdlog::trace("str attribute with type {} does not exist", type);
        return nullptr;
    }
    if (mnl_attr_validate(m_attributes[type], MNL_TYPE_STRING) < 0)
    {
        spdlog::warn("str attribute is invalid for type {}", type);
        return nullptr;
    }
    return mnl_attr_get_str(m_attributes[type]);
}

void Attributes::applyU8(uint16_t type, const std::function<void(uint8_t)> &callback) const
{
    applyValue<uint8_t>(type, MNL_TYPE_U8, callback, mnl_attr_get_u8, "u8");
}

void Attributes::applyU16(uint16_t type, const std::function<void(uint16_t)> &callback) const
{
    applyValue<uint16_t>(type, MNL_TYPE_U16, callback, mnl_attr_get_u16, "u16");
}

void Attributes::applyU32(uint16_t type, const std::function<void(uint32_t)> &callback) const
{
    applyValue<uint32_t>(type, MNL_TYPE_U32, callback, mnl_attr_get_u32, "u32");
}

} // namespace monkas::monitor
