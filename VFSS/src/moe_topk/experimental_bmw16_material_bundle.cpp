#include <moe_topk/experimental_bmw16_material_bundle.h>

#include <moe_topk/protocol_i_parallel_shuffle.h>
#include <moe_topk/protocol_i_party_package.h>

#include <openssl/evp.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/random.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace moe_topk {
namespace {

constexpr std::size_t kMaxBundleBytes = 512U * 1024U * 1024U;
constexpr std::size_t kTagBytes = 16;
constexpr std::size_t kNonceBytes = 12;
constexpr std::size_t kHeaderBytes = 100;
constexpr std::array<std::uint8_t, 8> kEnvelopeMagic{'B','M','W','1','6','S','1','4'};
constexpr std::array<std::uint8_t, 8> kPayloadMagic{'B','M','W','1','6','P','1','4'};
using Digest = std::array<std::uint8_t, 32>;
using Key = std::array<std::uint8_t, 32>;
using Nonce = std::array<std::uint8_t, kNonceBytes>;
using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

void check(bool ok, const char* what) {
  if (!ok) throw std::runtime_error(what);
}

class Writer {
 public:
  std::vector<std::uint8_t> bytes;
  void u8(std::uint8_t x) { bytes.push_back(x); }
  void u32(std::uint32_t x) { for (int s=24;s>=0;s-=8) bytes.push_back(static_cast<std::uint8_t>(x>>s)); }
  void u64(std::uint64_t x) { for (int s=56;s>=0;s-=8) bytes.push_back(static_cast<std::uint8_t>(x>>s)); }
  void raw(const std::uint8_t* p, std::size_t n) {
    if (n == 0) return;
    if (n > kMaxBundleBytes || bytes.size() > kMaxBundleBytes - n) throw std::runtime_error("BMW16 bundle size limit");
    bytes.insert(bytes.end(), p, p+n);
  }
  void blob(const std::vector<std::uint8_t>& v) { u64(v.size()); raw(v.data(), v.size()); }
};

class Reader {
 public:
  explicit Reader(const std::vector<std::uint8_t>& in) : bytes(in) {
    if (in.size() > kMaxBundleBytes) throw std::runtime_error("BMW16 bundle size limit");
  }
  std::uint8_t u8() { need(1); return bytes[pos++]; }
  std::uint32_t u32() { need(4); std::uint32_t x=0; for(int i=0;i<4;++i)x=(x<<8)|bytes[pos++]; return x; }
  std::uint64_t u64() { need(8); std::uint64_t x=0; for(int i=0;i<8;++i)x=(x<<8)|bytes[pos++]; return x; }
  std::vector<std::uint8_t> raw(std::size_t n) { need(n); std::vector<std::uint8_t> v(bytes.begin()+pos,bytes.begin()+pos+n); pos+=n; return v; }
  std::vector<std::uint8_t> blob() {
    const auto n=u64(); if(n>kMaxBundleBytes || n>bytes.size()-pos)throw std::runtime_error("BMW16 bundle blob length");
    return raw(static_cast<std::size_t>(n));
  }
  void done() const { if(pos!=bytes.size())throw std::runtime_error("BMW16 bundle trailing bytes"); }
 private:
  void need(std::size_t n) const { if(pos>bytes.size()||n>bytes.size()-pos)throw std::runtime_error("BMW16 bundle truncated at "+std::to_string(pos)+" need="+std::to_string(n)+" size="+std::to_string(bytes.size())); }
  const std::vector<std::uint8_t>& bytes; std::size_t pos=0;
};

void random_bytes(std::uint8_t* out, std::size_t n) {
  while (n) {
    const auto got=::getrandom(out,n,0);
    if(got<0&&errno==EINTR)continue;
    check(got>0,"BMW16 bundle OS entropy");
    out+=got; n-=static_cast<std::size_t>(got);
  }
}

Digest sha256(const std::vector<std::uint8_t>& bytes) {
  Digest out{}; unsigned int n=0;
  check(EVP_Digest(bytes.data(),bytes.size(),out.data(),&n,EVP_sha256(),nullptr)==1&&n==out.size(),"BMW16 manifest SHA256");
  return out;
}

std::uint32_t expected_padded(std::uint32_t n) {
  if(n==0||n>256)throw std::invalid_argument("BMW16 bundle n outside S15 bounded domain");
  std::uint32_t p=2; while(p<n)p<<=1U; return p;
}

std::uint8_t expected_index_bits(std::uint32_t padded) {
  std::uint8_t bits=1; for(auto x=padded;x>2;x>>=1U)++bits; return bits;
}

void validate_config(const ProtocolIBmw16ExperimentalPartyConfig& c, std::uint8_t party) {
  if (party>1||c.party!=party||c.session==0||c.fingerprint==0||c.k==0||c.k>c.n||
      c.padded_n!=expected_padded(c.n)||c.index_bits!=expected_index_bits(c.padded_n)||
      c.comparison_bits!=33U+c.index_bits||c.comparison_bits>53||c.timeout_ms<=0)
    throw std::invalid_argument("BMW16 bundle configuration mismatch");
}

void add_slot(std::vector<ProtocolIBmw16MaterialSlot>& slots,
              ProtocolIBmw16MaterialStage stage,std::uint8_t task,std::uint8_t round,
              std::uint32_t left,std::uint32_t right) {
  if(slots.size()>=std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("BMW16 slot ID overflow");
  slots.push_back({static_cast<std::uint64_t>(slots.size()),stage,task,round,left,right});
}

std::vector<ProtocolIBmw16MaterialSlot> enumerate_slots(const ProtocolIBmw16ExperimentalPartyConfig& c) {
  validate_config(c,c.party);
  std::vector<ProtocolIBmw16MaterialSlot> slots;
  const auto pairs=static_cast<std::uint64_t>(c.n)*(c.n-1U)/2U;
  slots.reserve(static_cast<std::size_t>(2U*c.padded_n+2U+9U*pairs));
  for(auto stage:{ProtocolIBmw16MaterialStage::RawCarry,ProtocolIBmw16MaterialStage::RawSign})
    for(std::uint32_t i=0;i<c.padded_n;++i)add_slot(slots,stage,0xff,0,i,i);
  add_slot(slots,ProtocolIBmw16MaterialStage::ForwardShuffle,0xff,0,0,c.n-1U);
  add_slot(slots,ProtocolIBmw16MaterialStage::InverseShuffle,0xff,0,0,c.n-1U);
  for(std::uint8_t task=0;task<2;++task)for(std::uint8_t round=1;round<=4;++round)
    for(std::uint32_t i=0;i<c.n;++i)for(std::uint32_t j=i+1;j<c.n;++j)
      add_slot(slots,ProtocolIBmw16MaterialStage::Select,task,round,i,j);
  for(std::uint32_t i=0;i<c.n;++i)for(std::uint32_t j=i+1;j<c.n;++j)
    add_slot(slots,ProtocolIBmw16MaterialStage::Membership,0xff,1,i,j);
  return slots;
}

std::vector<std::uint8_t> encode_manifest(const std::vector<ProtocolIBmw16MaterialSlot>& slots) {
  Writer w; w.u64(slots.size());
  for(const auto& s:slots){w.u64(s.id);w.u8(static_cast<std::uint8_t>(s.stage));w.u8(s.task);w.u8(s.round);w.u8(0);w.u32(s.left);w.u32(s.right);}
  return std::move(w.bytes);
}

bool same_slot(const ProtocolIBmw16MaterialSlot& a,const ProtocolIBmw16MaterialSlot& b) {
  return a.id==b.id&&a.stage==b.stage&&a.task==b.task&&a.round==b.round&&a.left==b.left&&a.right==b.right;
}

void encode_score_package(Writer& w,const ProtocolIPartyPackage& p,std::uint8_t party,
                          const ProtocolIBmw16ExperimentalPartyConfig& c) {
  if(p.party!=party||p.session!=c.session||p.fingerprint!=c.fingerprint||p.n!=c.padded_n||
     p.k!=c.k||p.comparison_bits!=c.comparison_bits||p.carry_materials.size()!=c.padded_n||
     p.sign_materials.size()!=c.padded_n)throw std::invalid_argument("BMW16 raw material binding");
  w.u64(p.session);w.u64(p.fingerprint);w.u32(p.n);w.u32(p.k);w.u8(party);w.u8(p.comparison_bits);
  for(const auto* list:{&p.carry_materials,&p.sign_materials}){
    w.u32(static_cast<std::uint32_t>(list->size()));
    for(std::uint32_t i=0;i<list->size();++i){
      const auto& x=(*list)[i];
      if(x.slot!=i||x.stage!=(list==&p.carry_materials?1:2)||x.material.party_id()!=party||x.material.comparison_bits()!=34)
        throw std::invalid_argument("BMW16 raw slot identity");
      w.u32(x.slot);w.u8(x.stage);w.u8(0);w.u8(0);w.u8(0);w.u64(x.left_mask_share);w.u64(x.right_mask_share);w.blob(x.material.serialize());
    }
  }
}

ProtocolIPartyPackage decode_score_package(Reader& r,std::uint8_t party,
                                            const ProtocolIBmw16ExperimentalPartyConfig& c) {
  ProtocolIPartyPackage p; p.session=r.u64();p.fingerprint=r.u64();p.n=r.u32();p.k=r.u32();p.party=r.u8();p.comparison_bits=r.u8();
  if(p.session!=c.session||p.fingerprint!=c.fingerprint||p.n!=c.padded_n||p.k!=c.k||p.party!=party||p.comparison_bits!=c.comparison_bits)
    throw std::runtime_error("BMW16 raw bundle metadata mismatch");
  for(std::uint8_t stage=1;stage<=2;++stage){
    const auto count=r.u32();if(count!=c.padded_n)throw std::runtime_error("BMW16 raw slot count");
    auto& list=stage==1?p.carry_materials:p.sign_materials;list.reserve(count);
    for(std::uint32_t i=0;i<count;++i){
      const auto slot=r.u32();const auto got_stage=r.u8();(void)r.u8();(void)r.u8();(void)r.u8();
      const auto left=r.u64(),right=r.u64();auto key=ProtocolIUcmpPartyMaterial::deserialize(r.blob());
      if(slot!=i||got_stage!=stage||key.party_id()!=party||key.comparison_bits()!=34)throw std::runtime_error("BMW16 raw key slot mismatch");
      list.emplace_back(slot,stage,left,right,std::move(key));
    }
  }
  return p;
}

std::vector<std::uint8_t> encode_payload(const ProtocolIBmw16ExperimentalPartyConfig& c,
    std::uint8_t party,const ProtocolIBmw16ExperimentalPartyMaterial& m,
    const std::vector<ProtocolIBmw16MaterialSlot>& slots,std::vector<std::uint8_t>* manifest_out) {
  if(m.forward_shuffle.has_cmpagg_material||m.inverse_shuffle.has_cmpagg_material||
     !m.forward_shuffle.edge_materials.empty()||!m.inverse_shuffle.edge_materials.empty())
    throw std::invalid_argument("BMW16-S14 bundle requires shuffle-only material profile");
  Writer payload;payload.raw(kPayloadMagic.data(),kPayloadMagic.size());payload.u32(2);
  payload.u64(c.session);payload.u64(c.fingerprint);payload.u8(party);payload.u32(c.n);payload.u32(c.k);
  payload.u32(c.padded_n);payload.u8(c.index_bits);payload.u8(c.comparison_bits);
  const auto manifest=encode_manifest(slots);payload.blob(manifest);*manifest_out=manifest;
  Writer score;encode_score_package(score,m.score_input,party,c);payload.blob(score.bytes);
  payload.blob(protocol_i_parallel_shuffle_serialize_material(m.forward_shuffle));
  payload.blob(protocol_i_parallel_shuffle_serialize_material(m.inverse_shuffle));
  for(const auto& task:m.select_keys)for(const auto& round:task){
    if(round.size()!=static_cast<std::size_t>(c.n)*(c.n-1U)/2U)throw std::invalid_argument("BMW16 Select pool coverage");
    payload.u64(round.size());for(const auto& key:round){if(key.party_id()!=party||key.comparison_bits()!=c.comparison_bits)throw std::invalid_argument("BMW16 Select key binding");payload.blob(key.serialize());}
  }
  if(m.membership_keys.size()!=static_cast<std::size_t>(c.n)*(c.n-1U)/2U)throw std::invalid_argument("BMW16 membership coverage");
  payload.u64(m.membership_keys.size());for(const auto& key:m.membership_keys){if(key.party_id()!=party||key.comparison_bits()!=c.comparison_bits)throw std::invalid_argument("BMW16 membership key binding");payload.blob(key.serialize());}
  if(payload.bytes.size()>kMaxBundleBytes)throw std::runtime_error("BMW16 payload size limit");
  return std::move(payload.bytes);
}

ProtocolIBmw16ExperimentalPartyMaterial decode_payload(const ProtocolIBmw16ExperimentalPartyConfig& c,
    std::uint8_t party,const std::vector<std::uint8_t>& payload,const std::vector<ProtocolIBmw16MaterialSlot>& expected_slots) {
  Reader r(payload);if(r.raw(kPayloadMagic.size())!=std::vector<std::uint8_t>(kPayloadMagic.begin(),kPayloadMagic.end())||r.u32()!=2)
    throw std::runtime_error("BMW16 payload version");
  if(r.u64()!=c.session||r.u64()!=c.fingerprint||r.u8()!=party||r.u32()!=c.n||r.u32()!=c.k||r.u32()!=c.padded_n||r.u8()!=c.index_bits||r.u8()!=c.comparison_bits)
    throw std::runtime_error("BMW16 payload parameters");
  const auto manifest_bytes=r.blob();Reader mr(manifest_bytes);const auto count=mr.u64();if(count!=expected_slots.size())throw std::runtime_error("BMW16 slot manifest count");
  std::vector<ProtocolIBmw16MaterialSlot> slots;slots.reserve(static_cast<std::size_t>(count));
  for(std::size_t i=0;i<count;++i){ProtocolIBmw16MaterialSlot s;s.id=mr.u64();s.stage=static_cast<ProtocolIBmw16MaterialStage>(mr.u8());s.task=mr.u8();s.round=mr.u8();(void)mr.u8();s.left=mr.u32();s.right=mr.u32();if(!same_slot(s,expected_slots[i]))throw std::runtime_error("BMW16 manifest tuple mismatch");slots.push_back(s);}mr.done();
  ProtocolIBmw16ExperimentalPartyMaterial m;m.slot_manifest=std::move(slots);
  const auto score_blob=r.blob();Reader sr(score_blob);m.score_input=decode_score_package(sr,party,c);sr.done();
  m.forward_shuffle=protocol_i_parallel_shuffle_deserialize_material(r.blob(),party);
  m.inverse_shuffle=protocol_i_parallel_shuffle_deserialize_material(r.blob(),party);
  if(m.forward_shuffle.has_cmpagg_material||m.inverse_shuffle.has_cmpagg_material||
     !m.forward_shuffle.edge_materials.empty()||!m.inverse_shuffle.edge_materials.empty())
    throw std::runtime_error("BMW16-S14 shuffle-only material profile mismatch");
  const auto pairs=static_cast<std::uint64_t>(c.n)*(c.n-1U)/2U;
  for(auto& task:m.select_keys)for(auto& round:task){if(r.u64()!=pairs)throw std::runtime_error("BMW16 Select serialized count");round.reserve(static_cast<std::size_t>(pairs));for(std::uint64_t i=0;i<pairs;++i){auto key=ProtocolIUcmpPartyMaterial::deserialize(r.blob());if(key.party_id()!=party||key.comparison_bits()!=c.comparison_bits)throw std::runtime_error("BMW16 Select serialized key binding");round.push_back(std::move(key));}}
  if(r.u64()!=pairs)throw std::runtime_error("BMW16 membership serialized count");m.membership_keys.reserve(static_cast<std::size_t>(pairs));
  for(std::uint64_t i=0;i<pairs;++i){auto key=ProtocolIUcmpPartyMaterial::deserialize(r.blob());if(key.party_id()!=party||key.comparison_bits()!=c.comparison_bits)throw std::runtime_error("BMW16 membership serialized binding");m.membership_keys.push_back(std::move(key));}
  r.done();
  if(m.forward_shuffle.party!=party||m.inverse_shuffle.party!=party||m.forward_shuffle.session!=c.session||m.inverse_shuffle.session!=c.session||m.forward_shuffle.n!=c.n||m.inverse_shuffle.n!=c.n||m.forward_shuffle.comparison_bits!=c.comparison_bits||m.inverse_shuffle.comparison_bits!=c.comparison_bits)
    throw std::runtime_error("BMW16 shuffle bundle binding");
  return m;
}

std::vector<std::uint8_t> make_header(const ProtocolIBmw16ExperimentalPartyConfig& c,
    std::uint8_t party,const Digest& digest,const Nonce& nonce,std::uint64_t plain_len) {
  Writer w;w.raw(kEnvelopeMagic.data(),kEnvelopeMagic.size());w.u32(2);w.u8(party);w.u8(0);w.u8(0);w.u8(0);
  w.u64(c.session);w.u64(c.fingerprint);w.u32(c.n);w.u32(c.k);w.u32(c.padded_n);w.u8(c.index_bits);w.u8(c.comparison_bits);w.u8(0);w.u8(0);
  w.raw(digest.data(),digest.size());w.raw(nonce.data(),nonce.size());w.u64(plain_len);
  check(w.bytes.size()==kHeaderBytes,"BMW16 envelope header size");return std::move(w.bytes);
}

std::vector<std::uint8_t> encrypt(const Key& key,const Nonce& nonce,const std::vector<std::uint8_t>& aad,const std::vector<std::uint8_t>& plain) {
  check(plain.size()<=static_cast<std::size_t>(std::numeric_limits<int>::max()),"BMW16 AEAD plaintext bound");
  CipherContext ctx(EVP_CIPHER_CTX_new(),EVP_CIPHER_CTX_free);check(bool(ctx),"BMW16 AEAD context");
  check(EVP_EncryptInit_ex(ctx.get(),EVP_aes_256_gcm(),nullptr,nullptr,nullptr)==1&&EVP_CIPHER_CTX_ctrl(ctx.get(),EVP_CTRL_GCM_SET_IVLEN,nonce.size(),nullptr)==1&&EVP_EncryptInit_ex(ctx.get(),nullptr,nullptr,key.data(),nonce.data())==1,"BMW16 AES-GCM init");
  int n=0;check(EVP_EncryptUpdate(ctx.get(),nullptr,&n,aad.data(),static_cast<int>(aad.size()))==1,"BMW16 AEAD AAD");
  std::vector<std::uint8_t> out(plain.size()+kTagBytes);int written=0;check(EVP_EncryptUpdate(ctx.get(),out.data(),&written,plain.data(),static_cast<int>(plain.size()))==1&&static_cast<std::size_t>(written)==plain.size(),"BMW16 AEAD encrypt");
  int tail=0;check(EVP_EncryptFinal_ex(ctx.get(),out.data()+written,&tail)==1&&tail==0,"BMW16 AEAD final");
  check(EVP_CIPHER_CTX_ctrl(ctx.get(),EVP_CTRL_GCM_GET_TAG,kTagBytes,out.data()+plain.size())==1,"BMW16 AEAD tag");return out;
}

std::vector<std::uint8_t> decrypt(const Key& key,const Nonce& nonce,const std::vector<std::uint8_t>& aad,const std::vector<std::uint8_t>& encrypted) {
  if(encrypted.size()<kTagBytes||encrypted.size()-kTagBytes>static_cast<std::size_t>(std::numeric_limits<int>::max()))throw std::runtime_error("BMW16 AEAD length");
  const auto length=encrypted.size()-kTagBytes;CipherContext ctx(EVP_CIPHER_CTX_new(),EVP_CIPHER_CTX_free);check(bool(ctx),"BMW16 AEAD context");
  check(EVP_DecryptInit_ex(ctx.get(),EVP_aes_256_gcm(),nullptr,nullptr,nullptr)==1&&EVP_CIPHER_CTX_ctrl(ctx.get(),EVP_CTRL_GCM_SET_IVLEN,nonce.size(),nullptr)==1&&EVP_DecryptInit_ex(ctx.get(),nullptr,nullptr,key.data(),nonce.data())==1,"BMW16 AES-GCM init");
  int n=0;check(EVP_DecryptUpdate(ctx.get(),nullptr,&n,aad.data(),static_cast<int>(aad.size()))==1,"BMW16 AEAD AAD");
  std::vector<std::uint8_t> plain(length);int written=0;check(EVP_DecryptUpdate(ctx.get(),plain.data(),&written,encrypted.data(),static_cast<int>(length))==1&&static_cast<std::size_t>(written)==length,"BMW16 AEAD decrypt");
  auto tag=const_cast<std::uint8_t*>(encrypted.data()+length);check(EVP_CIPHER_CTX_ctrl(ctx.get(),EVP_CTRL_GCM_SET_TAG,kTagBytes,tag)==1,"BMW16 AEAD set tag");
  int tail=0;check(EVP_DecryptFinal_ex(ctx.get(),plain.data()+written,&tail)==1&&tail==0,"BMW16 AEAD authentication failed");return plain;
}

void write_all(int fd,const std::uint8_t* p,std::size_t n) { while(n){auto v=::write(fd,p,n);if(v<0&&errno==EINTR)continue;check(v>0,"BMW16 atomic write");p+=v;n-=static_cast<std::size_t>(v);} }

void durable_marker(int directory,std::string name,const std::vector<std::uint8_t>& body) {
  const int fd=::openat(directory,name.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
  if(fd<0)throw std::runtime_error(errno==EEXIST?"BMW16 persistent claim replay":"BMW16 persistent claim create");
  bool okay=true;try{write_all(fd,body.data(),body.size());okay=::fdatasync(fd)==0;}catch(...){::close(fd);throw;}
  okay=::close(fd)==0&&okay;okay=(::fsync(directory)==0)&&okay;check(okay,"BMW16 persistent claim sync");
}

}  // namespace

std::vector<ProtocolIBmw16MaterialSlot> protocol_i_bmw16_enumerate_material_slots(
    const ProtocolIBmw16ExperimentalPartyConfig& config) {
  return enumerate_slots(config);
}

std::vector<std::uint8_t> protocol_i_bmw16_bundle_seal_party(
    const ProtocolIBmw16ExperimentalPartyConfig& c,std::uint8_t party,
    const ProtocolIBmw16ExperimentalPartyMaterial& material,const Key& key,
    ProtocolIBmw16BundleStats* stats) {
  validate_config(c,party);const auto slots=enumerate_slots(c);std::vector<std::uint8_t> manifest;
  const auto plain=encode_payload(c,party,material,slots,&manifest);const auto digest=sha256(manifest);Nonce nonce{};random_bytes(nonce.data(),nonce.size());
  const auto header=make_header(c,party,digest,nonce,plain.size());const auto encrypted=encrypt(key,nonce,header,plain);
  std::vector<std::uint8_t> out=header;out.insert(out.end(),encrypted.begin(),encrypted.end());
  if(stats){stats->plaintext_bytes=plain.size();stats->manifest_bytes=manifest.size();stats->ciphertext_bytes=encrypted.size();stats->envelope_bytes=out.size();stats->authenticated_metadata_bytes=header.size()+kTagBytes;stats->slot_count=slots.size();stats->manifest_sha256=digest;}
  return out;
}

ProtocolIBmw16ExperimentalPartyMaterial protocol_i_bmw16_bundle_open_party(
    const ProtocolIBmw16ExperimentalPartyConfig& expected,std::uint8_t party,
    const std::vector<std::uint8_t>& envelope,const Key& key,ProtocolIBmw16BundleStats* stats) {
  validate_config(expected,party);if(envelope.size()<kHeaderBytes+kTagBytes||envelope.size()>kMaxBundleBytes)throw std::runtime_error("BMW16 envelope length");
  const std::vector<std::uint8_t> header_bytes(envelope.begin(),envelope.begin()+kHeaderBytes);Reader h(header_bytes);
  if(h.raw(kEnvelopeMagic.size())!=std::vector<std::uint8_t>(kEnvelopeMagic.begin(),kEnvelopeMagic.end())||h.u32()!=2)throw std::runtime_error("BMW16 envelope version");
  const auto got_party=h.u8();(void)h.u8();(void)h.u8();(void)h.u8();const auto session=h.u64(),fingerprint=h.u64();const auto n=h.u32(),k=h.u32(),padded=h.u32();const auto index=h.u8(),bits=h.u8();(void)h.u8();(void)h.u8();
  Digest digest{};auto d=h.raw(digest.size());std::copy(d.begin(),d.end(),digest.begin());Nonce nonce{};auto no=h.raw(nonce.size());std::copy(no.begin(),no.end(),nonce.begin());const auto plain_len=h.u64();h.done();
  if(got_party!=party||session!=expected.session||fingerprint!=expected.fingerprint||n!=expected.n||k!=expected.k||padded!=expected.padded_n||index!=expected.index_bits||bits!=expected.comparison_bits||plain_len!=envelope.size()-kHeaderBytes-kTagBytes)throw std::runtime_error("BMW16 envelope identity mismatch");
  const std::vector<std::uint8_t> header(envelope.begin(),envelope.begin()+kHeaderBytes), encrypted(envelope.begin()+kHeaderBytes,envelope.end());
  auto plain=decrypt(key,nonce,header,encrypted);
  // Validate the authenticated manifest digest before constructing any key objects.
  Reader r(plain);(void)r.raw(kPayloadMagic.size());(void)r.u32();(void)r.u64();(void)r.u64();(void)r.u8();(void)r.u32();(void)r.u32();(void)r.u32();(void)r.u8();(void)r.u8();
  auto manifest=r.blob();if(sha256(manifest)!=digest)throw std::runtime_error("BMW16 authenticated manifest digest mismatch");
  const auto slots=enumerate_slots(expected);auto material=decode_payload(expected,party,plain,slots);material.slot_manifest=slots;
  if(stats){stats->plaintext_bytes=plain.size();stats->manifest_bytes=manifest.size();stats->ciphertext_bytes=encrypted.size();stats->envelope_bytes=envelope.size();stats->authenticated_metadata_bytes=kHeaderBytes+kTagBytes;stats->slot_count=slots.size();stats->manifest_sha256=digest;}
  return material;
}

void protocol_i_bmw16_bundle_write_atomic(const std::string& path,const std::vector<std::uint8_t>& bytes) {
  if(bytes.size()>kMaxBundleBytes||path.empty())throw std::invalid_argument("BMW16 bundle output bounds");
  const auto temp=path+".tmp."+std::to_string(::getpid());const int fd=::open(temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
  if(fd<0)throw std::runtime_error("BMW16 bundle temp create");bool okay=true;
  try{write_all(fd,bytes.data(),bytes.size());okay=::fdatasync(fd)==0;}catch(...){::close(fd);::unlink(temp.c_str());throw;}
  okay=::close(fd)==0&&okay;if(!okay){::unlink(temp.c_str());throw std::runtime_error("BMW16 bundle sync");}
  if(::rename(temp.c_str(),path.c_str())!=0){::unlink(temp.c_str());throw std::runtime_error("BMW16 bundle atomic rename");}
  const auto parent=std::filesystem::path(path).parent_path();const int dir=::open(parent.empty()?".":parent.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
  if(dir<0)throw std::runtime_error("BMW16 bundle parent open");okay=::fsync(dir)==0;::close(dir);check(okay,"BMW16 bundle directory sync");
}

std::vector<std::uint8_t> protocol_i_bmw16_bundle_read(const std::string& path) {
  const int fd=::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)throw std::runtime_error("BMW16 bundle open");
  struct stat st{};if(::fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||st.st_size<0||static_cast<std::uint64_t>(st.st_size)>kMaxBundleBytes){::close(fd);throw std::runtime_error("BMW16 bundle file identity/size");}
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(st.st_size));std::size_t off=0;
  while(off<bytes.size()){const auto n=::read(fd,bytes.data()+off,bytes.size()-off);if(n<0&&errno==EINTR)continue;if(n<=0){::close(fd);throw std::runtime_error("BMW16 bundle file truncated");}off+=static_cast<std::size_t>(n);}
  ::close(fd);return bytes;
}

ProtocolIBmw16PersistentClaimStore::ProtocolIBmw16PersistentClaimStore(const std::string& directory,std::uint64_t session,std::uint8_t party)
    :session_(session),party_(party){if(!session||party>1)throw std::invalid_argument("BMW16 claim identity");directory_fd_=::open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);if(directory_fd_<0)throw std::runtime_error("BMW16 claim directory");struct stat st{};if(::fstat(directory_fd_,&st)!=0||!S_ISDIR(st.st_mode)||(st.st_mode&077)!=0){::close(directory_fd_);directory_fd_=-1;throw std::runtime_error("BMW16 claim directory permissions");}}
ProtocolIBmw16PersistentClaimStore::ProtocolIBmw16PersistentClaimStore(ProtocolIBmw16PersistentClaimStore&& x) noexcept:directory_fd_(x.directory_fd_),session_(x.session_),party_(x.party_){x.directory_fd_=-1;}
ProtocolIBmw16PersistentClaimStore& ProtocolIBmw16PersistentClaimStore::operator=(ProtocolIBmw16PersistentClaimStore&& x) noexcept {if(this!=&x){if(directory_fd_>=0)::close(directory_fd_);directory_fd_=x.directory_fd_;session_=x.session_;party_=x.party_;x.directory_fd_=-1;}return *this;}
ProtocolIBmw16PersistentClaimStore::~ProtocolIBmw16PersistentClaimStore(){if(directory_fd_>=0)::close(directory_fd_);}
void ProtocolIBmw16PersistentClaimStore::claim_bundle(const Digest& digest){Writer w;w.u64(session_);w.u8(party_);w.raw(digest.data(),digest.size());durable_marker(directory_fd_,"bundle-"+std::to_string(session_)+".claim",w.bytes);}

ProtocolIBmw16ProcessSlotClaimSet::ProtocolIBmw16ProcessSlotClaimSet()
    : owner_process_id_(static_cast<std::uint64_t>(::getpid())) {}

void ProtocolIBmw16ProcessSlotClaimSet::claim_once(std::uint64_t slot_id) {
  if (owner_process_id_ != static_cast<std::uint64_t>(::getpid()))
    throw ProtocolIBmw16ExpectedFailure(ProtocolIBmw16ExpectedFailureKind::Material,
                                        "BMW16 slot claim guard cannot be reused after fork");
  std::lock_guard<std::mutex> lock(mutex_);
  if (!claimed_.insert(slot_id).second)
    throw ProtocolIBmw16ExpectedFailure(ProtocolIBmw16ExpectedFailureKind::Material,
                                        "BMW16 duplicate in-process slot claim");
}

std::vector<std::uint64_t> ProtocolIBmw16ProcessSlotClaimSet::snapshot() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::uint64_t> result(claimed_.begin(), claimed_.end());
  std::sort(result.begin(), result.end());
  return result;
}

}  // namespace moe_topk
