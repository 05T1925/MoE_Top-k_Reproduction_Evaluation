# M2 Candidate-B entropy derivation contract

Date: 2026-09-21

## Status and scope

**Decision-complete C-INSTANTIATION; DESIGN ONLY.** This is the normative byte-level `M2CBKDF1` contract for the independent Dealer-DPF BoundPublicMaskShuffle Candidate B: **Agarwal Protocol I functionality-aligned 3-round experimental candidate (NON-EXACT), using an independent Dealer-DPF shuffle construction.** It is not target-paper behavior, Agarwal author behavior, Chase behavior, or paper-exact Protocol I.

Backend design: **VALID**. Security-contract remediation: **COMPLETE**. Entropy derivation contract: **COMPLETE**. Implementation: **NOT PRESENT**. Implementation authorization: **NO**. Backend implementation tests and benchmarks: **NOT RUN**. Empirical communication: **NOT_MEASURED**. Strict author-exact G1, official G2, official G3, and M5 runtime dependency remain **BLOCKED**.

## Identifier and frame contract

`session_id` is exactly 8 bytes and exactly one nonzero `uint64_t` logical value. `material_set_id` and `party_package_id` are exactly 16 bytes / 128 bits. `ledger_attempt_id` is exactly 16 bytes, a new Candidate-B C-INSTANTIATION that changes no existing repository field. The rejected draft with `session_id = 16 bytes` and `material_set_id = 32 bytes` was never authoritative and has no compatibility status.

The existing `ProtocolIFrameConfig` session type is `std::uint64_t`: logical session width is 64 bits and wire encoding is 8-byte big-endian. Existing frame schema and code changes are **NO**. No widening; no truncation.

## RFC5869 Extract and Expand

Hash is SHA-256 and `HashLen=32`. IKM is exactly 32 bytes from one complete production `getrandom(2, flags=0)` acquisition; retry only `EINTR` and fail closed otherwise. RFC5869 empty salt is concretely `salt_key = 0000000000000000000000000000000000000000000000000000000000000000` (32 zero bytes):

```text
PRK = HMAC-SHA-256(key = salt_key, data = IKM)
```

`PRK` length is 32 bytes. Do not rely on omitted-salt defaults. Extract does not include session ID, material-set ID, configuration, counter, PID, timestamp, or other metadata.

```text
T(0) = empty
T(i) = HMAC-SHA-256(PRK, T(i-1) || info || OCTET(i))
OKM = first L bytes of T(1) || ...
```

Only `L in {8,16,32}` is permitted, so current Expand calls use only `T(1)`.

## Preamble, TLVs, and fingerprint

The exact preamble is ASCII `M2CBKDF1` (8 bytes, case-sensitive, no NUL). Each following field is `uint8 type || uint16 BE length || exactly length value`. Types strictly ascend; no duplicates, unknown types, padding, terminator, trailing bytes, or incomplete input is allowed.

| Type | Field | Exact value |
| --- | --- | --- |
| `01` | protocol label | ASCII `moe-topk/dealer-dpf-bound-public-mask-shuffle`, 45 bytes, case-sensitive, no NUL |
| `02` | config fingerprint | exactly 32 bytes |
| `03` | session ID | 8 bytes `U64BE(session_id)` |
| `04` | material-set ID | 16 bytes |
| `05` | domain tag | `uint16` BE |
| `06` | party | `uint8`: P0=0, P1=1 |
| `07` | field | `uint16` BE: priority key=1, original-index payload=2 |
| `08` | DPF output slot | `uint32` BE |
| `09` | DCF left slot | `uint32` BE |
| `0a` | DCF right slot | `uint32` BE |
| `0b` | output length | `uint16` BE: 8, 16, or 32 |

Session info contains `01,02,05,0b`, `L=8`; material-set info contains `01,02,03,05,0b`, `L=16`; later info contains `01,02,03,04,05`, required selectors, then `0b`. Package and ledger IDs include party; DPF includes one output slot; DCF includes canonical `left < right < N`. No derivation counter exists.

```text
SHA-256(ASCII("M2CB-PUBLIC-CONFIG-V1") || U32BE(logical_n) || U32BE(N) || U32BE(K) || U16BE(b) || U16BE(ell_prime) || U16BE(p) || U16BE(record_schema_id=1) || U16BE(dpf_bin=b) || U16BE(dpf_bout=64) || U32BE(dpf_count=N) || U64BE(dcf_edge_count=N*(N-1)/2))
```

Validate configuration before hashing; callers cannot provide a replacement fingerprint. Derive `PRK -> session_id -> material_set_id -> all subsequent objects`; material-set info binds session ID and later info binds session plus material-set ID.

## Domain tags and lengths

