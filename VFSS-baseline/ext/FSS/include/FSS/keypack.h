#pragma once

#include <cryptoTools/Common/Defines.h>
#include <FSS/group_element.h>
#include <FSS/cavern_group_element.h>
#include <vector>

struct DCFKeyPack{
    int Bin, Bout, groupSize;
    osuCrypto::block *k;   // size Bin+1
    GroupElement *g;    // bitsize Bout, size groupSize
    GroupElement *v;   // bitsize Bout, size Bin x groupSize
    DCFKeyPack(int Bin, int Bout, int groupSize,
                osuCrypto::block *k,
                GroupElement *g,
                GroupElement *v) : Bin(Bin), Bout(Bout), groupSize(groupSize), k(k), g(g), v(v){}
    DCFKeyPack() {
        Bin = Bout = groupSize = 0;
        k = nullptr;
        g = nullptr;
        v = nullptr;
    }
};

struct DualDCFKeyPack{  
    int Bin, Bout, groupSize;
    DCFKeyPack dcfKey;
    GroupElement *sb;   // size: groupSize
    DualDCFKeyPack() {}
};

struct AddKey{
    int Bin, Bout;
    GroupElement rb;
};

struct MultKey{
    int Bin, Bout;
    GroupElement a, b, c;
};

struct MatMulKey{
    int Bin, Bout;
    int s1, s2, s3;
    GroupElement *a, *b, *c;    
};

struct MultKeyNew {
    GroupElement a, b, c;
    DCFKeyPack k1, k2, k3, k4;
};

struct Conv2DKey{
    int Bin, Bout;
    int N, H, W, CI, FH, FW, CO,
        zPadHLeft, zPadHRight, 
        zPadWLeft, zPadWRight,
        strideH, strideW;
    GroupElement *a, *b, *c;    
};

struct Conv3DKey{
    int Bin, Bout;
    int N, D, H, W, CI, FD, FH, FW, CO,
        zPadDLeft, zPadDRight, 
        zPadHLeft, zPadHRight, 
        zPadWLeft, zPadWRight,
        strideD, strideH, strideW;
    GroupElement *a, *b, *c;    
};

struct TripleKeyPack {
    int bw;
    int64_t na, nb, nc;
    GroupElement *a, *b, *c;
};

struct ScmpKeyPack
{
    int Bin, Bout;
    DualDCFKeyPack dualDcfKey;
    GroupElement rb;
};

struct PublicICKeyPack
{
    int Bin, Bout;
    DCFKeyPack dcfKey;
    GroupElement zb;
};

struct PublicDivKeyPack
{
    int Bin, Bout;
    DualDCFKeyPack dualDcfKey;
    ScmpKeyPack scmpKey;
    GroupElement zb;
};

struct SignedPublicDivKeyPack
{
    int Bin, Bout;
    GroupElement d;     // divisor
    DCFKeyPack dcfKey;
    PublicICKeyPack publicICkey;
    ScmpKeyPack scmpKey;
    GroupElement A_share, corr_share, B_share, rdiv_share;
    GroupElement rout_temp_share, rout_share;
};

struct ReluKeyPack
{
    int Bin, Bout;
    osuCrypto::block *k;
    GroupElement *g, *v;
    GroupElement e_b0, e_b1;		 // size: degree+1 (same as beta)
    GroupElement beta_b0, beta_b1;	 // size: degree+1 (shares of beta, which is set of poly coeffs) (beta: highest to lowest power left to right)
    GroupElement r_b;
    GroupElement drelu;
};

struct MaxpoolKeyPack
{
    int Bin, Bout;
    ReluKeyPack reluKey;
    GroupElement rb;
};

struct ARSKeyPack
{
    // arithmetic right shift
    int Bin, Bout, shift;
    DCFKeyPack dcfKey;
    DualDCFKeyPack dualDcfKey;      // groupSize = 2 for payload
    GroupElement rb;
    ARSKeyPack() {}
};

struct ReluTruncateKeyPack {
    int Bin, Bout, shift;
    DCFKeyPack dcfKeyN;
    DCFKeyPack dcfKeyS;
    GroupElement zTruncate;
    GroupElement a, b, c, d1, d2;
};

struct Relu2RoundKeyPack {
    int effectiveBin, Bin;
    DCFKeyPack dcfKey;
    GroupElement a, b, c, d1, d2;
};

