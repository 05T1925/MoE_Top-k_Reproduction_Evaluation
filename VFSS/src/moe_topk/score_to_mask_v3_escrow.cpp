#include <moe_topk/score_to_mask_v3_gmw.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <unistd.h>

namespace moe_topk {
namespace {

constexpr std::uint8_t kEscrowMessageType = 0x73;
constexpr std::uint8_t kReadyPhase = 1;
constexpr std::uint8_t kRequestPhase = 2;
constexpr std::uint8_t kOpSubmit = 1;
constexpr std::uint8_t kOpFetch = 2;
constexpr std::uint8_t kStatusAbort = 0;
constexpr std::uint8_t kStatusCommitted = 1;
constexpr std::uint8_t kEnvelopeVersion = 2;
constexpr std::uint32_t kMessageMagic = 0x53324d41U;  // S2MA
constexpr std::array<std::uint8_t, 8> kCipherMagic =
    {'S','2','M','E','N','C','3',1};

void fail(const std::string& message) { throw std::runtime_error(message); }

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value >> 8U));
  out.push_back(static_cast<std::uint8_t>(value));
}
void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  out.push_back(static_cast<std::uint8_t>(value >> 24U));
  out.push_back(static_cast<std::uint8_t>(value >> 16U));
  out.push_back(static_cast<std::uint8_t>(value >> 8U));
  out.push_back(static_cast<std::uint8_t>(value));
}
std::uint16_t read_u16(const std::vector<std::uint8_t>& in, std::size_t& at) {
  if (at + 2 > in.size()) fail("v3 escrow message truncated u16");
  const auto value = static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(in[at]) << 8U) | in[at + 1U]);
  at += 2;
  return value;
}
std::uint32_t read_u32(const std::vector<std::uint8_t>& in, std::size_t& at) {
  if (at + 4 > in.size()) fail("v3 escrow message truncated u32");
  std::uint32_t value = 0;
  for (int i = 0; i < 4; ++i) value = (value << 8U) | in[at++];
  return value;
}

std::array<std::uint8_t, 32> sha256(const std::uint8_t* bytes, std::size_t size) {
  std::array<std::uint8_t, 32> result{};
  if (SHA256(bytes, size, result.data()) == nullptr) fail("v3 escrow SHA-256 failed");
  return result;
}
std::array<std::uint8_t, 32> sha256(const std::vector<std::uint8_t>& bytes) {
  return sha256(bytes.data(), bytes.size());
}
std::string hex(const std::uint8_t* bytes, std::size_t size) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(size * 2U);
  for (std::size_t i = 0; i < size; ++i) {
    result.push_back(digits[bytes[i] >> 4U]);
    result.push_back(digits[bytes[i] & 0xfU]);
  }
  return result;
}

std::string transaction_id(const ScoreToMaskV3PackageInfo& package) {
  return "s2m-v3-" + hex(package.parameters.session.data(),
                         package.parameters.session.size()) + "-" +
         hex(package.parameters.invocation.data(),
             package.parameters.invocation.size());
}

std::string transaction_id(const ScoreToMaskV3EscrowContext& context) {
  return "s2m-v3-" + hex(context.session.data(), context.session.size()) + "-" +
         hex(context.invocation.data(), context.invocation.size());
}

ProtocolIFrameConfig escrow_frame_config(
    const ScoreToMaskV3EscrowContext& context, std::uint8_t party, bool server) {
  if (party > 1) fail("v3 escrow invalid party");
  auto frame = score_to_mask_v3_escrow_frame_config(context, party, server);
  frame.phase = kReadyPhase;
  return frame;
}

ProtocolIFrameConfig escrow_frame_config(
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party, bool server) {
  if (party > 1) fail("v3 escrow invalid party");
  auto party_package = package;
  party_package.party = party;
  auto frame = score_to_mask_v3_frame_config(party_package, party);
  frame.sender = server ? 2 : party;
  frame.receiver = server ? party : 2;
  frame.phase = kReadyPhase;
  return frame;
}

std::vector<std::uint8_t> make_request(
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party,
    std::uint8_t operation, const std::string& transaction,
    const std::vector<std::uint8_t>& ciphertext) {
  if (transaction.empty() || transaction.size() > 128 ||
      ciphertext.size() > kScoreToMaskV3MaxCiphertextBytes)
    fail("v3 escrow request size or transaction id invalid");
  std::vector<std::uint8_t> out;
  append_u32(out, kMessageMagic);
  out.push_back(kEnvelopeVersion);
  out.push_back(operation);
  out.push_back(party);
  out.push_back(0);
  out.insert(out.end(), package.parameters.session.begin(), package.parameters.session.end());
  out.insert(out.end(), package.parameters.invocation.begin(), package.parameters.invocation.end());
  out.insert(out.end(), package.circuit_sha256.begin(), package.circuit_sha256.end());
  out.insert(out.end(), package.gate_schedule_sha256.begin(),
             package.gate_schedule_sha256.end());
  out.insert(out.end(), package.package_sha256.begin(), package.package_sha256.end());
  append_u16(out, static_cast<std::uint16_t>(transaction.size()));
  append_u32(out, static_cast<std::uint32_t>(ciphertext.size()));
  out.insert(out.end(), transaction.begin(), transaction.end());
  out.insert(out.end(), ciphertext.begin(), ciphertext.end());
  return out;
}

struct Request {
  std::uint8_t operation = 0;
  std::uint8_t party = 0;
  std::string transaction;
  std::vector<std::uint8_t> ciphertext;
};

