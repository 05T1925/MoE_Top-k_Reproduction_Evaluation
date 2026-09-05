# M2.7 CmpAgg process foundation

Implementation label: `m2_cmpagg_process_e2e`.  This is a project extension
(C), not a complete Agarwal Protocol I claim.

| Role | Input / material | Messages | Output |
| --- | --- | --- | --- |
| TEST_ONLY controller | clear scores and oracle | test channels only | reconstructed validation result |
| P2 offline | fresh full node masks and per-edge DCF keys | offline party packages | exits before online |
| P0/P1 | own key/mask shares and own party material | one framed masked-share exchange | additive rank shares |

The only online opened value is the public masked-key vector.  Online frame
counters include headers and payloads; controller/P2 channels are excluded.
The local socketpair test establishes a bounded connected-fd frame and counter
boundary, not WAN performance.  Secure shuffle, reverse routing, final mask
adapter, complete Protocol I rounds, PRG metrics, and network performance are
`NOT_MEASURED`.