/*
struct SplineOneKeyPack
{
    int Bin, Bout;
    int degree; // degree of poly in payload beta
    DCFKeyPack dcfKey;
    std::vector<GroupElement> e_b;		 // size: degree+1 (same as beta)
    std::vector<GroupElement> beta_b;	 // size: degree+1 (shares of beta, which is set of poly coeffs) (beta: highest to lowest power left to right)
    GroupElement r_b;
};
*/
struct SplineKeyPack
{
    int Bin, Bout;
    int numPoly, degree;
    DCFKeyPack dcfKey;
    std::vector<GroupElement> p;        // spline breakpoints, size: numPoly + 1; p[0] = 0 and p[numPoly] = N-1
    std::vector<std::vector<GroupElement>> e_b; // 2d array dim: numPoly x (degree+1) (size is same as beta)
    std::vector<GroupElement> beta_b;           // 1d array size: numPoly * (degree+1) (shares of beta, which is set of poly coeffs) (beta: highest to lowest power left to right)
    GroupElement r_b;
};

struct PrivateScaleKeyPack
{
    GroupElement rin;
    GroupElement rout;
};

struct SquareKey {
    GroupElement b;
    GroupElement c;
};

struct TaylorSqKey {
    GroupElement a;
    GroupElement b;
};

struct MICKeyPack {
    DCFKeyPack dcfKey;
    GroupElement *z;
};

struct MSNZBKeyPack {
    MICKeyPack micKey;
    GroupElement r;
};

struct BulkyLRSKeyPack
{
    DCFKeyPack dcfKeyN;
    DCFKeyPack *dcfKeyS;
    GroupElement *z;
    GroupElement out;
};

struct TaylorKeyPack {
    MSNZBKeyPack msnzbKey;
    TaylorSqKey squareKey;
    BulkyLRSKeyPack lrsKeys[2];
    PrivateScaleKeyPack privateScaleKey;
};

struct SelectKeyPack {
    int Bin;
    GroupElement a, b, c, d1, d2;
};

struct MaxpoolDoubleKeyPack
{
    int Bin, Bout;
    Relu2RoundKeyPack reluKey;
    GroupElement rb;
};

struct BitwiseAndKeyPack
{
    GroupElement t[4];
};

struct FixToFloatKeyPack
{
    MICKeyPack micKey;
    GroupElement rs, rpow, ry, rm;
    SelectKeyPack selectKey;
};

struct FloatToFixKeyPack
{
    GroupElement rm, re, rw, /*rt,*/ rh;
    DCFKeyPack dcfKey;
    SelectKeyPack selectKey;
    ARSKeyPack arsKey;
    GroupElement p[1024];
    GroupElement q[1024];
};

struct ReluExtendKeyPack
{
    DCFKeyPack dcfKey;
    GroupElement rd, rw;
    GroupElement p[4];
    GroupElement q[2];
};

struct SignExtend2KeyPack
{
    DCFKeyPack dcfKey;
    GroupElement rw;
    GroupElement p[2];
};

struct EdabitsPrTruncKeyPack
{
    GroupElement a, b;
};

class DPFKeyPack
{
public:
    int bin, bout;
    osuCrypto::block *s;
    union {
        struct {
            uint64_t tLcw;
            uint64_t tRcw;
        };
        uint64_t tcw[2];
    };
    GroupElement payload;

    DPFKeyPack(int bin, int bout) : bin(bin), bout(bout)
    {
        s = new osuCrypto::block[bin+1];
        tLcw = 0;
        tRcw = 0;
    }

    DPFKeyPack()
    {
        s = nullptr;
        tLcw = 0;
        tRcw = 0;
    }
};

class DPFETKeyPack
{
public:
    int bin;
    osuCrypto::block *s;
    union {
        struct {
            uint64_t tLcw;
            uint64_t tRcw;
        };
        uint64_t tcw[2];
    };
    osuCrypto::block leaf;

    DPFETKeyPack(int bin) : bin(bin)
    {
        s = new osuCrypto::block[bin+1-7];
        tLcw = 0;
        tRcw = 0;
    }

    DPFETKeyPack()
    {
        s = nullptr;
        tLcw = 0;
        tRcw = 0;
    }
};

// =========================================================================
// GTDCF KeyPack
// =========================================================================
struct GTDCFKeyPack {
    int bin, w, d, groupSize;
    osuCrypto::block seed;       
    osuCrypto::block *scw;       
    uint8_t *tcw;                
    GroupElement *vcw;           
    GroupElement *leaf_vcw;      
    GroupElement rout_share; // 输出的掩码份额