Request parse_request(const std::vector<std::uint8_t>& in,
                      const ScoreToMaskV3EscrowContext& context,
                      std::uint8_t expected_party) {
  constexpr std::size_t fixed = 4 + 4 + 16 + 16 + 32 + 32 + 32 + 2 + 4;
  if (in.size() < fixed) fail("v3 escrow request truncated");
  std::size_t at = 0;
  if (read_u32(in, at) != kMessageMagic || in[at++] != kEnvelopeVersion)
    fail("v3 escrow request magic/version");
  Request request;
  request.operation = in[at++];
  request.party = in[at++];
  if (in[at++] != 0) fail("v3 escrow reserved field");
  if (request.party != expected_party) fail("v3 escrow TLS identity/party mismatch");
  if (!std::equal(context.session.begin(), context.session.end(),
                  in.begin() + static_cast<std::ptrdiff_t>(at)))
    fail("v3 escrow session mismatch");
  at += context.session.size();
  if (!std::equal(context.invocation.begin(), context.invocation.end(),
                  in.begin() + static_cast<std::ptrdiff_t>(at)))
    fail("v3 escrow invocation mismatch");
  at += context.invocation.size();
  if (!std::equal(context.circuit_sha256.begin(), context.circuit_sha256.end(),
                  in.begin() + static_cast<std::ptrdiff_t>(at)))
    fail("v3 escrow circuit hash mismatch");
  at += context.circuit_sha256.size();
  if (!std::equal(context.gate_schedule_sha256.begin(),
                  context.gate_schedule_sha256.end(),
                  in.begin() + static_cast<std::ptrdiff_t>(at)))
    fail("v3 escrow gate schedule hash mismatch");
  at += context.gate_schedule_sha256.size();
  if (!std::equal(context.party_package_sha256[expected_party].begin(),
                  context.party_package_sha256[expected_party].end(),
                  in.begin() + static_cast<std::ptrdiff_t>(at)))
    fail("v3 escrow party package/context binding mismatch");
  at += context.party_package_sha256[expected_party].size();
  const auto transaction_size = read_u16(in, at);
  const auto ciphertext_size = read_u32(in, at);
  if (transaction_size == 0 || transaction_size > 128 ||
      ciphertext_size > context.state.max_ciphertext_bytes ||
      at + transaction_size + ciphertext_size != in.size())
    fail("v3 escrow request length mismatch");
  request.transaction.assign(in.begin() + static_cast<std::ptrdiff_t>(at),
                             in.begin() + static_cast<std::ptrdiff_t>(at + transaction_size));
  at += transaction_size;
  request.ciphertext.assign(in.begin() + static_cast<std::ptrdiff_t>(at), in.end());
  if (request.transaction != transaction_id(context))
    fail("v3 escrow transaction id mismatch");
  if (request.operation == kOpSubmit) {
     if (request.ciphertext.size() < 8 + 2 + 4 + 12 + 16 + 256 + context.n)
      fail("v3 escrow encrypted share is too short");
    if (!std::equal(kCipherMagic.begin(), kCipherMagic.end(), request.ciphertext.begin()))
      fail("v3 escrow received a non-v3 recipient ciphertext");
  } else if (request.operation == kOpFetch) {
    if (!request.ciphertext.empty()) fail("v3 escrow fetch has an unexpected payload");
  } else {
    fail("v3 escrow operation is not supported");
  }
  return request;
}

void sqlite_check(int status, sqlite3* db, const char* context) {
  if (status != SQLITE_OK && status != SQLITE_DONE && status != SQLITE_ROW)
    fail(std::string(context) + ": " + (db ? sqlite3_errmsg(db) : sqlite3_errstr(status)));
}

std::unique_ptr<sqlite3, decltype(&sqlite3_close)> open_database(
    const std::string& path) {
  sqlite3* raw = nullptr;
  const auto status = sqlite3_open_v2(path.c_str(), &raw,
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
  if (status != SQLITE_OK) {
    const std::string message = raw ? sqlite3_errmsg(raw) : sqlite3_errstr(status);
    if (raw) sqlite3_close(raw);
    fail("open v3 escrow SQLite database: " + message);
  }
  std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db(raw, sqlite3_close);
  sqlite3_busy_timeout(db.get(), 10000);
  sqlite_check(sqlite3_exec(db.get(),
      "PRAGMA journal_mode=WAL;PRAGMA synchronous=FULL;PRAGMA foreign_keys=ON;"
      "CREATE TABLE IF NOT EXISTS transactions("
      "transaction_id TEXT PRIMARY KEY,session BLOB NOT NULL,invocation BLOB NOT NULL,"
       "n INTEGER NOT NULL,k INTEGER NOT NULL,circuit_hash BLOB NOT NULL,"
       "gate_schedule_hash BLOB NOT NULL,context_hash BLOB NOT NULL,"
       "package_set_hash BLOB NOT NULL,state INTEGER NOT NULL,receipt BLOB);"
      "CREATE TABLE IF NOT EXISTS shares("
      "transaction_id TEXT NOT NULL,party INTEGER NOT NULL CHECK(party IN(0,1)),"
      "ciphertext BLOB NOT NULL,cipher_hash BLOB NOT NULL,"
      "PRIMARY KEY(transaction_id,party),"
      "FOREIGN KEY(transaction_id) REFERENCES transactions(transaction_id));",
      nullptr, nullptr, nullptr), db.get(), "initialize v3 escrow schema");
  return db;
}

std::array<std::uint8_t, 32> make_receipt(
    const ScoreToMaskV3EscrowContext& context, const std::string& transaction,
    const std::array<std::uint8_t, 32>& hash0,
    const std::array<std::uint8_t, 32>& hash1) {
  if (context.state.receipt_format_version == kScoreToMaskV3ReceiptFormatV2) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL
    std::array<std::uint8_t, 32> token{};
    if (RAND_bytes(token.data(), static_cast<int>(token.size())) != 1)
      fail("v3 opaque receipt/v2 CSPRNG failed");
    return token;
#else
    fail("v3 opaque receipt/v2 is not enabled in this build");
#endif
  }
  if (context.state.receipt_format_version != kScoreToMaskV3ReceiptFormatV1)
    fail("v3 escrow receipt format is unsupported");
  static constexpr std::array<std::uint8_t, 32> domain = {
      's','c','o','r','e','-','t','o','-','m','a','s','k','-','v','3','/',
      'e','s','c','r','o','w','/','c','o','m','m','i','t','/','1'};
  std::vector<std::uint8_t> bytes(domain.begin(), domain.end());
  bytes.insert(bytes.end(), transaction.begin(), transaction.end());
  bytes.insert(bytes.end(), context.session.begin(), context.session.end());
  bytes.insert(bytes.end(), context.invocation.begin(), context.invocation.end());
  append_u32(bytes, context.n);
  append_u32(bytes, context.k);
  bytes.insert(bytes.end(), context.circuit_sha256.begin(), context.circuit_sha256.end());
  bytes.insert(bytes.end(), context.gate_schedule_sha256.begin(),
               context.gate_schedule_sha256.end());
  bytes.insert(bytes.end(), context.context_sha256.begin(), context.context_sha256.end());
  bytes.insert(bytes.end(), context.package_set_sha256.begin(),
               context.package_set_sha256.end());
  bytes.insert(bytes.end(), hash0.begin(), hash0.end());
  bytes.insert(bytes.end(), hash1.begin(), hash1.end());
  return sha256(bytes);
}

