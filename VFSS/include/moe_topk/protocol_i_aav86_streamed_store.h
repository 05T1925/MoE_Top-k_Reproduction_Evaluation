#pragma once

#include <moe_topk/protocol_i_aav86_small.h>

#include <array>
#include <cstdint>
#include <string>

namespace moe_topk {

struct ProtocolIStreamDeliveryBytes {
  std::uint64_t party0 = 0, party1 = 0;
};

// E20_STREAM_AEAD_V1. The two descriptors must be private, intact offline
// T->party channels. The function sends every canonical edge before the base
// bundle and returns only after both deliveries finish.
ProtocolIStreamDeliveryBytes protocol_i_aav86_stream_dealer_send(
    const ProtocolIAav86SmallConfig& config, int party0_fd, int party1_fd);

// E21_CLIQUE_MINIMAL_SEALED_V1. Generates only full-clique node masks,
// score carry/sign materials, and every canonical comparison key. No AAV86
// permutation, translation, or pivot state is generated or delivered.
ProtocolIStreamDeliveryBytes protocol_i_clique_minimal_dealer_send(
    const ProtocolIAav86SmallConfig& config, int party0_fd, int party1_fd);

class ProtocolIAav86StreamedPartyMaterial {
 public:
  ProtocolIAav86StreamedPartyMaterial() = default;
  ~ProtocolIAav86StreamedPartyMaterial();
  ProtocolIAav86StreamedPartyMaterial(const ProtocolIAav86StreamedPartyMaterial&) = delete;
  ProtocolIAav86StreamedPartyMaterial& operator=(const ProtocolIAav86StreamedPartyMaterial&) = delete;
  ProtocolIAav86StreamedPartyMaterial(ProtocolIAav86StreamedPartyMaterial&&) noexcept;
  ProtocolIAav86StreamedPartyMaterial& operator=(
      ProtocolIAav86StreamedPartyMaterial&&) noexcept;

  ProtocolIAav86SmallPartyMaterial base;
  std::uint64_t disk_bytes() const { return disk_bytes_; }
  std::uint64_t plaintext_payload_bytes() const { return plaintext_payload_bytes_; }
  const std::string& path() const { return path_; }

  // Atomically and durably consumes this complete material before any online
  // input-dependent message. A second claim, including after a crash, fails.
  void claim(const ProtocolIAav86SmallConfig& config);

  ProtocolIUcmpPartyMaterial read_edge(
      std::uint32_t iteration, std::uint32_t a, std::uint32_t c,
      std::uint64_t material_id);

 private:
  friend ProtocolIAav86StreamedPartyMaterial
  protocol_i_aav86_stream_receive_party(const ProtocolIAav86SmallConfig&,
                                       int, const std::string&);
  friend ProtocolIAav86StreamedPartyMaterial
  protocol_i_clique_minimal_receive_party(const ProtocolIAav86SmallConfig&,
                                         int, const std::string&);
  friend ProtocolIAav86StreamedPartyMaterial
  protocol_i_stream_receive_impl(const ProtocolIAav86SmallConfig&,
                                 int, const std::string&, bool);
  friend ProtocolIAav86SmallOutput protocol_i_aav86_stream_party_from_store(
      const ProtocolIAav86SmallConfig&,
      ProtocolIAav86StreamedPartyMaterial&&,
      const std::vector<std::uint32_t>&,const std::array<int,2>&,
      const std::vector<int>&,int);
  int fd_ = -1;
  std::array<std::uint8_t,32> key_{};
  std::array<std::uint8_t,80> header_{};
  std::uint64_t disk_bytes_ = 0, plaintext_payload_bytes_ = 0;
  std::uint32_t record_bytes_ = 0, key_bytes_ = 0;
  bool claimed_ = false;
  std::vector<bool> read_slots_;
  std::string path_;
};

// Each party creates its own AEAD key from OS entropy, never stores that key,
// and atomically seals the full file before returning ready to the controller.
ProtocolIAav86StreamedPartyMaterial protocol_i_aav86_stream_receive_party(
    const ProtocolIAav86SmallConfig& config, int dealer_fd,
    const std::string& private_directory);
ProtocolIAav86StreamedPartyMaterial protocol_i_clique_minimal_receive_party(
    const ProtocolIAav86SmallConfig& config, int dealer_fd,
    const std::string& private_directory);

ProtocolIAav86SmallOutput protocol_i_aav86_stream_party_from_store(
    const ProtocolIAav86SmallConfig& config,
    ProtocolIAav86StreamedPartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const std::array<int,2>& score_fds,
    const std::vector<int>& core_fds, int inverse_fd);

}  // namespace moe_topk