    GTDCFKeyPack(int bin, int w, int groupSize = 2) 
        : bin(bin), w(w), groupSize(groupSize) {
        d = bin - w;
        int B = 1 << w; 
        scw = new osuCrypto::block[d];
        tcw = new uint8_t[2 * d];
        vcw = new GroupElement[d * groupSize];
        leaf_vcw = new GroupElement[B * groupSize];
        rout_share = 0;
    }

    GTDCFKeyPack() {
        bin = w = d = groupSize = 0;
        scw = nullptr; tcw = nullptr; vcw = nullptr; leaf_vcw = nullptr;
        rout_share = 0;
    }
};



struct PubCmpKeyPack {
    int bin;
    DCFKeyPack dcfKey;
    GroupElement rout;
};

struct ClipKeyPack {
    int bin;
    PubCmpKeyPack cmpKey;
    GroupElement a, b, c, d1, d2;
};

struct LUTKeyPack {
    int bin, bout;
    DPFKeyPack dpfKey;
    GroupElement rout;
};

struct F2BF16KeyPack {
    int bin;
    DCFKeyPack dcfKey, dcfTruncate;
    GroupElement rout_k, rout_m, rin, prod, rout, rProd;
};

struct TruncateReduceKeyPack {
    int bin, shift;
    DCFKeyPack dcfKey;
    GroupElement rout;
};

struct LUTSSKeyPack {
    int bin, bout;
    GroupElement b0, b1, b2, b3;
    GroupElement routRes, routCorr;
    GroupElement rout;
};

struct LUTDPFETKeyPack {
    int bin, bout;
    DPFETKeyPack dpfKey;
    GroupElement routRes, routCorr;
};

struct SlothDreluKeyPack {
    int bin;
    DPFETKeyPack dpfKey;
    GroupElement r;
};

struct WrapSSKeyPack {
    int bin;
    uint64_t b0, b1;
};

struct WrapDPFKeyPack {
    int bin;
    DPFETKeyPack dpfKey;
    GroupElement r;
};

struct SlothLRSKeyPack {
    int bin, shift;
    GroupElement msb;
    GroupElement rout;
    GroupElement select;
};

struct SlothTRKeyPack {
    int bin, shift;
    GroupElement rout;
    GroupElement select;
};

struct SlothSignExtendKeyPack {
    int bin, bout;
    GroupElement rout;
    GroupElement select;
};


// ==== 请添加到 keypack.h 的末尾 ====
class QuadDPFKeyPack
{
public:
    int bin;        // 输入位宽
    int bout;       // 输出位宽
    int depth;      // 树深 ceil(bin / 2)
    osuCrypto::block s0_initial; // 初始种子
    osuCrypto::block *scw;       // Correction Words for seeds (size: depth * 4)
    uint8_t *tcw;                // Correction Words for bits (size: depth) 每一位打包了4个分支的t_cw
    GroupElement payload;        // 叶子节点的 Payload

    QuadDPFKeyPack(int bin, int bout) : bin(bin), bout(bout)
    {
        depth = (bin + 1) / 2;
        scw = new osuCrypto::block[depth * 4];
        tcw = new uint8_t[depth];
    }

    QuadDPFKeyPack()
    {
        scw = nullptr;
        tcw = nullptr;
        bin = bout = depth = 0;
    }
};

class OctDPFKeyPack
{
public:
    int bin;        // 输入位宽
    int bout;       // 输出位宽
    int depth;      // 树深 ceil(bin / 3)
    osuCrypto::block s0_initial; // 初始种子
    osuCrypto::block *scw;       // Correction Words for seeds (size: depth * 8)
    uint8_t *tcw;                // Correction Words for bits (size: depth) 每个字节存8位
    GroupElement payload;        // 叶子节点的 Payload

    OctDPFKeyPack(int bin, int bout) : bin(bin), bout(bout)
    {
        depth = (bin + 2) / 3;
        scw = new osuCrypto::block[depth * 8];
        tcw = new uint8_t[depth];
    }

    OctDPFKeyPack()
    {
        scw = nullptr;
        tcw = nullptr;
        bin = bout = depth = 0;
    }
};

// 可验证DPF——VDPF
// ==========================================
// Verifiable DPF KeyPack
// Based on Castro-Polychroniadou VerDPF.
// Compatible with the existing DPF style:
//   s[0]       : initial seed
//   s[i+1]     : seed correction word scw_i
//   tcw[0/1]   : left/right control-bit correction words
//   cs[4]      : final 4λ-bit correction seed
//   ocw        : output correction word
// ==========================================
struct VerDPFKeyPack
{
    int bin, bout;