struct CommitState {
  int state = 0;  // 0 pending, 1 committed, 2 aborted
  std::array<std::uint8_t, 32> receipt{};
};

CommitState query_state(const std::string& db_path,
                        const std::string& transaction) {
  auto db = open_database(db_path);
  sqlite3_stmt* raw = nullptr;
  sqlite_check(sqlite3_prepare_v2(db.get(),
      "SELECT state,receipt FROM transactions WHERE transaction_id=?1",
      -1, &raw, nullptr), db.get(), "prepare v3 escrow status query");
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> stmt(raw, sqlite3_finalize);
  sqlite_check(sqlite3_bind_text(stmt.get(), 1, transaction.c_str(), -1, SQLITE_TRANSIENT),
               db.get(), "bind v3 escrow transaction status");
  const int step = sqlite3_step(stmt.get());
  if (step == SQLITE_DONE) return {2, {}};
  sqlite_check(step, db.get(), "query v3 escrow status");
  CommitState result;
  result.state = sqlite3_column_int(stmt.get(), 0);
  if (result.state == 1) {
    const auto* receipt = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt.get(), 1));
    const auto size = sqlite3_column_bytes(stmt.get(), 1);
    if (receipt == nullptr || size != static_cast<int>(result.receipt.size()))
      fail("v3 escrow commit receipt is corrupt");
    std::copy_n(receipt, result.receipt.size(), result.receipt.begin());
  }
  return result;
}

