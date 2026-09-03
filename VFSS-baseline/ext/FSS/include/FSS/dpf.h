#pragma once
#include <cryptoTools/Common/Defines.h>
#include <FSS/group_element.h>
#include <FSS/keypack.h>
#include <vector>

// 传统DPF接口
std::pair<DPFKeyPack, DPFKeyPack> keyGenDPF(int bin, int bout, GroupElement idx, GroupElement payload);
GroupElement evalDPF_EQ(int party, DPFKeyPack &key, GroupElement x);
GroupElement evalDPF_EQ2(int party, DPFKeyPack &key, GroupElement x);
GroupElement evalDPF_EQ2_slow(int party, DPFKeyPack &key, GroupElement x);
GroupElement evalDPF_GT(int party, DPFKeyPack &key, GroupElement x);
GroupElement evalDPF_LT(int party, DPFKeyPack &key, GroupElement x);
void evalAll(int party, DPFKeyPack &key, GroupElement rightShift, GroupElement *out);
GroupElement evalAll_reduce(int party, DPFKeyPack &key, GroupElement rightShift, const std::vector<GroupElement> &tab);

// Grotto-DPF接口
std::pair<DPFETKeyPack, DPFETKeyPack> keyGenDPFET(int bin, GroupElement idx);
std::pair<GroupElement, GroupElement> evalAll_reduce_et(int party, DPFETKeyPack &key, GroupElement rightShift, const std::vector<GroupElement> &tab);
GroupElement evalDPFET_LT(int party, const DPFETKeyPack &key, GroupElement x);

// GTDPF接口
std::pair<DPFETKeyPack, DPFETKeyPack> keyGenGTDPF(int bin, GroupElement idx);
GroupElement evalGTDPF(int party, const DPFETKeyPack &key, GroupElement x);

// GTDCF
std::pair<GTDCFKeyPack, GTDCFKeyPack> keyGenGTDCF(
    int bin, int w, int groupSize, GroupElement idx, const GroupElement* beta);

void evalGTDCF(
    int party, const GTDCFKeyPack &key, GroupElement x, GroupElement* res);

// 
std::pair<QuadDPFKeyPack, QuadDPFKeyPack> keyGenQuadDPF(int bin, int bout, GroupElement idx, GroupElement payload);
GroupElement evalQuadDPF(int party, const QuadDPFKeyPack &key, GroupElement x);

std::pair<OctDPFKeyPack, OctDPFKeyPack> keyGenOctDPF(int bin, int bout, GroupElement idx, GroupElement payload);
GroupElement evalOctDPF(int party, const OctDPFKeyPack &key, GroupElement x);

// VDPF
std::pair<VerDPFKeyPack, VerDPFKeyPack>keyGenVerDPF(int bin, int bout, GroupElement idx, GroupElement payload);

void evalVerDPF_Batch(int party,
                      const VerDPFKeyPack &key,
                      const std::vector<GroupElement> &x_vec,
                      std::vector<GroupElement> &y_vec,
                      osuCrypto::block pi_out[2]);

GroupElement evalVerDPF(int party,
                        const VerDPFKeyPack &key,
                        GroupElement x);

bool verifyVerDPF(const osuCrypto::block pi0[2],
                  const osuCrypto::block pi1[2]);


// ============================================================
// DPF payload evaluation
// ============================================================

GroupElement evalDPF_Payload(int party,
                             DPFKeyPack &key,
                             GroupElement x);

// ============================================================
// IFSS common arithmetic MAC helpers
// ============================================================

IFSSGlobalMACKey ifss_setup_arithmetic_mac(int bout);

GroupElement ifss_reconstruct_value(IFSSAuthShare s0,
                                    IFSSAuthShare s1,
                                    int bout);

bool ifss_check_mac_single(IFSSAuthShare s0,
                           IFSSAuthShare s1,
                           GroupElement deltaA0,
                           GroupElement deltaA1,
                           int bout);

bool ifss_batch_check_arithmetic(const std::vector<IFSSAuthShare> &shares0,
                                 const std::vector<IFSSAuthShare> &shares1,
                                 GroupElement deltaA0,
                                 GroupElement deltaA1,
                                 int bout,
                                 uint64_t seed);

// ============================================================
// IFSS_DPF
// ============================================================

std::pair<IFSS_DPFKeyPack, IFSS_DPFKeyPack>
keyGenIFSS_DPF(int bin,
               int bout,
               GroupElement alpha,
               GroupElement beta,
               GroupElement deltaA);

IFSSAuthShare evalIFSS_DPF(int party,
                           IFSS_DPFKeyPack &key,
                           GroupElement x);


// ============================================================================
// Half-Tree DPF declarations
// Put this block in FSS/dpf.h
// ============================================================================

#include <vector>

struct HalfTreeDPFEvalTrace
{
    // parent[i] is the node share on prefix x_1...x_i before processing level i+1.
    // parent[0] is the root share. Size = bin.
    std::vector<osuCrypto::block> parent;

    // Final leaf share.
    osuCrypto::block leaf;
};

std::pair<HalfTreeDPFKeyPack, HalfTreeDPFKeyPack>
keyGenHalfTreeDPF(int bin, int bout, GroupElement alpha, GroupElement beta);

// Internal helper used by Half-Tree DCF key generation.
// path0/path1 store on-path parent shares for each DCF level.
std::pair<HalfTreeDPFKeyPack, HalfTreeDPFKeyPack>
keyGenHalfTreeDPFWithPath(int bin,
                          int bout,
                          GroupElement alpha,
                          GroupElement beta,
                          std::vector<osuCrypto::block> *path0,
                          std::vector<osuCrypto::block> *path1);

GroupElement evalHalfTreeDPF(int party,
                             const HalfTreeDPFKeyPack &key,
                             GroupElement x,
                             HalfTreeDPFEvalTrace *trace = nullptr);

void evalAllHalfTreeDPF(int party,
                        const HalfTreeDPFKeyPack &key,
                        GroupElement *out);

GroupElement reconstructHalfTreeDPF(const HalfTreeDPFKeyPack &k0,
                                    const HalfTreeDPFKeyPack &k1,
                                    GroupElement x);
                            
// ============================================================================
// CAVERN VIDPF.
// ============================================================================

VIDPFKeyGenResult keyGenVIDPF(
    int bin,
    int ringBw,
    GroupElement alpha,
    const std::vector<VIDPFPayload> &betaVec);

VIDPFKeyGenResult keyGenVIDPF(
    int bin,
    int ringBw,
    GroupElement alpha,
    const VIDPFPayload &beta);

VIDPFEvalResult evalVIDPF(
    int party,
    const VIDPFKeyPack &key,
    const std::vector<VIDPFQuery> &queries);

VIDPFPayload evalVIDPFPrefix(
    int party,
    const VIDPFKeyPack &key,
    GroupElement prefix,
    int prefixLength,
    uint8_t *controlBit = nullptr);

bool verifyVIDPF(
    const VIDPFToken &mu0,
    const VIDPFToken &mu1);