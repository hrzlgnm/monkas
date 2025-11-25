#pragma once

#include <compare>
#include <optional>

#include <fmt/ostream.h>
#include <ip/Address.hpp>
#include <util/FlagSet.hpp>

namespace monkas::network
{
using Family = ip::Family;

enum class Scope : uint8_t
{
    Global,
    Site,
    Link,
    Host,
    Nowhere,
};
auto operator<<(std::ostream& o, Scope s) -> std::ostream&;

auto fromRtnlScope(uint8_t rtnlScope) -> Scope;

enum class AddressFlag : uint8_t
{
    Temporary,
    NoDuplicateAddressDetection,
    Optimistic,
    HomeAddress,
    DuplicateAddressDetectionFailed,
    Deprecated,
    Tentative,
    Permanent,
    ManagedTemporaryAddress,
    NoPrefixRoute,
    MulticastAutoJoin,
    StablePrivacy,
    FlagsCount,
};
using AddressFlags = util::FlagSet<AddressFlag>;

enum class AddressAssignmentProtocol : uint8_t
{
    Unspecified,
    KernelLoopback,
    KernelRouterAdvertisement,
    KernelLinkLocal,
};

class Address
{
  public:
    Address() = default;
    Address(const ip::Address& address,
            const std::optional<ip::Address>& broadcast,
            uint8_t prefixLen,
            Scope scope,
            const AddressFlags& flags,
            AddressAssignmentProtocol proto);

    [[nodiscard]] auto family() const -> Family;

    [[nodiscard]] auto isV4() const -> bool;
    [[nodiscard]] auto isV6() const -> bool;

    [[nodiscard]] auto ip() const -> const ip::Address&;
    [[nodiscard]] auto broadcast() const -> std::optional<ip::Address>;
    [[nodiscard]] auto prefixLength() const -> uint8_t;
    [[nodiscard]] auto scope() const -> Scope;
    [[nodiscard]] auto flags() const -> AddressFlags;
    [[nodiscard]] auto addressAssignmentProtocol() const -> AddressAssignmentProtocol;

    [[nodiscard]] auto operator<=>(const Address& other) const -> std::strong_ordering;
    [[nodiscard]] auto operator==(const Address& other) const -> bool = default;

  private:
    ip::Address m_ip;
    std::optional<ip::Address> m_brd;
    uint8_t m_prefixlen {};
    Scope m_scope {Scope::Nowhere};
    AddressFlags m_flags;
    AddressAssignmentProtocol m_prot {AddressAssignmentProtocol::Unspecified};
};

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&;
auto operator<<(std::ostream& o, AddressFlag a) -> std::ostream&;
auto operator<<(std::ostream& o, const AddressFlags& a) -> std::ostream&;
auto operator<<(std::ostream& o, AddressAssignmentProtocol a) -> std::ostream&;

}  // namespace monkas::network

template<>
struct fmt::formatter<monkas::network::Address> : ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::network::AddressFlag> : ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::network::AddressFlags> : ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::network::AddressAssignmentProtocol> : ostream_formatter
{
};