CommitState submit_ciphertext(const std::string& db_path,
                              const ScoreToMaskV3EscrowContext& context,
                              const Request& request) {
  auto db = open_database(db_path);
  sqlite_check(sqlite3_exec(db.get(), "BEGIN IMMEDIATE", nullptr, nullptr, nullptr),
               db.get(), "begin v3 escrow transaction");
  try {
    sqlite3_stmt* raw = nullptr;
    sqlite_check(sqlite3_prepare_v2(db.get(),
        "INSERT OR IGNORE INTO transactions(transaction_id,session,invocation,n,k,"
        "circuit_hash,gate_schedule_hash,context_hash,package_set_hash,state) "
        "VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,0)",
        -1, &raw, nullptr), db.get(), "prepare v3 escrow transaction row");
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> insert(raw, sqlite3_finalize);
    sqlite_check(sqlite3_bind_text(insert.get(), 1, request.transaction.c_str(), -1, SQLITE_TRANSIENT),
                 db.get(), "bind v3 transaction id");
    sqlite_check(sqlite3_bind_blob(insert.get(), 2, context.session.data(), 16, SQLITE_TRANSIENT),
                 db.get(), "bind v3 escrow session");
    sqlite_check(sqlite3_bind_blob(insert.get(), 3, context.invocation.data(), 16, SQLITE_TRANSIENT),
                 db.get(), "bind v3 escrow invocation");
    sqlite_check(sqlite3_bind_int(insert.get(), 4, static_cast<int>(context.n)), db.get(), "bind v3 N");
    sqlite_check(sqlite3_bind_int(insert.get(), 5, static_cast<int>(context.k)), db.get(), "bind v3 K");
    sqlite_check(sqlite3_bind_blob(insert.get(), 6, context.circuit_sha256.data(), 32, SQLITE_TRANSIENT),
                 db.get(), "bind v3 circuit hash");
    sqlite_check(sqlite3_bind_blob(insert.get(), 7, context.gate_schedule_sha256.data(), 32, SQLITE_TRANSIENT),
                 db.get(), "bind v3 gate schedule hash");
    sqlite_check(sqlite3_bind_blob(insert.get(), 8, context.context_sha256.data(), 32, SQLITE_TRANSIENT),
                 db.get(), "bind v3 escrow context hash");
    sqlite_check(sqlite3_bind_blob(insert.get(), 9, context.package_set_sha256.data(), 32, SQLITE_TRANSIENT),
                 db.get(), "bind v3 package set hash");
    sqlite_check(sqlite3_step(insert.get()), db.get(), "insert v3 escrow transaction");

    sqlite3_stmt* check_raw = nullptr;
    sqlite_check(sqlite3_prepare_v2(db.get(),
        "SELECT state,session,invocation,n,k,circuit_hash,gate_schedule_hash,"
        "context_hash,package_set_hash,receipt "
        "FROM transactions WHERE transaction_id=?1", -1, &check_raw, nullptr),
        db.get(), "prepare v3 escrow transaction validation");
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> check(check_raw, sqlite3_finalize);
    sqlite_check(sqlite3_bind_text(check.get(), 1, request.transaction.c_str(), -1, SQLITE_TRANSIENT),
                 db.get(), "bind v3 transaction lookup");
    sqlite_check(sqlite3_step(check.get()), db.get(), "read v3 transaction metadata");
    const auto column_blob_matches = [&](int column, const std::uint8_t* expected,
                                         int expected_size) {
      const auto* actual = static_cast<const std::uint8_t*>(sqlite3_column_blob(check.get(), column));
      return actual != nullptr && sqlite3_column_bytes(check.get(), column) == expected_size &&
             std::equal(expected, expected + expected_size, actual);
    };
    const int state = sqlite3_column_int(check.get(), 0);
    if (!column_blob_matches(1, context.session.data(), 16) ||
        !column_blob_matches(2, context.invocation.data(), 16) ||
        sqlite3_column_int(check.get(), 3) != static_cast<int>(context.n) ||
        sqlite3_column_int(check.get(), 4) != static_cast<int>(context.k) ||
        !column_blob_matches(5, context.circuit_sha256.data(), 32) ||
        !column_blob_matches(6, context.gate_schedule_sha256.data(), 32) ||
        !column_blob_matches(7, context.context_sha256.data(), 32) ||
        !column_blob_matches(8, context.package_set_sha256.data(), 32))
      fail("v3 escrow transaction metadata conflict");
    if (state == 2) fail("v3 escrow transaction is durably aborted");

    const auto cipher_hash = sha256(request.ciphertext);
    sqlite3_stmt* existing_raw = nullptr;
    sqlite_check(sqlite3_prepare_v2(db.get(),
        "SELECT cipher_hash FROM shares WHERE transaction_id=?1 AND party=?2",
        -1, &existing_raw, nullptr), db.get(), "prepare v3 escrow replay check");
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> existing(existing_raw, sqlite3_finalize);
    sqlite_check(sqlite3_bind_text(existing.get(), 1, request.transaction.c_str(), -1, SQLITE_TRANSIENT),
                 db.get(), "bind v3 replay transaction");
    sqlite_check(sqlite3_bind_int(existing.get(), 2, request.party), db.get(), "bind v3 replay party");
    const auto existing_step = sqlite3_step(existing.get());
    if (existing_step == SQLITE_ROW) {
      const auto* old_hash = static_cast<const std::uint8_t*>(sqlite3_column_blob(existing.get(), 0));
      if (old_hash == nullptr || sqlite3_column_bytes(existing.get(), 0) != 32 ||
          !std::equal(cipher_hash.begin(), cipher_hash.end(), old_hash))
        fail("v3 escrow duplicate submit changed ciphertext");
    } else {
      sqlite_check(existing_step, db.get(), "read v3 replay check");
      sqlite3_stmt* share_raw = nullptr;
      sqlite_check(sqlite3_prepare_v2(db.get(),
          "INSERT INTO shares(transaction_id,party,ciphertext,cipher_hash) VALUES(?1,?2,?3,?4)",
          -1, &share_raw, nullptr), db.get(), "prepare v3 encrypted share insert");
      std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> share(share_raw, sqlite3_finalize);
      sqlite_check(sqlite3_bind_text(share.get(), 1, request.transaction.c_str(), -1, SQLITE_TRANSIENT), db.get(), "bind share transaction");
      sqlite_check(sqlite3_bind_int(share.get(), 2, request.party), db.get(), "bind share recipient");
      sqlite_check(sqlite3_bind_blob(share.get(), 3, request.ciphertext.data(),
                   static_cast<int>(request.ciphertext.size()), SQLITE_TRANSIENT), db.get(), "bind ciphertext");
      sqlite_check(sqlite3_bind_blob(share.get(), 4, cipher_hash.data(), 32, SQLITE_TRANSIENT), db.get(), "bind ciphertext hash");
      sqlite_check(sqlite3_step(share.get()), db.get(), "insert v3 encrypted share");
    }

    sqlite3_stmt* count_raw = nullptr;
    sqlite_check(sqlite3_prepare_v2(db.get(),
        "SELECT count(*) FROM shares WHERE transaction_id=?1", -1, &count_raw, nullptr),
        db.get(), "prepare v3 escrow share count");
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> count(count_raw, sqlite3_finalize);
    sqlite_check(sqlite3_bind_text(count.get(), 1, request.transaction.c_str(), -1, SQLITE_TRANSIENT), db.get(), "bind v3 escrow count transaction");
    sqlite_check(sqlite3_step(count.get()), db.get(), "read v3 escrow share count");
    if (sqlite3_column_int(count.get(), 0) == 2 && state == 0) {
      std::array<std::array<std::uint8_t, 32>, 2> hashes{};
      sqlite3_stmt* hashes_raw = nullptr;
      sqlite_check(sqlite3_prepare_v2(db.get(),
          "SELECT party,cipher_hash FROM shares WHERE transaction_id=?1 ORDER BY party",
          -1, &hashes_raw, nullptr), db.get(), "prepare v3 escrow hashes");
      std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> hashes_stmt(hashes_raw, sqlite3_finalize);
      sqlite_check(sqlite3_bind_text(hashes_stmt.get(), 1, request.transaction.c_str(), -1, SQLITE_TRANSIENT), db.get(), "bind v3 escrow hashes transaction");
      for (int i = 0; i < 2; ++i) {
        sqlite_check(sqlite3_step(hashes_stmt.get()), db.get(), "read v3 escrow ciphertext hash");
        const int party = sqlite3_column_int(hashes_stmt.get(), 0);
        const auto* hash = static_cast<const std::uint8_t*>(sqlite3_column_blob(hashes_stmt.get(), 1));
        if (party < 0 || party > 1 || hash == nullptr || sqlite3_column_bytes(hashes_stmt.get(), 1) != 32)
          fail("v3 escrow hash record invalid");
        std::copy_n(hash, 32, hashes[party].begin());
      }
      const auto receipt = make_receipt(context, request.transaction, hashes[0], hashes[1]);
      sqlite3_stmt* update_raw = nullptr;
      sqlite_check(sqlite3_prepare_v2(db.get(),
          "UPDATE transactions SET state=1,receipt=?2 WHERE transaction_id=?1 AND state=0",
          -1, &update_raw, nullptr), db.get(), "prepare v3 escrow commit");
      std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> update(update_raw, sqlite3_finalize);
      sqlite_check(sqlite3_bind_text(update.get(), 1, request.transaction.c_str(), -1, SQLITE_TRANSIENT), db.get(), "bind v3 commit transaction");
      sqlite_check(sqlite3_bind_blob(update.get(), 2, receipt.data(), 32, SQLITE_TRANSIENT), db.get(), "bind v3 commit receipt");
      sqlite_check(sqlite3_step(update.get()), db.get(), "write v3 escrow commit state");
    }
    sqlite_check(sqlite3_exec(db.get(), "COMMIT", nullptr, nullptr, nullptr), db.get(), "durably commit v3 escrow transaction");
  } catch (...) {
    sqlite3_exec(db.get(), "ROLLBACK", nullptr, nullptr, nullptr);
    throw;
  }
  return query_state(db_path, request.transaction);
}