    osuCrypto::block *s; // size: bin + 1

    union {
        struct {
            uint64_t tLcw;
            uint64_t tRcw;
        };
        uint64_t tcw[2];
    };

    osuCrypto::block cs[4]; // 4λ-bit final correction seed
    GroupElement ocw;

    VerDPFKeyPack(int bin, int bout) : bin(bin), bout(bout)
    {
        s = new osuCrypto::block[bin + 1];
        tcw[0] = 0;
        tcw[1] = 0;
        ocw = 0;
        for (int i = 0; i < 4; ++i) {
            cs[i] = osuCrypto::ZeroBlock;
        }
    }

    VerDPFKeyPack()
    {
        bin = bout = 0;
        s = nullptr;
        tcw[0] = 0;
        tcw[1] = 0;
        ocw = 0;
        for (int i = 0; i < 4; ++i) {
            cs[i] = osuCrypto::ZeroBlock;
        }
    }
};


// ============================================================
// IFSS authenticated share and MAC key
// ============================================================

struct IFSSAuthShare
{
    GroupElement value;  // value share
    GroupElement tag;    // MAC/tag share
};

struct IFSSGlobalMACKey
{
    GroupElement deltaA0;
    GroupElement deltaA1;
    GroupElement deltaA;
};

// ============================================================
// IFSS_DPF
//
// Since current DPF supports only single payload,
// IFSS_DPF is implemented by two ordinary DPF keys:
//
//   valKey : DPF(alpha, beta)
//   macKey : DPF(alpha, deltaA * beta)
// ============================================================

struct IFSS_DPFKeyPack
{
    int bin, bout;

    DPFKeyPack valKey;
    DPFKeyPack macKey;

    IFSS_DPFKeyPack()
    {
        bin = bout = 0;
    }
};

// ============================================================
// IFSS_DCF_TwoDCF
//
// Two-DCF implementation:
//
//   valKey : DCF(alpha, beta)
//   macKey : DCF(alpha, deltaA * beta)
// ============================================================

struct IFSS_DCF_TwoDCFKeyPack
{
    int bin, bout;

    DCFKeyPack valKey;
    DCFKeyPack macKey;

    IFSS_DCF_TwoDCFKeyPack()
    {
        bin = bout = 0;
    }
};

// ============================================================
// IFSS_DCF
//
// Vector-payload implementation:
//
//   payload[0] = beta
//   payload[1] = deltaA * beta
//
// One DCF key with groupSize = 2.
// ============================================================

struct IFSS_DCFKeyPack
{
    int bin, bout;

    DCFKeyPack dcfKey; // groupSize = 2

    IFSS_DCFKeyPack()
    {
        bin = bout = 0;
    }
};



// ============================================================
// 传统 DIFKeyPack
//
// Distributed Interval Function:
//   DIF([a,b], beta)(x) = beta if a <= x <= b, else 0.
//
// Implementation using existing DCF:
//   beta * {a <= x <= b}
// = beta * {x < b + 1} - beta * {x < a}
//
// For b = 2^bin - 1, the first term is a constant beta.
// For a = 0, the second term is omitted.
// ============================================================

struct DIFKeyPack
{
    int bin, bout;

    bool hasUpperDcf;   // beta * {x < b+1}
    bool hasLowerDcf;   // -beta * {x < a}

    GroupElement constShare; // used when b is max: share of beta

    DCFKeyPack upperKey;
    DCFKeyPack lowerKey;

    DIFKeyPack()
    {
        bin = 0;
        bout = 0;
        hasUpperDcf = false;
        hasLowerDcf = false;
        constShare = 0;
    }
};

// ============================================================================
// Half-Tree DPF / DCF KeyPack
// Put this block in FSS/keypack.h
// ============================================================================

struct HalfTreeDPFKeyPack
{
    int bin, bout;

    // XOR-share of root node X_0 = s_0 || t_0.
    osuCrypto::block root;

    // Engineering convenience:
    // In the paper, hash key S can be setup once and not counted per key.
    // In this implementation we store it in each key for easy local evaluation.
    osuCrypto::block hashKey;

    // Internal correction words CW_1,...,CW_{n-1}.
    // Size = max(bin - 1, 0).
    osuCrypto::block *cw;

