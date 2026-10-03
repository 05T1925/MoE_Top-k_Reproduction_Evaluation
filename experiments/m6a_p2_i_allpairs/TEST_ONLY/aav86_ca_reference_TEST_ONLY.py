#!/usr/bin/env python3
"""TEST_ONLY cleartext AAV86/CA reference; not a secure protocol.
Sources: AAV86 FOCS 1986 section 3.1/Theorem 3.1; Agarwal et al. CCS 2024
sections 5.2-5.3, Algorithms 1-2. Keys, pivots, ranks and traces are cleartext.
"""
from __future__ import annotations
import argparse, bisect, csv, itertools, json, random
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Set, Tuple

HERE=Path(__file__).resolve().parent
E1_DIR=HERE.parent
SIZES=[1,2,3,4,5,8,16,32,128,256,1000,10000,100000]
R_COUNTS=[2,3,4,5]
SEEDS=[(1,1001),(7,1007),(42,1042)]
HANDLE_XOR=0x4D36415F48414E44

@dataclass(frozen=True)
class Record:
    score:int
    original_index:int
    @property
    def priority(self)->Tuple[int,int]:
        # Project TEST_ONLY extension: score descending, original_index ascending.
        return (-self.score,self.original_index)

@dataclass
class RoundStats:
    iteration:int
    subproblems_entered:int=0
    graph_subproblems:int=0
    singleton_subproblems_entered:int=0
    logical_vertices_in_graph:int=0
    pivots:int=0
    pivot_pivot_edges:int=0
    pivot_nonpivot_edges:int=0
    active_edges:int=0
    bucket_slots_created:int=0
    empty_bucket_slots:int=0
    singleton_bucket_slots:int=0
    nontrivial_bucket_slots:int=0
    nonterminal_empty_bucket_slots:int=0
    nonterminal_singleton_bucket_slots:int=0
    nonterminal_nontrivial_bucket_slots:int=0
    nonempty_child_subproblems_created:int=0
    terminal_bucket_slots:int=0
    terminal_empty_bucket_slots:int=0
    terminal_singleton_bucket_slots:int=0
    terminal_nontrivial_bucket_slots:int=0

@dataclass
class RunContext:
    records:Dict[int,Record]
    rounds:int
    pivot_seed:int
    enumerate_edges:bool
    collect_trace:bool
    rng:random.Random=field(init=False)
    stats:List[RoundStats]=field(init=False)
    edges_by_round:List[List[Tuple[int,int]]]=field(init=False)
    trace:List[dict]=field(default_factory=list)
    def __post_init__(self):
        if self.rounds<1: raise ValueError("rounds must be >= 1")
        if not self.records: raise ValueError("logical_n must be >= 1")
        if len({x.original_index for x in self.records.values()})!=len(self.records):
            raise ValueError("original_index values must be unique")
        self.rng=random.Random(self.pivot_seed)
        self.stats=[RoundStats(i+1) for i in range(self.rounds)]
        self.edges_by_round=[[] for _ in range(self.rounds)]

def ceil_nth_root(m:int,d:int)->int:
    """Exact integer ceil(m**(1/d)), no floating-point boundary ambiguity."""
    if m<1 or d<1: raise ValueError("m and d must be >= 1")
    lo,hi=1,1
    while hi**d<m: hi*=2
    while lo<hi:
        mid=(lo+hi)//2
        if mid**d>=m: hi=mid
        else: lo=mid+1
    return lo

def pair(a:int,b:int)->Tuple[int,int]:
    if a==b: raise ValueError("self edge")
    return (a,b) if a<b else (b,a)

def graph_edges(vertices:Sequence[int],pivots:Set[int])->List[Tuple[int,int]]:
    """Pivot clique plus complete pivot/nonpivot bipartite graph."""
    nonp=[v for v in vertices if v not in pivots]
    result={pair(a,b) for a,b in itertools.combinations(pivots,2)}
    result.update(pair(p,v) for p in pivots for v in nonp)
    return sorted(result)