std::vector<std::uint8_t> fetch_ciphertext(const std::string& db_path,
                                          const ScoreToMaskV3EscrowContext& context,
                                          const std::string& transaction,
                                          std::uint8_t party,
                                          std::array<std::uint8_t, 32>& receipt) {
  const auto state = query_state(db_path, transaction);
  if (state.state != 1) fail("v3 escrow record is not committed");
  receipt = state.receipt;
  auto db = open_database(db_path);
  sqlite3_stmt* raw = nullptr;
  sqlite_check(sqlite3_prepare_v2(db.get(),
      "SELECT s.ciphertext,t.context_hash FROM shares s "
      "JOIN transactions t USING(transaction_id) "
      "WHERE s.transaction_id=?1 AND s.party=?2",
      -1, &raw, nullptr), db.get(), "prepare recipient-only v3 escrow fetch");
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> stmt(raw, sqlite3_finalize);
  sqlite_check(sqlite3_bind_text(stmt.get(), 1, transaction.c_str(), -1, SQLITE_TRANSIENT), db.get(), "bind recipient transaction");
  sqlite_check(sqlite3_bind_int(stmt.get(), 2, party), db.get(), "bind recipient identity");
  sqlite_check(sqlite3_step(stmt.get()), db.get(), "fetch recipient ciphertext");
  const auto* bytes = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt.get(), 0));
  const auto size = sqlite3_column_bytes(stmt.get(), 0);
  const auto* context_hash = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt.get(), 1));
  if (context_hash == nullptr || sqlite3_column_bytes(stmt.get(), 1) != 32 ||
      !std::equal(context.context_sha256.begin(), context.context_sha256.end(), context_hash))
    fail("v3 escrow fetch context identity mismatch");
  if (bytes == nullptr || size <= 0 || size > static_cast<int>(kScoreToMaskV3MaxCiphertextBytes))
    fail("v3 escrow recipient ciphertext missing or invalid");
  return {bytes, bytes + size};
}

std::vector<std::uint8_t> make_response(std::uint8_t status,
                                       const std::array<std::uint8_t, 32>& receipt,
                                       const std::string& transaction,
                                       const std::vector<std::uint8_t>& ciphertext) {
  std::vector<std::uint8_t> out{status};
  append_u16(out, static_cast<std::uint16_t>(transaction.size()));
  append_u32(out, static_cast<std::uint32_t>(ciphertext.size()));
  out.insert(out.end(), receipt.begin(), receipt.end());
  out.insert(out.end(), transaction.begin(), transaction.end());
  out.insert(out.end(), ciphertext.begin(), ciphertext.end());
  return out;
}

int make_listener(const std::string& address, std::uint16_t port) {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  addrinfo* addresses = nullptr;
  const auto service = std::to_string(port);
  const int lookup = getaddrinfo(address.empty() ? nullptr : address.c_str(),
                                 service.c_str(), &hints, &addresses);
  if (lookup != 0) fail("v3 escrow bind address lookup failed");
  int listener = -1;
  for (auto* candidate = addresses; candidate != nullptr; candidate = candidate->ai_next) {
    listener = ::socket(candidate->ai_family, candidate->ai_socktype | SOCK_CLOEXEC,
                        candidate->ai_protocol);
    if (listener < 0) continue;
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (::bind(listener, candidate->ai_addr, candidate->ai_addrlen) == 0 &&
        ::listen(listener, 32) == 0) break;
    ::close(listener);
    listener = -1;
  }
  freeaddrinfo(addresses);
  if (listener < 0) fail("v3 escrow failed to bind/listen");
  return listener;
}

int connect_tcp(const std::string& host, std::uint16_t port) {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* addresses = nullptr;
  const auto service = std::to_string(port);
  if (getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses) != 0)
    fail("v3 escrow host lookup failed");
  int fd = -1;
  for (auto* candidate = addresses; candidate != nullptr; candidate = candidate->ai_next) {
    fd = ::socket(candidate->ai_family, candidate->ai_socktype | SOCK_CLOEXEC,
                  candidate->ai_protocol);
    if (fd < 0) continue;
    if (::connect(fd, candidate->ai_addr, candidate->ai_addrlen) == 0) break;
    ::close(fd);
    fd = -1;
  }
  freeaddrinfo(addresses);
  if (fd < 0) fail("v3 escrow connection failed");
  return fd;
}

void abort_pending_on_restart(const std::string& db_path) {
  auto db = open_database(db_path);
  sqlite_check(sqlite3_exec(db.get(), "BEGIN IMMEDIATE;UPDATE transactions SET state=2 WHERE state=0;COMMIT;",
                            nullptr, nullptr, nullptr), db.get(),
               "abort pre-commit records after escrow service restart");
}