    // Compressed final correction word:
    // CW_n = (HCW, LCW_0, LCW_1)
    // hcw stores the high-bit correction with LSB cleared.
    // lcw packs LCW_0 in bit 0 and LCW_1 in bit 1.
    osuCrypto::block hcw;
    uint8_t lcw;

    // Output correction word CW_{n+1} in Z_{2^bout}.
    GroupElement outCW;

    HalfTreeDPFKeyPack()
    {
        bin = bout = 0;
        root = osuCrypto::ZeroBlock;
        hashKey = osuCrypto::ZeroBlock;
        cw = nullptr;
        hcw = osuCrypto::ZeroBlock;
        lcw = 0;
        outCW = 0;
    }

    HalfTreeDPFKeyPack(int bin, int bout) : bin(bin), bout(bout)
    {
        root = osuCrypto::ZeroBlock;
        hashKey = osuCrypto::ZeroBlock;

        int cwSize = (bin > 1) ? (bin - 1) : 0;
        cw = (cwSize > 0) ? new osuCrypto::block[cwSize] : nullptr;

        for (int i = 0; i < cwSize; ++i) {
            cw[i] = osuCrypto::ZeroBlock;
        }

        hcw = osuCrypto::ZeroBlock;
        lcw = 0;
        outCW = 0;
    }
};

struct HalfTreeDCFKeyPack
{
    int bin, bout;

    // DPF key for point-function term f^bullet_{alpha, -alpha_n * beta}.
    HalfTreeDPFKeyPack dpfKey;

    // Prefix correction words VCW_1,...,VCW_n.
    // Size = bin.
    GroupElement *vcw;

    HalfTreeDCFKeyPack()
    {
        bin = bout = 0;
        vcw = nullptr;
    }
};



// ============================================================================
// Four-ary Wide Distributed Comparison Function (WDCF), m = 2
//
// Implements:
//     f^<_{alpha,beta}(x) = beta, if x < alpha
//                           0,    otherwise
//
// Each tree level processes two input bits and therefore has four branches.
// ============================================================================
class WDCFKeyPack
{
public:
    int bin;
    int bout;

    int m;
    int depth;
    int branches;

    // Initial seed share s_b^(0)
    osuCrypto::block seed;

    // Four seed correction words per level.
    // Layout:
    //   scw[level * 4 + branch]
    osuCrypto::block *scw;

    // Four control-bit correction words packed into one byte per level.
    // Bit c:
    //   (tcw[level] >> c) & 1
    uint8_t *tcw;

    // Four value correction words per level.
    // Layout:
    //   vcw[level * 4 + branch]
    GroupElement *vcw;

    // Final CW^(depth+1)
    GroupElement finalCW;

    WDCFKeyPack(int bin, int bout)
        : bin(bin),
          bout(bout),
          m(2),
          depth((bin + 1) / 2),
          branches(4),
          seed(osuCrypto::ZeroBlock),
          finalCW(0)
    {
        scw = new osuCrypto::block[depth * branches];
        tcw = new uint8_t[depth];
        vcw = new GroupElement[depth * branches];

        for (int i = 0; i < depth * branches; ++i)
        {
            scw[i] = osuCrypto::ZeroBlock;
            vcw[i] = 0;
        }

        for (int i = 0; i < depth; ++i)
        {
            tcw[i] = 0;
        }
    }

    WDCFKeyPack()
        : bin(0),
          bout(0),
          m(2),
          depth(0),
          branches(4),
          seed(osuCrypto::ZeroBlock),
          scw(nullptr),
          tcw(nullptr),
          vcw(nullptr),
          finalCW(0)
    {
    }
};



// ============================================================================
// Verifiable DCF
// ============================================================================
//
// VerDCF follows Algorithm 4:
//
//   Gen(1^lambda, alpha, beta) -> (k0, k1)
//   Eval(b, kb, x)             -> (yb, pi_b)
//   Verify(pi0, pi1)           -> Accept / Reject
//
// H : {0,1}^lambda -> {0,1}^{3 lambda}
//
// Since lambda = 128 in the current implementation,
// one H output contains exactly three 128-bit blocks.
// ============================================================================

struct VerDCFHash3
{
    osuCrypto::block w[3];

    VerDCFHash3()
    {
        w[0] = osuCrypto::ZeroBlock;
        w[1] = osuCrypto::ZeroBlock;
        w[2] = osuCrypto::ZeroBlock;
    }
};


