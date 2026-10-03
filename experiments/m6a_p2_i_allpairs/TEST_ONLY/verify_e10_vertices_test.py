"""Check E10 incident-vertex counting against the E5 plaintext CA graph."""

from aav86_ca_reference_TEST_ONLY import Record, run_reference


def main() -> None:
    cases = 0
    for r in (2, 5):
        for seed in (17, 9001):
            scores = [7, 7, -2, 0, 2147483647, -2147483648, 7, 3]
            records = {i: Record(score, i) for i, score in enumerate(scores)}
            _, ctx = run_reference(records, r, seed, True, True)
            for stats, edges in zip(ctx.stats, ctx.edges_by_round):
                participating = {v for a, c in edges for v in (a, c)}
                assert len(participating) == stats.logical_vertices_in_graph
                assert len(edges) == stats.active_edges
                cases += 1
            print(
                f"E10_E5_GRAPH r={r} seed={seed} "
                f"vertices_by_round={[s.logical_vertices_in_graph for s in ctx.stats]} "
                f"edges_by_round={[s.active_edges for s in ctx.stats]}"
            )
    print(f"E10_E5_VERTEX_DEFINITION_PASS rounds={cases}")


if __name__ == "__main__":
    main()