void abort_transaction_if_pending(const std::string& db_path,
                                  const std::string& transaction) {
  auto db = open_database(db_path);
  sqlite3_stmt* raw = nullptr;
  sqlite_check(sqlite3_prepare_v2(db.get(),
      "UPDATE transactions SET state=2 WHERE transaction_id=?1 AND state=0",
      -1, &raw, nullptr), db.get(), "prepare durable v3 escrow abort");
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> stmt(raw, sqlite3_finalize);
  sqlite_check(sqlite3_bind_text(stmt.get(), 1, transaction.c_str(), -1, SQLITE_TRANSIENT),
               db.get(), "bind durable v3 escrow abort transaction");
  sqlite_check(sqlite3_exec(db.get(), "BEGIN IMMEDIATE", nullptr, nullptr, nullptr),
               db.get(), "begin durable v3 escrow abort");
  try {
    sqlite_check(sqlite3_step(stmt.get()), db.get(), "write durable v3 escrow abort");
    sqlite_check(sqlite3_exec(db.get(), "COMMIT", nullptr, nullptr, nullptr),
                 db.get(), "commit durable v3 escrow abort");
  } catch (...) {
    sqlite3_exec(db.get(), "ROLLBACK", nullptr, nullptr, nullptr);
    throw;
  }
}

void serve_connection(int fd, std::uint8_t party,
                      const ScoreToMaskV3EscrowServerConfig& config,
                      const ScoreToMaskV3EscrowContext& context,
                      SSL_CTX* tls_context) noexcept {
  bool channel_owns_fd = false;
  try {
    auto endpoint = protocol_i_tls13_wrap_fd(fd, tls_context, true,
                                             config.party_tls_identities[party]);
    ProtocolIFramedChannel channel(endpoint,
        escrow_frame_config(context, party, true),
        ProtocolIFramedChannelOptions{
            static_cast<int>(context.state.transaction_wait_timeout_ms), 4096});
    channel_owns_fd = true;
    channel.complete_authenticated_handshake();
    if (channel.peer_certificate_sha256() !=
        context.party_identity_sha256[party])
      fail("v3 escrow TLS party certificate pin mismatch");
    const std::vector<std::uint8_t> welcome{'A','R','E','A','D','Y',1};
    channel.send(welcome);
    channel.set_phase(kRequestPhase);
    const auto request = parse_request(channel.receive(), context, party);
    if (request.operation == kOpSubmit) {
      auto state = submit_ciphertext(config.sqlite_database_path,
                                     context, request);
      const auto deadline = std::chrono::steady_clock::now() +
          std::chrono::milliseconds(context.state.transaction_wait_timeout_ms);
      while (state.state == 0 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        state = query_state(config.sqlite_database_path, request.transaction);
      }
      if (state.state == 0) {
        abort_transaction_if_pending(config.sqlite_database_path, request.transaction);
        state = query_state(config.sqlite_database_path, request.transaction);
      }
      if (state.state == 1) {
        channel.send(make_response(kStatusCommitted, state.receipt,
                                   request.transaction, {}));
      } else {
        channel.send(make_response(kStatusAbort, {}, request.transaction, {}));
      }
    } else {
      std::array<std::uint8_t, 32> receipt{};
      const auto ciphertext = fetch_ciphertext(config.sqlite_database_path,
          context, request.transaction, party, receipt);
      channel.send(make_response(kStatusCommitted, receipt,
                                 request.transaction, ciphertext));
    }
  } catch (const std::exception& error) {
    std::fprintf(stderr, "v3 escrow request failed party=%u: %s\n",
                 static_cast<unsigned>(party), error.what());
    if (!channel_owns_fd) ::close(fd);
  } catch (...) {
    std::fprintf(stderr, "v3 escrow request failed party=%u: unknown error\n",
                 static_cast<unsigned>(party));
    if (!channel_owns_fd) ::close(fd);
  }
}

void serve_party_port(std::uint8_t party,
                      const ScoreToMaskV3EscrowServerConfig& config,
                      const ScoreToMaskV3EscrowContext& context,
                      SSL_CTX* tls_context) {
  const int listener = make_listener(config.bind_address, config.party_ports[party]);
  for (;;) {
    const int client = ::accept4(listener, nullptr, nullptr, SOCK_CLOEXEC);
    if (client < 0) continue;
    std::thread(serve_connection, client, party, std::cref(config),
                std::cref(context), tls_context).detach();
  }
}

std::vector<std::uint8_t> app_context(const ScoreToMaskV3PackageInfo& package,
                                      std::uint8_t party,
                                      std::uint8_t operation,
                                      const std::string& transaction,
                                      const std::vector<std::uint8_t>& cipher) {
  return make_request(package, party, operation, transaction, cipher);
}

std::vector<std::uint8_t> parse_response(const std::vector<std::uint8_t>& in,
                                         std::uint8_t expected_status,
                                         const std::string& transaction,
                                         std::array<std::uint8_t, 32>& receipt) {
  if (in.size() < 1 + 2 + 4 + 32) fail("v3 escrow response truncated");
  std::size_t at = 0;
  const auto status = in[at++];
  const auto transaction_size = read_u16(in, at);
  const auto ciphertext_size = read_u32(in, at);
  if (in.size() != 1U + 2U + 4U + 32U + transaction_size + ciphertext_size)
    fail("v3 escrow response length mismatch");
  std::copy_n(in.begin() + static_cast<std::ptrdiff_t>(at), receipt.size(), receipt.begin());
  at += receipt.size();
  const std::string returned(in.begin() + static_cast<std::ptrdiff_t>(at),
                             in.begin() + static_cast<std::ptrdiff_t>(at + transaction_size));
  at += transaction_size;
  if (returned != transaction) fail("v3 escrow response transaction mismatch");
  if (status != expected_status) fail("v3 escrow transaction not committed");
  return {in.begin() + static_cast<std::ptrdiff_t>(at), in.end()};
}