def local_ranks_and_buckets(vertices:Sequence[int],pivots:Sequence[int],records):
    """Evaluate CA LRank using the graph's exact neighborhood structure.

    Each nonpivot is adjacent to all pivots, so its rank is its number of
    preceding pivots. Each pivot is adjacent to every other vertex, so its rank
    is its full-subproblem stable rank. This optimizes evaluation only.
    """
    ps=set(pivots)
    nonp=[v for v in vertices if v not in ps]
    porder=sorted(pivots,key=lambda v:records[v].priority)
    pkeys=[records[v].priority for v in porder]
    full=sorted(vertices,key=lambda v:records[v].priority)
    fullrank={v:i for i,v in enumerate(full)}
    ranks={p:fullrank[p] for p in pivots}
    buckets=[[] for _ in range(len(pivots)+1)]
    for v in nonp:
        b=bisect.bisect_left(pkeys,records[v].priority)
        ranks[v]=b
        buckets[b].append(v)
    return ranks,porder,buckets

def reference_sort(handles:Sequence[int],ctx:RunContext,depth:int,
                  path:str="root",parent:str="")->List[int]:
    """Recursive cleartext TEST_ONLY AAV86/CA execution."""
    iteration=ctx.rounds-depth+1
    st=ctx.stats[iteration-1]
    st.subproblems_entered+=1
    vertices=list(handles); m=len(vertices)
    if m==0: raise AssertionError("empty buckets are counted but not recursed")
    if m==1:
        st.singleton_subproblems_entered+=1
        if ctx.collect_trace:
            ctx.trace.append(dict(iteration=iteration,remaining_depth=depth,
                subproblem_id=path,parent_id=parent,state="SINGLETON_NOOP",
                vertices=vertices,pivots=[],local_ranks={str(vertices[0]):0},
                buckets=[],edges=[]))
        return vertices
    st.graph_subproblems+=1
    st.logical_vertices_in_graph+=m
    t=ceil_nth_root(m,depth); q=t-1
    if not 1<=q<m: raise AssertionError(f"invalid m={m}, d={depth}, t={t}")
    # Preserve E1 deterministic sample and depth-first bucket visitation order.
    chosen=ctx.rng.sample(vertices,q)
    ps=set(chosen); nonp=[v for v in vertices if v not in ps]
    ranks,porder,buckets=local_ranks_and_buckets(vertices,chosen,ctx.records)
    pp=q*(q-1)//2; pn=q*(m-q); edge_count=pp+pn
    st.pivots+=q; st.pivot_pivot_edges+=pp
    st.pivot_nonpivot_edges+=pn; st.active_edges+=edge_count
    st.bucket_slots_created+=len(buckets)
    empty=sum(not b for b in buckets)
    single=sum(len(b)==1 for b in buckets)
    many=sum(len(b)>1 for b in buckets)
    st.empty_bucket_slots+=empty; st.singleton_bucket_slots+=single
    st.nontrivial_bucket_slots+=many
    listed=[]
    if ctx.enumerate_edges:
        listed=graph_edges(vertices,ps)
        if len(listed)!=edge_count or len(set(listed))!=len(listed):
            raise AssertionError("edge enumeration/count/uniqueness mismatch")
        layer=ctx.edges_by_round[iteration-1]
        if set(layer).intersection(listed): raise AssertionError("same-round duplicate edge")
        layer.extend(listed)
    terminal=depth==1
    if terminal:
        st.terminal_bucket_slots+=len(buckets)
        st.terminal_empty_bucket_slots+=empty
        st.terminal_singleton_bucket_slots+=single
        st.terminal_nontrivial_bucket_slots+=many
    else:
        st.nonterminal_empty_bucket_slots+=empty
        st.nonterminal_singleton_bucket_slots+=single
        st.nonterminal_nontrivial_bucket_slots+=many
        st.nonempty_child_subproblems_created+=sum(bool(b) for b in buckets)
    if ctx.collect_trace:
        ctx.trace.append(dict(iteration=iteration,remaining_depth=depth,
            subproblem_id=path,parent_id=parent,
            state="TERMINAL_GRAPH" if terminal else "GRAPH",vertices=vertices,
            pivots=list(chosen),nonpivots=nonp,local_ranks={str(v):ranks[v] for v in vertices},
            pivot_order=porder,buckets=buckets,
            edges=[list(e) for e in listed],edge_count=edge_count,
            pivot_pivot_edges=pp,pivot_nonpivot_edges=pn))
    if terminal:
        # d=1 gives q=m-1; Algorithm 1/2 still buckets, then stops recursion.
        children=buckets
    else:
        children=[]
        for i,b in enumerate(buckets):
            children.append(reference_sort(b,ctx,depth-1,f"{path}/{i}",path) if b else [])
    out=[]
    for i,p in enumerate(porder):
        out.extend(children[i]); out.append(p)
    out.extend(children[-1])
    if len(out)!=m or len(set(out))!=m or set(out)!=set(vertices):
        raise AssertionError("recombination lost or duplicated an item")
    return out