| tag | domain | selector | L |
| --- | --- | --- | --- |
| `0001` | session-id | none | 8 |
| `0002` | material-set-id | session ID | 16 |
| `0003` | package-id | party | 16 |
| `0004` | ledger-attempt-id | party | 16 |
| `0100` | permutation/src | none | 32 |
| `0200` | u/full | field | 32 |
| `0201` | u/share0 | field | 32 |
| `0202` | v/share0 | field | 32 |
| `0300` | rA/full | none | 32 |
| `0301` | rA/share0 | none | 32 |
| `1000` | DPF | output slot | 16 |
| `1001` | DCF | canonical left/right | 16 |

Human-readable labels do not enter info; numeric tags are dispositive. DPF/DCF `L=16` are FSS SetSeed material; every `L=32` is an AES-256 counter-stream key.

## Streams, rings, Fisher-Yates

`R[j]=AES-256-ECB(key=OKM32, plaintext=U128BE(j))`; stream is `R[0] || R[1] || ...`, `j` starts at zero, padding is disabled, and implementation fails before 128-bit wrap or on cipher error. This is not `FSSConfig::prngs`; future code uses vetted OpenSSL 3 libcrypto, not handwritten AES.

For width `w`, consume `ceil(w/8)` bytes, decode unsigned big-endian, and mask excess high bits modulo `2^w`, yielding uniform ring elements. Compute `u1=u-u0`, `v=pi(u)`, `v1=v-v0`, `rA1=rA-rA0`; no extra KDF domains exist for computed values.

For `i=N-1` down to 1, let `m=i+1`, consume `x=U64BE(next 8 bytes)`, set `threshold=2^64 mod m` (unsigned `(-m)%m`), reject while `x<threshold`, set `j=x mod m`, and swap `i,j`. Modulo-biased sampling is forbidden.

## Session and FSS mapping

`session_okm=HKDF-Expand(PRK,session_info,L=8)` and `session_id=U64BE(session_okm[0..7])`; subsequent info encodes identical `U64BE(session_id)`. Do not derive 16 then truncate, use host-endian loads, or `reinterpret_cast`. If production session is zero, emit no package/ID/ledger, destroy root/PRK/context, acquire fresh 32-byte OS root, and restart Extract; never map zero to one. A TEST_ONLY zero session fails without automatic reroot.

For `s[0]..s[15]`: `low64=LE64(s[0]...s[7])`, `high64=LE64(s[8]...s[15])`, `block=osuCrypto::toBlock(high64,low64)`, then `FSSConfig::prngs[0].SetSeed(block)`. LE64 uses explicit byte shift/OR. Prohibit `reinterpret_cast`, pointer punning, unaligned/native loads, `memcpy` into host integer semantics, and `toBlock(u8*)`. Logical `__m128i` lanes 0..15 equal `s[0]..s[15]`. Lock spans block construction, SetSeed, and complete DPF/DCF keygen in dedicated P2, outside OpenMP, thread 0.

## Production and TEST_ONLY boundary

The sole conceptual production entry is `generate_candidate_b_material(validated_public_config)`: compute fingerprint; obtain fresh root; derive IDs/streams/FSS seeds; use serialized FSS lane; return opaque production packages and public IDs. It accepts no deterministic root, entropy provider, PRK, OKM, raw seed, `osuCrypto::block`, caller IDs/fingerprint, or TEST_ONLY type. Secrets are ephemeral and never serialized/logged.

Test-only `generate_candidate_b_material_test_only(validated_public_config,TestOnlyRootEntropy32)` has a non-convertible root unavailable to production; its symbol is absent from production; packages remain TEST_ONLY and production loaders reject them; entropy injection and PRK/info/OKM/block diagnostics are TEST_ONLY; byte helper remains private; it never legitimizes `FSSBase::initPrngs()` in production.

Preserve P2 fork+exec, root after exec, no root/PRK/stream/FSS cache across generations, fresh root per attempt, discard/fail child of live context, no restart state, and domain/slot/canonical-edge isolation; both bootstrap IDs bind later FSS info.

## Approved KATs

```text
root = 000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f
config: logical_n=3 N=4 K=2 b=2 ell_prime=35 p=2 record_schema_id=1 dpf_bin=2 dpf_bout=64 dpf_count=4 dcf_edge_count=6
config encoding = 4d3243422d5055424c49432d434f4e4649472d5631000000030000000400000002000200230002000100020040000000040000000000000006
fingerprint = 0fddf34dc2d1e038d2b049d8bff167db4d5cd21e1625600115c860c761e1e035
PRK = 46bd320605c5a6b6163ab70bc6345b92a5f908e79fe58979c23ebb47d1a5e307
session OKM = 4666b8ce4395f207
session_id = 0x4666b8ce4395f207
material-set ID = dbe65309c94a51411e9b69db8c2a9374
```

RFC5869 Test Case 1 exactly: IKM=`0b` repeated 22; salt=`000102030405060708090a0b0c`; info=`f0f1f2f3f4f5f6f7f8f9`; L=42; PRK=`077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5`; OKM=`3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865`.