std::vector<std::uint8_t> request_service(
    const ScoreToMaskV3EscrowClientConfig& config,
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party,
    std::uint8_t operation, const std::string& transaction,
    const std::vector<std::uint8_t>& ciphertext,
    std::array<std::uint8_t, 32>& receipt) {
  if (party > 1) fail("v3 escrow client party invalid");
  const int fd = connect_tcp(config.host, config.port);
  SSL_CTX* tls_context = nullptr;
  bool channel_owns_fd = false;
  std::vector<std::uint8_t> result;
  try {
    tls_context = protocol_i_tls13_context_create(config.tls_credentials);
    auto endpoint = protocol_i_tls13_wrap_fd(fd, tls_context, false,
                                             config.expected_server_identity);
    auto party_package = package;
    party_package.party = party;
    ProtocolIFramedChannel channel(endpoint,
        escrow_frame_config(party_package, party, false),
        ProtocolIFramedChannelOptions{60000, 4096});
    channel_owns_fd = true;
    channel.complete_authenticated_handshake();
    if (channel.peer_certificate_sha256() !=
        package.parameters.escrow_identity_sha256)
      fail("v3 escrow TLS certificate pin mismatch");
    const auto welcome = channel.receive();
    if (welcome != std::vector<std::uint8_t>({'A','R','E','A','D','Y',1}))
      fail("v3 escrow service READY mismatch");
    channel.set_phase(kRequestPhase);
    channel.send(app_context(package, party, operation, transaction, ciphertext));
    const auto response = channel.receive();
    result = parse_response(response, kStatusCommitted, transaction, receipt);
  } catch (...) {
    if (tls_context != nullptr) protocol_i_tls13_context_free(tls_context);
    if (!channel_owns_fd) ::close(fd);
    throw;
  }
  protocol_i_tls13_context_free(tls_context);
  return result;
}

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> read_rsa_key(
    const std::string& path, bool private_key) {
  FILE* file = std::fopen(path.c_str(), "rb");
  if (file == nullptr) fail("cannot open v3 recipient RSA key");
  EVP_PKEY* raw = private_key ? PEM_read_PrivateKey(file, nullptr, nullptr, nullptr)
                              : PEM_read_PUBKEY(file, nullptr, nullptr, nullptr);
  std::fclose(file);
  if (raw == nullptr || EVP_PKEY_base_id(raw) != EVP_PKEY_RSA || EVP_PKEY_bits(raw) < 2048) {
    if (raw != nullptr) EVP_PKEY_free(raw);
    fail("v3 recipient RSA key must be >=2048 bits");
  }
  return {raw, EVP_PKEY_free};
}

std::vector<std::uint8_t> decrypt_ciphertext(
    const std::vector<std::uint8_t>& envelope,
    const std::string& private_key_path,
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party) {
  constexpr std::size_t fixed = 8 + 2 + 4 + 12 + 16;
  if (party > 1 || envelope.size() < fixed ||
      !std::equal(kCipherMagic.begin(), kCipherMagic.end(), envelope.begin()))
    fail("v3 recipient ciphertext header invalid");
  std::size_t at = 8;
  const auto encrypted_key_size = read_u16(envelope, at);
  const auto plaintext_size = read_u32(envelope, at);
  if (plaintext_size != package.parameters.n || encrypted_key_size == 0 ||
      envelope.size() != fixed + encrypted_key_size + plaintext_size)
    fail("v3 recipient ciphertext length mismatch");
  std::array<std::uint8_t, 12> nonce{};
  std::copy_n(envelope.begin() + static_cast<std::ptrdiff_t>(at), nonce.size(), nonce.begin());
  at += nonce.size();
  std::array<std::uint8_t, 16> tag{};
  std::copy_n(envelope.begin() + static_cast<std::ptrdiff_t>(at), tag.size(), tag.begin());
  at += tag.size();
  auto key = read_rsa_key(private_key_path, true);
  const int public_der_size = i2d_PUBKEY(key.get(), nullptr);
  if (public_der_size <= 0 || public_der_size > 4096)
    fail("v3 recipient private key public identity invalid");
  std::vector<std::uint8_t> public_der(static_cast<std::size_t>(public_der_size));
  auto* public_der_cursor = public_der.data();
  if (i2d_PUBKEY(key.get(), &public_der_cursor) != public_der_size ||
      sha256(public_der) != package.parameters.recipient_rsa_public_sha256[party])
    fail("v3 recipient private key does not match signed package identity");
  std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> rsa_ctx(
      EVP_PKEY_CTX_new(key.get(), nullptr), EVP_PKEY_CTX_free);
  if (!rsa_ctx || EVP_PKEY_decrypt_init(rsa_ctx.get()) <= 0 ||
      EVP_PKEY_CTX_set_rsa_padding(rsa_ctx.get(), RSA_PKCS1_OAEP_PADDING) <= 0 ||
      EVP_PKEY_CTX_set_rsa_oaep_md(rsa_ctx.get(), EVP_sha256()) <= 0 ||
      EVP_PKEY_CTX_set_rsa_mgf1_md(rsa_ctx.get(), EVP_sha256()) <= 0)
    fail("v3 recipient RSA-OAEP initialization failed");
  std::vector<std::uint8_t> aes_key_buffer(
      static_cast<std::size_t>(EVP_PKEY_get_size(key.get())));
  std::size_t aes_key_size = aes_key_buffer.size();
  if (EVP_PKEY_decrypt(rsa_ctx.get(), aes_key_buffer.data(), &aes_key_size,
      envelope.data() + at, encrypted_key_size) <= 0 || aes_key_size != 32)
    fail("v3 recipient RSA-OAEP decryption failed");
  std::array<std::uint8_t, 32> aes_key{};
  std::copy_n(aes_key_buffer.begin(), aes_key.size(), aes_key.begin());
  at += encrypted_key_size;
  std::vector<std::uint8_t> aad;
  aad.insert(aad.end(), package.parameters.session.begin(), package.parameters.session.end());
  aad.insert(aad.end(), package.parameters.invocation.begin(), package.parameters.invocation.end());
  aad.push_back(party);
  aad.insert(aad.end(), package.circuit_sha256.begin(), package.circuit_sha256.end());
  aad.insert(aad.end(), package.gate_schedule_sha256.begin(), package.gate_schedule_sha256.end());
  std::vector<std::uint8_t> plaintext(plaintext_size);
  std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> aes_ctx(
      EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
  int written = 0;
  int total = 0;
  if (!aes_ctx || EVP_DecryptInit_ex(aes_ctx.get(), EVP_aes_256_gcm(), nullptr,
                                     nullptr, nullptr) != 1 ||
      EVP_CIPHER_CTX_ctrl(aes_ctx.get(), EVP_CTRL_GCM_SET_IVLEN,
                          static_cast<int>(nonce.size()), nullptr) != 1 ||
      EVP_DecryptInit_ex(aes_ctx.get(), nullptr, nullptr, aes_key.data(), nonce.data()) != 1 ||
      EVP_DecryptUpdate(aes_ctx.get(), nullptr, &written, aad.data(),
                        static_cast<int>(aad.size())) != 1 ||
      EVP_DecryptUpdate(aes_ctx.get(), plaintext.data(), &written,
                        envelope.data() + at, static_cast<int>(plaintext_size)) != 1)
    fail("v3 recipient AES-GCM decryption failed");
  total = written;
  if (EVP_CIPHER_CTX_ctrl(aes_ctx.get(), EVP_CTRL_GCM_SET_TAG,
                          static_cast<int>(tag.size()), tag.data()) != 1 ||
      EVP_DecryptFinal_ex(aes_ctx.get(), plaintext.data() + total, &written) != 1)
    fail("v3 recipient ciphertext authentication failed");
  total += written;
  plaintext.resize(static_cast<std::size_t>(total));
  if (plaintext.size() != package.parameters.n ||
      std::any_of(plaintext.begin(), plaintext.end(), [](std::uint8_t bit) { return bit > 1; }))
    fail("v3 decrypted recipient share is malformed");
  return plaintext;
}

}  // namespace