def run_reference(records:Dict[int,Record],rounds:int,pivot_seed:int,
                  enumerate_edges=False,collect_trace=False):
    ctx=RunContext(records,rounds,pivot_seed,enumerate_edges,collect_trace)
    return reference_sort(list(records),ctx,rounds),ctx

def make_random_records(n:int,input_seed:int):
    """Reproduce E1 score and handle mapping generation."""
    if n<1: raise ValueError("n must be >= 1")
    rng=random.Random(input_seed); scores=[]
    for _ in range(n):
        raw=rng.getrandbits(32)
        scores.append(raw-(1<<32) if raw&(1<<31) else raw)
    originals=list(range(n))
    random.Random(input_seed^HANDLE_XOR).shuffle(originals)
    return {h:Record(scores[o],o) for h,o in enumerate(originals)},list(range(n))

def padded_n(n:int)->int:
    if n<1: raise ValueError("logical_n must be >= 1")
    return max(2,1<<(n-1).bit_length())

def topk_mask(sorted_handles,records,k):
    if not 0<=k<=len(records): raise ValueError("k outside [0,logical_n]")
    mask=[0]*len(records)
    for h in sorted_handles[:k]: mask[records[h].original_index]=1
    return mask

def independent_edge_ranks(vertices,edges,records):
    """Brute-force local-stable-rank formula for canonical endpoint u<v."""
    ranks={v:0 for v in vertices}
    for u,v in edges:
        if u not in ranks or v not in ranks: raise AssertionError("nonlogical endpoint")
        # LRank(u) adds if key(u)>key(v); LRank(v) adds if key(u)<=key(v).
        if records[u].priority>records[v].priority: ranks[u]+=1
        else: ranks[v]+=1
    return ranks