| vector | info length | info SHA-256 | T(1) | OKM |
| --- | ---: | --- | --- | --- |
| session | 101 | `113d466e032a92beddb13621b9edfae082bc7f437aebd6cd81818e04b0ad2114` | `4666b8ce4395f207003bf01b2dc3564425bb63b66ab8457de6517e6ad596173d` | `4666b8ce4395f207` |
| material-set | 112 | `5cb9fbf75c06b70cfa515000f8d470447543f6a3733965778d02196e8531ea94` | `dbe65309c94a51411e9b69db8c2a93743ae065d8fe6a532109f0c9b182003d9c` | `dbe65309c94a51411e9b69db8c2a9374` |
| DPF 1 | 138 | `a976e8409579516215afabcb596b2bb59469e38cd27148efdf5b113a51cbc810` | `483e89e9f045a0741b8fb626b27dd7227e3f0a6b315e95ccd55484fb26d57820` | `483e89e9f045a0741b8fb626b27dd722` |
| DPF 2 | 138 | `7d897eb2592a858c1732e212b2addf8137f88e7cfb0fac028f8f394f95abea9c` | `5a2d52aac9e1afa2644d9569feca29be8e3a94afdad86836e2a93ed2ecc84dbb` | `5a2d52aac9e1afa2644d9569feca29be` |
| DCF (1,3) | 145 | `3272cb19cb75cf2d117e539453fc029c20b7eae39be42913cb83d262361cdf99` | `25a96874982d72bfe86ab1282ff7fd96de9da387c2728c5d420427a4aa7bdd40` | `25a96874982d72bfe86ab1282ff7fd96` |

```text
session info = 4d3243424b44463101002d6d6f652d746f706b2f6465616c65722d6470662d626f756e642d7075626c69632d6d61736b2d73687566666c650200200fddf34dc2d1e038d2b049d8bff167db4d5cd21e1625600115c860c761e1e03505000200010b00020008
material-set info = 4d3243424b44463101002d6d6f652d746f706b2f6465616c65722d6470662d626f756e642d7075626c69632d6d61736b2d73687566666c650200200fddf34dc2d1e038d2b049d8bff167db4d5cd21e1625600115c860c761e1e0350300084666b8ce4395f20705000200020b00020010
DPF1 info = 4d3243424b44463101002d6d6f652d746f706b2f6465616c65722d6470662d626f756e642d7075626c69632d6d61736b2d73687566666c650200200fddf34dc2d1e038d2b049d8bff167db4d5cd21e1625600115c860c761e1e0350300084666b8ce4395f207040010dbe65309c94a51411e9b69db8c2a93740500021000080004000000010b00020010
DPF2 info = 4d3243424b44463101002d6d6f652d746f706b2f6465616c65722d6470662d626f756e642d7075626c69632d6d61736b2d73687566666c650200200fddf34dc2d1e038d2b049d8bff167db4d5cd21e1625600115c860c761e1e0350300084666b8ce4395f207040010dbe65309c94a51411e9b69db8c2a93740500021000080004000000020b00020010
DCF13 info = 4d3243424b44463101002d6d6f652d746f706b2f6465616c65722d6470662d626f756e642d7075626c69632d6d61736b2d73687566666c650200200fddf34dc2d1e038d2b049d8bff167db4d5cd21e1625600115c860c761e1e0350300084666b8ce4395f207040010dbe65309c94a51411e9b69db8c2a93740500021001090004000000010a0004000000030b00020010
```

DPF1 maps `low64=0x74a045f0e9893e48`, `high64=0x22d77db226b68f1b`, `toBlock(0x22d77db226b68f1b,0x74a045f0e9893e48)`; DPF2 maps `low64=0xa2afe1c9aa522d5a`, `high64=0xbe29cafe69954d64`, `toBlock(0xbe29cafe69954d64,0xa2afe1c9aa522d5a)`; DCF (1,3) maps `low64=0xbf722d987468a925`, `high64=0x96fdf72f28b16ae8`, `toBlock(0x96fdf72f28b16ae8,0xbf722d987468a925)`.

`PYTHON_REFERENCE_MATCH=YES`; `OPENSSL_REFERENCE_MATCH=YES`. Python used standard-library hmac/hashlib; OpenSSL 3 EVP_KDF independently reproduced corrected DPF slot-1 OKM16. These are not mentally calculated vectors.

## Versioning and dependency

`M2CBKDF1` is the only valid current derivation version. The previously rejected draft is NOT M2CBKDF1, has NO compatibility status, and MUST NOT be accepted or parsed. `M2CBKDF1` versions empty-salt Extract, config/TLV encoding, widths/endianness, tags, output lengths, stream, ring decoding, Fisher-Yates, and FSS block mapping. A change requires `M2CBKDF2` or later, new KATs, and explicit migration decision; unknown/mismatched versions fail closed. Required future dependency is OpenSSL 3 libcrypto: EVP_KDF HKDF/SHA256, SHA-256, and AES-256-ECB without padding. Handwritten SHA-256, HMAC, HKDF, or AES is not authorized. A Candidate-B target not inheriting EMP's OpenSSL link must require OpenSSL 3 and link `OpenSSL::Crypto`. This is documentation only.