std::array<std::uint8_t, 32> score_to_mask_v3_certificate_sha256(
    const std::string& certificate_pem) {
  FILE* file = std::fopen(certificate_pem.c_str(), "rb");
  if (file == nullptr) fail("cannot open v3 escrow certificate");
  X509* raw = PEM_read_X509(file, nullptr, nullptr, nullptr);
  std::fclose(file);
  if (raw == nullptr) fail("cannot parse v3 escrow certificate");
  std::unique_ptr<X509, decltype(&X509_free)> certificate(raw, X509_free);
  const int encoded_size = i2d_X509(certificate.get(), nullptr);
  if (encoded_size <= 0 || encoded_size > (1 << 20)) fail("v3 certificate DER size invalid");
  std::vector<std::uint8_t> encoded(static_cast<std::size_t>(encoded_size));
  auto* cursor = encoded.data();
  if (i2d_X509(certificate.get(), &cursor) != encoded_size)
    fail("v3 certificate DER encode failed");
  return sha256(encoded);
}

void score_to_mask_v3_run_escrow_service(
    const ScoreToMaskV3EscrowServerConfig& config) {
  if (config.party_ports[0] == 0 || config.party_ports[1] == 0 ||
      config.party_ports[0] == config.party_ports[1] ||
      config.sqlite_database_path.empty() ||
      config.signed_context_path.empty() ||
      config.dealer_ed25519_public_key_pem.empty())
    fail("v3 escrow service configuration invalid");
  const auto context = score_to_mask_v3_validate_escrow_context(
      config.signed_context_path, config.dealer_ed25519_public_key_pem);
  if (context.n == 0 ||
      (context.format_version == kScoreToMaskV3EscrowContextFormatVersion &&
       context.n > kScoreToMaskV3MaxN))
    fail("v3 escrow context N is outside the admitted range");
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (context.format_version == kScoreToMaskV3EscrowContextFormatVersionFtV3 &&
      context.n > kScoreToMaskV3FtV3MaxN)
    fail("F_T/v3 escrow context N is outside the admitted range");
#endif
  if (score_to_mask_v3_certificate_sha256(config.tls_credentials.certificate_file) !=
      context.escrow_identity_sha256)
    fail("v3 escrow certificate does not match the signed context identity");
  auto db = open_database(config.sqlite_database_path);
  sqlite_check(sqlite3_exec(db.get(),
      "SELECT context_hash,package_set_hash FROM transactions LIMIT 0",
      nullptr, nullptr, nullptr), db.get(),
      "validate v3 escrow database context schema");
  db.reset();
  abort_pending_on_restart(config.sqlite_database_path);
  SSL_CTX* tls_context = protocol_i_tls13_context_create(config.tls_credentials);
  std::thread p0(serve_party_port, 0, std::cref(config), std::cref(context), tls_context);
  std::thread p1(serve_party_port, 1, std::cref(config), std::cref(context), tls_context);
  p0.join();
  p1.join();
  protocol_i_tls13_context_free(tls_context);
}

std::string score_to_mask_v3_escrow_submit(
    const ScoreToMaskV3EscrowClientConfig& config,
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party,
    const std::string& transaction,
    const std::vector<std::uint8_t>& recipient_ciphertext) {
  if (transaction != transaction_id(package)) fail("v3 escrow transaction id is not session bound");
  std::array<std::uint8_t, 32> receipt{};
  (void)request_service(config, package, party, kOpSubmit, transaction,
                        recipient_ciphertext, receipt);
  return hex(receipt.data(), receipt.size());
}

ScoreToMaskV3EscrowFetchedRecord score_to_mask_v3_escrow_fetch(
    const ScoreToMaskV3EscrowClientConfig& config,
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party,
    const std::string& transaction) {
  if (transaction != transaction_id(package)) fail("v3 escrow fetch transaction id mismatch");
  std::array<std::uint8_t, 32> receipt{};
  ScoreToMaskV3EscrowFetchedRecord result;
  result.recipient_ciphertext = request_service(
      config, package, party, kOpFetch, transaction, {}, receipt);
  result.commit_receipt = hex(receipt.data(), receipt.size());
  return result;
}

std::vector<std::uint8_t> score_to_mask_v3_decrypt_recipient_share(
    const std::vector<std::uint8_t>& recipient_ciphertext,
    const std::string& recipient_rsa_private_key_pem,
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party) {
  return decrypt_ciphertext(recipient_ciphertext, recipient_rsa_private_key_pem,
                            package, party);
}

}  // namespace moe_topk