def audit_trace(ctx:RunContext):
    used=set(); by_round={}
    for node in ctx.trace:
        it=node["iteration"]; by_round.setdefault(it,[]).append(node)
        if node["state"]=="SINGLETON_NOOP":
            if len(node["vertices"])!=1: raise AssertionError("bad singleton")
            continue
        vs=node["vertices"]; ps=set(node["pivots"]); ns=set(node["nonpivots"])
        if not ps or not ps<set(vs) or ps&ns or ps|ns!=set(vs):
            raise AssertionError("pivot/nonpivot partition invalid")
        expected={pair(a,b) for a,b in itertools.combinations(vs,2) if a in ps or b in ps}
        edges={tuple(e) for e in node["edges"]}
        if edges!=expected or len(edges)!=len(node["edges"]):
            raise AssertionError("graph violates source edge definition")
        actual=independent_edge_ranks(vs,node["edges"],ctx.records)
        recorded={int(k):v for k,v in node["local_ranks"].items()}
        if actual!=recorded: raise AssertionError("CA LRank mismatch")
        if node["pivot_order"]!=sorted(ps,key=lambda h:ctx.records[h].priority):
            raise AssertionError("pivot local ranks/order mismatch")
        buckets=node["buckets"]; flat=[v for b in buckets for v in b]
        if len(flat)!=len(set(flat)) or set(flat)!=ns:
            raise AssertionError("buckets do not disjointly cover nonpivots")
        keys=[ctx.records[p].priority for p in node["pivot_order"]]
        for i,b in enumerate(buckets):
            for v in b:
                key=ctx.records[v].priority
                if i and not keys[i-1]<key: raise AssertionError("bucket before left pivot")
                if i<len(keys) and not key<keys[i]: raise AssertionError("bucket after right pivot")
                if node["remaining_depth"]>1 and len(b)>=len(vs):
                    raise AssertionError("recursive child not smaller")
        if node["remaining_depth"]==1 and len(vs)>1:
            if len(node["pivot_order"])!=len(vs)-1 or len(flat)!=1:
                raise AssertionError("terminal layer must have m-1 pivots and one nonpivot")
        for e in edges:
            if e in used: raise AssertionError("edge repeats across layers/subproblems")
            used.add(e)
    for it,nodes in by_round.items():
        sets=[set(x["vertices"]) for x in nodes if x["state"]!="SINGLETON_NOOP"]
        for a,b in itertools.combinations(sets,2):
            if a&b: raise AssertionError(f"same-round overlap: {it}")