// ============================================================================
// VerDCF key share
//
// Each party owns:
//
//   DCFKeyPack dcfKey:
//       s_b^(0)
//       CW^(1), ..., CW^(n)
//       final CW^(n+1)
//
//   cs[i]:
//       H(s_0^(i)) XOR H(s_1^(i))
//
//   betaShare:
//       beta_0 = beta XOR l_0 XOR p
//       beta_1 = l_1 XOR p
//
//   lShare:
//       l_b
//
// ============================================================================

struct VerDCFKeyPack
{
    int bin;
    int bout;

    // Underlying ordinary DCF key.
    DCFKeyPack dcfKey;

    // One 3-lambda-bit correction seed per tree level.
    // Size: bin.
    VerDCFHash3 *cs;

    // beta_b in the VDCF pseudocode.
    GroupElement betaShare;

    // l_b in the VDCF pseudocode.
    GroupElement lShare;


    VerDCFKeyPack()
        : bin(0),
          bout(0),
          cs(nullptr),
          betaShare(0),
          lShare(0)
    {
    }


    VerDCFKeyPack(int bin, int bout)
        : bin(bin),
          bout(bout),
          cs(nullptr),
          betaShare(0),
          lShare(0)
    {
        if (bin > 0)
        {
            cs = new VerDCFHash3[bin];
        }
    }
};


// ============================================================================
// VerDCF proof
//
// Algorithm 4 defines:
//
//   pi_b[0 ... n-1] : path-verification components
//   pi_b[n]         : V_b
//   pi_b[n+1]       : beta_b
//   pi_b[n+2]       : R_b
//   pi_b[n+3]       : l_b
//
// Here the typed representation is:
//
//   path[i]          -> pi_b[i], 3 lambda bits
//   valueShare       -> pi_b[n]
//   betaShare        -> pi_b[n+1]
//   randomMaskXor    -> pi_b[n+2]
//   lShare           -> pi_b[n+3]
//
// ============================================================================

struct VerDCFProof
{
    int bin;
    int bout;

    // Size: bin.
    VerDCFHash3 *path;

    // Arithmetic DCF output share V_b.
    GroupElement valueShare;

    // XOR-shared beta_b.
    GroupElement betaShare;

    // R_b = XOR_i r_b^(i).
    osuCrypto::block randomMaskXor;

    // XOR-shared l_b.
    GroupElement lShare;


    VerDCFProof()
        : bin(0),
          bout(0),
          path(nullptr),
          valueShare(0),
          betaShare(0),
          randomMaskXor(osuCrypto::ZeroBlock),
          lShare(0)
    {
    }


    VerDCFProof(int bin, int bout)
        : bin(bin),
          bout(bout),
          path(nullptr),
          valueShare(0),
          betaShare(0),
          randomMaskXor(osuCrypto::ZeroBlock),
          lShare(0)
    {
        if (bin > 0)
        {
            path = new VerDCFHash3[bin];
        }
    }
};


// ============================================================================
// Return value of VerDCF.Eval
// ============================================================================

struct VerDCFEvalResult
{
    GroupElement value;
    VerDCFProof proof;

    VerDCFEvalResult()
        : value(0)
    {
    }

    VerDCFEvalResult(
        GroupElement value,
        const VerDCFProof &proof
    )
        : value(value),
          proof(proof)
    {
    }
};



// ============================================================================
// VWDCF: Verifiable Wide Distributed Comparison Function
// Four-ary tree, m = 2
// ============================================================================
//
// Input:
//     alpha, x in {0,1}^bin
//
// Function:
//     f_{alpha,beta}(x) = beta, if x < alpha
//                         0,    otherwise
//
// Wide tree:
//     m      = 2
//     arity  = 4
//     depth  = ceil(bin / 2)
//
// H:
//     {0,1}^lambda -> {0,1}^{3 lambda}
//
// lambda = 128 in the current cryptoTools-based implementation.
// ============================================================================


// ----------------------------------------------------------------------------
// 3-lambda-bit hash output.
// ----------------------------------------------------------------------------

struct VWDCFHash3
{
    osuCrypto::block w[3];

    VWDCFHash3()
    {
        w[0] = osuCrypto::ZeroBlock;
        w[1] = osuCrypto::ZeroBlock;
        w[2] = osuCrypto::ZeroBlock;
    }
};


// ----------------------------------------------------------------------------
// VWDCF key share.
//
// For every wide-tree level i:
//
//   scw[i * 4 + c] : seed correction word for branch c
//   tcw[i]          : 4 packed t_CW bits
//   vcw[i * 4 + c] : value correction word for branch c
//   cs[i]           : 3-lambda-bit verification correction seed
//
// dvec[i * 4 + c]:
//   D_b^(i)[c], represented as a 2-bit vector.
//
// For m = 2:
//   bit 1 -> first bit in the 2-bit chunk
//   bit 0 -> second bit in the 2-bit chunk
// ----------------------------------------------------------------------------

