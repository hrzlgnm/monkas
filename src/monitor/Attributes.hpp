// Copyright 2023-2025 hrzlgnm
// SPDX-License-Identifier: MIT-0

#pragma once
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
    static auto parse(const nlmsghdr* n,
                      uint32_t offset,
                      uint16_t maxType,
                      uint64_t& seenCounter,
                      uint64_t& unknownCounter) -> Attributes;

    ~Attributes() = default;
    Attributes(const Attributes&) = default;
    Attributes(Attributes&&) = default;
    auto operator=(const Attributes&) -> Attributes& = default;
    auto operator=(Attributes&&) -> Attributes& = default;

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

    struct CallbackArgs
    {
        Attributes* attrs;
        uint64_t* seenCounter;
        uint64_t* unknownCounter;
    };

    void parseAttribute(const nlattr* a, uint64_t& seenCounter, uint64_t& unknownCounter);
    static auto dispatchMnlAttributeCallback(const nlattr* attr, void* args) -> int;

    std::vector<const nlattr*> m_attributes;
};
}  // namespace monkas::monitor
