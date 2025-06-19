#pragma once
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <libmnl/libmnl.h>
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

    [[nodiscard]] auto getStr(uint16_t type) const -> const char *;

    [[nodiscard]] auto hasAttribute(uint16_t type) const -> bool
    {
        return type < m_attributes.size() && m_attributes[type] != nullptr;
    }

    void applyU8(uint16_t type, const std::function<void(uint8_t)> &callback) const;
    void applyU16(uint16_t type, const std::function<void(uint16_t)> &callback) const;
    void applyU32(uint16_t type, const std::function<void(uint32_t)> &callback) const;

    template <std::size_t N>
    void applyPayload(uint16_t type, const std::function<void(const std::array<uint8_t, N>)> &callback) const
    {
        if (type >= m_attributes.size() || m_attributes[type] == nullptr)
        {
            return;
        }
        if (mnl_attr_validate2(m_attributes[type], MNL_TYPE_UNSPEC, N) < 0)
        {
            spdlog::trace("payload of type {} has len {} != {}", type, mnl_attr_get_payload_len(m_attributes[type]), N);
            return;
        }

        const auto *payload = static_cast<const uint8_t *>(mnl_attr_get_payload(m_attributes[type]));
        std::array<uint8_t, N> arr;
        std::copy_n(payload, N, arr.data());
        callback(arr);
    }

  private:
    explicit Attributes(std::size_t toAlloc);

    template <typename T>
    void applyValue(uint16_t type, mnl_attr_data_type mnlType, const std::function<void(T)> &callback,
                    T (*getter)(const nlattr *), const char *typeName) const
    {
        if (type >= m_attributes.size() || m_attributes[type] == nullptr)
        {
            spdlog::trace("{} attribute with type {} does not exist", typeName, type);
            return;
        }
        if (mnl_attr_validate(m_attributes[type], mnlType) < 0)
        {
            spdlog::warn("{} attribute is invalid for type {}", typeName, type);
            return;
        }
        callback(getter(m_attributes[type]));
    }

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