struct VWDCFKeyPack
{
    int bin;
    int bout;
    int depth;

    // Initial seed s_b^(0).
    osuCrypto::block root;

    // Size: depth * 4.
    osuCrypto::block *scw;

    // Size: depth.
    // Low four bits store t_CW^0 ... t_CW^3.
    uint8_t *tcw;

    // Size: depth * 4.
    GroupElement *vcw;

    // Size: depth.
    // Each entry contains 3 lambda bits.
    VWDCFHash3 *cs;

    // Size: depth * 4.
    // Party-specific D_b^(i).
    // Only low two bits are used.
    uint8_t *dvec;

    // CW^(depth + 1).
    GroupElement finalCW;

    // beta_b from Algorithm 6.
    GroupElement betaShare;

    // l_b from Algorithm 6.
    GroupElement lShare;


    VWDCFKeyPack()
        : bin(0),
          bout(0),
          depth(0),
          root(osuCrypto::ZeroBlock),
          scw(nullptr),
          tcw(nullptr),
          vcw(nullptr),
          cs(nullptr),
          dvec(nullptr),
          finalCW(0),
          betaShare(0),
          lShare(0)
    {
    }


    VWDCFKeyPack(int bin, int bout)
        : bin(bin),
          bout(bout),
          depth((bin + 1) / 2),
          root(osuCrypto::ZeroBlock),
          finalCW(0),
          betaShare(0),
          lShare(0)
    {
        scw  = new osuCrypto::block[depth * 4];
        tcw  = new uint8_t[depth];
        vcw  = new GroupElement[depth * 4];
        cs   = new VWDCFHash3[depth];
        dvec = new uint8_t[depth * 4];

        for (int i = 0; i < depth * 4; ++i)
        {
            scw[i] = osuCrypto::ZeroBlock;
            vcw[i] = 0;
            dvec[i] = 0;
        }

        for (int i = 0; i < depth; ++i)
        {
            tcw[i] = 0;
        }
    }
};


// ----------------------------------------------------------------------------
// VWDCF proof.
//
// Algorithm 6 has:
//
//   pi[0 ... depth-1]
//       path-verification components.
//
//   pi[depth ... 2*depth-1]
//       D_b^(i)[x_i] components.
//
//   pi[2*depth]
//       V_b.
//
//   pi[2*depth+1]
//       beta_b.
//
//   pi[2*depth+2]
//       R_b.
//
//   pi[2*depth+3]
//       l_b.
//
// We store these components with explicit types instead of forcing them all
// into one heterogeneous vector.
// ----------------------------------------------------------------------------

struct VWDCFProof
{
    int bin;
    int bout;
    int depth;

    // Size: depth.
    // Each path component is 3 lambda bits.
    VWDCFHash3 *path;

    // Size: depth.
    // dProof[i] = d_b^{x_i}.
    // Only low 2 bits are used.
    uint8_t *dProof;

    GroupElement valueShare;
    GroupElement betaShare;

    // R_b = XOR_i r_b^(i).
    osuCrypto::block randomMaskXor;

    GroupElement lShare;


    VWDCFProof()
        : bin(0),
          bout(0),
          depth(0),
          path(nullptr),
          dProof(nullptr),
          valueShare(0),
          betaShare(0),
          randomMaskXor(osuCrypto::ZeroBlock),
          lShare(0)
    {
    }


    VWDCFProof(int bin, int bout)
        : bin(bin),
          bout(bout),
          depth((bin + 1) / 2),
          valueShare(0),
          betaShare(0),
          randomMaskXor(osuCrypto::ZeroBlock),
          lShare(0)
    {
        path = new VWDCFHash3[depth];
        dProof = new uint8_t[depth];

        for (int i = 0; i < depth; ++i)
        {
            dProof[i] = 0;
        }
    }
};


// ----------------------------------------------------------------------------
// Return type of VWDCF.Eval.
// ----------------------------------------------------------------------------

struct VWDCFEvalResult
{
    GroupElement value;
    VWDCFProof proof;

    VWDCFEvalResult()
        : value(0)
    {
    }
};

// ============================================================================
// CAVERN authenticated share.
// ============================================================================

struct CavernAuthShare
{
    WideGroupElement value;
    WideGroupElement mac;
};