def self_test():
    exhaustive=random_runs=tie_runs=0
    for n in range(1,6):
        for perm in itertools.permutations(range(n)):
            rec={h:Record(-perm[h],h) for h in range(n)}
            oracle=sorted(rec,key=lambda h:rec[h].priority)
            for r in range(1,5):
                for seed in (17+n,9001+n):
                    got,ctx=run_reference(rec,r,seed,True,True)
                    if got!=oracle: raise AssertionError(("exhaustive sort",n,perm,r,seed))
                    audit_trace(ctx)
                    for k in {1,max(1,n//2),n}:
                        mask=[int(h in oracle[:k]) for h in range(n)]
                        if topk_mask(got,rec,k)!=mask: raise AssertionError(("mask",n,k))
                    exhaustive+=1
    for n in (2,3,5,8,13,32):
        for iseed in (1,7,42,20260930):
            rec,_=make_random_records(n,iseed); oracle=sorted(rec,key=lambda h:rec[h].priority)
            for r in (1,2,3,4,5):
                seed=50000+101*n+iseed*7+r
                got,ctx=run_reference(rec,r,seed,True,True)
                if got!=oracle: raise AssertionError(("random sort",n,iseed,r))
                audit_trace(ctx)
                for k in {1,max(1,n//2),n}:
                    if sum(topk_mask(got,rec,k))!=k: raise AssertionError(("mask cardinality",n,k))
                random_runs+=1
    fixtures=[[5,5,5,5],[7,-2,7,-2,0,0],[-(1<<31),(1<<31)-1,-1,0,(1<<31)-1]]
    for values in fixtures:
        mapping=[(i+1)%len(values) for i in range(len(values))]
        rec={h:Record(values[o],o) for h,o in enumerate(mapping)}
        for r in (1,2,3,4):
            got,ctx=run_reference(rec,r,771+len(values),True,True)
            if got!=sorted(rec,key=lambda h:rec[h].priority): raise AssertionError(("tie",values,r))
            audit_trace(ctx); tie_runs+=1
    print(f"SELF_TEST PASS exhaustive_cases={exhaustive} random_cases={random_runs} tie_fixture_runs={tie_runs}; graph/rank/bucket/recursion/repetition and K=1/mid/n masks")

ROUND_FIELDS=["run_id","logical_n","padded_n_informational","r","input_seed","pivot_seed",
"iteration","remaining_depth","subproblems_entered","graph_subproblems","singleton_subproblems_entered",
"logical_vertices_in_graph","pivots","pivot_pivot_edges","pivot_nonpivot_edges","active_edges",
"bucket_slots_created","empty_bucket_slots","singleton_bucket_slots","nontrivial_bucket_slots",
"nonterminal_empty_bucket_slots","nonterminal_singleton_bucket_slots","nonterminal_nontrivial_bucket_slots",
"nonempty_child_subproblems_created","terminal_bucket_slots","terminal_empty_bucket_slots",
"terminal_singleton_bucket_slots","terminal_nontrivial_bucket_slots","edge_audit_basis"]
RUN_FIELDS=["run_id","logical_n","padded_n_informational","r","input_seed","pivot_seed",
"run_total_active_edges","max_active_edges_observed","max_active_edges_observed_round",
"logical_pool_slots_per_party_per_round","logical_pool_slots_per_party_full_protocol",
"active_edge_slots_used_per_party","logical_pool_slots_unused_per_party","logical_pool_used_ratio",
"conditional_padded_pool_slots_per_party_per_round","conditional_padded_pool_slots_per_party_full_protocol",
"conditional_padded_pool_slots_unused_per_party","conditional_padded_pool_used_ratio","edge_audit_basis"]

def write_csv(path,fields,rows):
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=fields); w.writeheader(); w.writerows(rows)

def matrix_rows():
    rows=[]; summaries=[]
    for n in SIZES:
      for r in R_COUNTS:
       for iseed,pseed in SEEDS:
        rid=f"n{n}_r{r}_is{iseed}_ps{pseed}"
        rec,_=make_random_records(n,iseed)
        _,ctx=run_reference(rec,r,pseed,n<=32,False)
        counts=[x.active_edges for x in ctx.stats]
        total=sum(counts); maxe=max(counts,default=0); maxround=counts.index(maxe)+1
        cap=n*(n-1)//2; fullcap=r*cap; pn=padded_n(n); pcap=pn*(pn-1)//2
        basis="EXHAUSTIVE_ENUMERATION_AND_DUPLICATE_CHECK" if n<=32 else "PIVOT_REMOVAL_STRUCTURAL_ARGUMENT"
        if n<=32:
            all_edges=set()
            for layer in ctx.edges_by_round:
                if len(layer)!=len(set(layer)) or all_edges.intersection(layer):
                    raise AssertionError(f"edge repetition: {rid}")
                all_edges.update(layer)
        for s in ctx.stats:
            rows.append(dict(run_id=rid,logical_n=n,padded_n_informational=pn,r=r,
              input_seed=iseed,pivot_seed=pseed,iteration=s.iteration,
              remaining_depth=r-s.iteration+1,subproblems_entered=s.subproblems_entered,
              graph_subproblems=s.graph_subproblems,
              singleton_subproblems_entered=s.singleton_subproblems_entered,
              logical_vertices_in_graph=s.logical_vertices_in_graph,pivots=s.pivots,
              pivot_pivot_edges=s.pivot_pivot_edges,pivot_nonpivot_edges=s.pivot_nonpivot_edges,
              active_edges=s.active_edges,bucket_slots_created=s.bucket_slots_created,
              empty_bucket_slots=s.empty_bucket_slots,singleton_bucket_slots=s.singleton_bucket_slots,
              nontrivial_bucket_slots=s.nontrivial_bucket_slots,
              nonterminal_empty_bucket_slots=s.nonterminal_empty_bucket_slots,
              nonterminal_singleton_bucket_slots=s.nonterminal_singleton_bucket_slots,
              nonterminal_nontrivial_bucket_slots=s.nonterminal_nontrivial_bucket_slots,
              nonempty_child_subproblems_created=s.nonempty_child_subproblems_created,
              terminal_bucket_slots=s.terminal_bucket_slots,
              terminal_empty_bucket_slots=s.terminal_empty_bucket_slots,
              terminal_singleton_bucket_slots=s.terminal_singleton_bucket_slots,
              terminal_nontrivial_bucket_slots=s.terminal_nontrivial_bucket_slots,
              edge_audit_basis=basis))
        summaries.append(dict(run_id=rid,logical_n=n,padded_n_informational=pn,r=r,
          input_seed=iseed,pivot_seed=pseed,run_total_active_edges=total,
          max_active_edges_observed=maxe,max_active_edges_observed_round=maxround,
          logical_pool_slots_per_party_per_round=cap,
          logical_pool_slots_per_party_full_protocol=fullcap,
          active_edge_slots_used_per_party=total,
          logical_pool_slots_unused_per_party=fullcap-total,
          logical_pool_used_ratio=f"{total/fullcap:.12f}" if fullcap else "0",
          conditional_padded_pool_slots_per_party_per_round=pcap,
          conditional_padded_pool_slots_per_party_full_protocol=r*pcap,
          conditional_padded_pool_slots_unused_per_party=r*pcap-total,
          conditional_padded_pool_used_ratio=f"{total/(r*pcap):.12f}" if r*pcap else "0",
          edge_audit_basis=basis))
    return rows,summaries

def compare_e1(rows,summaries,old_path):
    with old_path.open(newline="",encoding="utf-8") as f: old=list(csv.DictReader(f))
    om={(x["run_id"],int(x["iteration"])):x for x in old}
    nm={(x["run_id"],int(x["iteration"])):x for x in rows}
    if set(om)!=set(nm): raise AssertionError("E1/E5 run-round keys differ")
    pairs=[("active_subproblems","subproblems_entered"),("nontrivial_subproblems","graph_subproblems"),
      ("logical_nodes","logical_vertices_in_graph"),("singleton_carries","singleton_subproblems_entered"),
      ("pivots","pivots"),("pivot_pivot_edges","pivot_pivot_edges"),
      ("pivot_element_edges","pivot_nonpivot_edges"),("total_edges","active_edges")]
    for key,o in om.items():
        n=nm[key]
        for oldf,newf in pairs:
            if int(o[oldf])!=int(n[newf]): raise AssertionError(f"E1 mismatch {key}:{oldf}")
        # E1 assigns empty-call count to child iteration; v2 assigns bucket creation to parent.
        expected=0
        if int(n["iteration"])>1:
            expected=int(nm[(key[0],int(n["iteration"])-1)]["nonterminal_empty_bucket_slots"])
        if int(o["empty_buckets"])!=expected: raise AssertionError(f"empty bucket shift mismatch {key}")
    sm={x["run_id"]:x for x in summaries}
    for rid,s in sm.items():
        o=om[(rid,1)]
        for f,v in [("run_total_edges",s["run_total_active_edges"]),
          ("M_observed_sample_max",s["max_active_edges_observed"]),
          ("pool_capacity_formula_slots",s["logical_pool_slots_per_party_full_protocol"]),
          ("pool_slots_used",s["active_edge_slots_used_per_party"]),
          ("pool_slots_unused",s["logical_pool_slots_unused_per_party"])]:
            if int(o[f])!=int(v): raise AssertionError(f"E1 summary mismatch {rid}:{f}")
    return len(old),len(sm)

def write_matrix(out,old):
    rows,summaries=matrix_rows()
    rp=out/"aav86_ca_counts_E5_v2_TEST_ONLY.csv"
    sp=out/"aav86_ca_runs_E5_v2_TEST_ONLY.csv"
    write_csv(rp,ROUND_FIELDS,rows); write_csv(sp,RUN_FIELDS,summaries)
    if old:
        nr,nu=compare_e1(rows,summaries,old)
        print(f"E1_COMPARE PASS rows={nr} runs={nu}; edges/pivots/vertices/subproblems/empty-bucket offset/capacity")
    print(f"MATRIX_WRITTEN runs={len(summaries)} round_rows={len(rows)} round_csv={rp} run_csv={sp}")

def write_fixture(out):
    mapping=[3,0,4,1,2]
    scores=[(1<<31)-1,7,7,-4,-(1<<31)]
    rec={h:Record(scores[o],o) for h,o in enumerate(mapping)}
    result,ctx=run_reference(rec,2,20260930,True,True); audit_trace(ctx)
    counts=[s.active_edges for s in ctx.stats]
    if counts!=[7,1]: raise AssertionError(f"fixture edges changed: {counts}")
    if result!=sorted(rec,key=lambda h:rec[h].priority): raise AssertionError("fixture sort mismatch")
    fields=["logical_n","padded_n_informational","r","input_seed","pivot_seed","iteration",
    "remaining_depth","subproblem_id","parent_id","state","vertices_json","pivots_json",
    "nonpivots_json","local_ranks_json","pivot_order_json","buckets_json","edges_json","edge_count",
    "pivot_pivot_edges","pivot_nonpivot_edges","terminal_empty_bucket_slots",
    "terminal_singleton_bucket_slots","stable_sorted_handles_json","topk_k1_original_order_mask_json"]
    rows=[]
    for x in ctx.trace:
        s=ctx.stats[x["iteration"]-1]
        rows.append(dict(logical_n=5,padded_n_informational=padded_n(5),r=2,input_seed="FIXTURE",
          pivot_seed=20260930,iteration=x["iteration"],remaining_depth=x["remaining_depth"],
          subproblem_id=x["subproblem_id"],parent_id=x["parent_id"],state=x["state"],
          vertices_json=json.dumps(x["vertices"]),pivots_json=json.dumps(x.get("pivots",[])),
          nonpivots_json=json.dumps(x.get("nonpivots",[])),
          local_ranks_json=json.dumps(x.get("local_ranks",{}),sort_keys=True),
          pivot_order_json=json.dumps(x.get("pivot_order",[])),
          buckets_json=json.dumps(x.get("buckets",[])),edges_json=json.dumps(x.get("edges",[])),
          edge_count=x.get("edge_count",0),pivot_pivot_edges=x.get("pivot_pivot_edges",0),
          pivot_nonpivot_edges=x.get("pivot_nonpivot_edges",0),
          terminal_empty_bucket_slots=s.terminal_empty_bucket_slots if x["state"]=="TERMINAL_GRAPH" else 0,
          terminal_singleton_bucket_slots=s.terminal_singleton_bucket_slots if x["state"]=="TERMINAL_GRAPH" else 0,
          stable_sorted_handles_json=json.dumps(result),
          topk_k1_original_order_mask_json=json.dumps(topk_mask(result,rec,1))))
    path=out/"aav86_ca_fixture_trace_E5_v2_TEST_ONLY.csv"
    write_csv(path,fields,rows)
    print(f"FIXTURE PASS logical_n=5 r=2 edges={counts} stable_sort/mask=PASS trace_rows={len(rows)} path={path}")

def main():
    p=argparse.ArgumentParser(description="TEST_ONLY cleartext AAV86/CA reference and E1 count v2")
    sub=p.add_subparsers(dest="command",required=True)
    sub.add_parser("self-test")
    m=sub.add_parser("matrix"); m.add_argument("--out-dir",type=Path,default=HERE)
    m.add_argument("--compare-e1",action="store_true")
    m.add_argument("--old-csv",type=Path,default=E1_DIR/"aav86_graph_counts.csv")
    f=sub.add_parser("fixture"); f.add_argument("--out-dir",type=Path,default=HERE)
    a=p.parse_args()
    if a.command=="self-test": self_test()
    elif a.command=="matrix": write_matrix(a.out_dir,a.old_csv if a.compare_e1 else None)
    elif a.command=="fixture": write_fixture(a.out_dir)
if __name__=="__main__": main()