// The VIDPF output group contains two coordinates:
//
//   value : function-output share
//   mac   : authentication-tag share
//
using VIDPFPayload = CavernAuthShare;

struct VIDPFHash4
{
    osuCrypto::block h[4];

    VIDPFHash4()
    {
        for (int i = 0; i < 4; ++i)
        {
            h[i] = osuCrypto::ZeroBlock;
        }
    }
};

struct VIDPFToken
{
    osuCrypto::block v[2];

    VIDPFToken()
    {
        v[0] = osuCrypto::ZeroBlock;
        v[1] = osuCrypto::ZeroBlock;
    }
};

struct VIDPFQuery
{
    int length;

    // The prefix is stored in the low `length` bits.
    GroupElement prefix;

    VIDPFQuery()
        : length(0), prefix(0)
    {
    }

    VIDPFQuery(
        int length_,
        GroupElement prefix_)
        : length(length_),
          prefix(prefix_)
    {
    }
};

struct VIDPFKeyPack
{
    int bin;
    int ringBw;

    osuCrypto::block root;

    // size = bin
    osuCrypto::block *scw;

    // size = bin
    // bit 0: left control-bit correction
    // bit 1: right control-bit correction
    uint8_t *tcw;

    // size = bin
    VIDPFPayload *ocw;

    // size = bin
    VIDPFHash4 *cs;

    VIDPFKeyPack()
        : bin(0),
          ringBw(0),
          root(osuCrypto::ZeroBlock),
          scw(nullptr),
          tcw(nullptr),
          ocw(nullptr),
          cs(nullptr)
    {
    }

    VIDPFKeyPack(
        int bin_,
        int ringBw_)
        : bin(bin_),
          ringBw(ringBw_),
          root(osuCrypto::ZeroBlock)
    {
        scw = new osuCrypto::block[bin];
        tcw = new uint8_t[bin];
        ocw = new VIDPFPayload[bin];
        cs = new VIDPFHash4[bin];

        for (int i = 0; i < bin; ++i)
        {
            scw[i] = osuCrypto::ZeroBlock;
            tcw[i] = 0;
        }
    }
};

struct VIDPFEvalResult
{
    std::vector<VIDPFPayload> y;

    // Control bit at the output layer of each query.
    std::vector<uint8_t> t;

    VIDPFToken mu;
};

struct VIDPFKeyGenResult
{
    VIDPFKeyPack key0;
    VIDPFKeyPack key1;

    // Control bits on alpha's path at every level.
    std::vector<uint8_t> t0;
    std::vector<uint8_t> t1;
};

// ============================================================================
// CAVERN authenticated Beaver multiplication.
// ============================================================================

struct CavernTripleShare
{
    CavernAuthShare a;
    CavernAuthShare b;
    CavernAuthShare c;
};

// ============================================================================
// One CAVERN key for one scalar spline evaluation.
// ============================================================================

struct CavernSplineKeyPack
{
    int bin;
    int slack;
    int ringBw;

    int numPoly;
    int degree;

    // P_sigma's share of Delta.
    WideGroupElement deltaShare;

    CavernAuthShare alphaShare;

    // Authenticated constant 1.
    CavernAuthShare betaShare;

    // Authenticated share of -r_in.
    CavernAuthShare negInputMaskShare;

    // Authenticated share of r_out.
    CavernAuthShare outputMaskShare;

    // Public parameters for the second/tag VIDPF.
    GroupElement tagA;
    GroupElement tagB;
    GroupElement tagC;
    GroupElement tagH;

    VIDPFKeyPack mainVIDPF;
    VIDPFKeyPack tagVIDPF;

    // q[i] = (-1)^{t_1^(i)} * m.
    std::vector<WideGroupElement> q;

    // Horner evaluation requires `degree` authenticated multiplications.
    std::vector<CavernTripleShare> polyTriples;

    // Opening masks:
    //
    //   openMasks[0]                  : x-alpha
    //   openMasks[1+2*j]              : e_j
    //   openMasks[1+2*j+1]            : f_j
    //   openMasks[1+2*degree]         : final masked output
    //
    std::vector<CavernAuthShare> openMasks;

    CavernSplineKeyPack()
        : bin(0),
          slack(0),
          ringBw(0),
          numPoly(0),
          degree(0),
          tagA(0),
          tagB(0),
          tagC(0),
          tagH(0)
    {
    }
};

struct CavernReluKeyPack
{
    CavernSplineKeyPack splineKey;
};