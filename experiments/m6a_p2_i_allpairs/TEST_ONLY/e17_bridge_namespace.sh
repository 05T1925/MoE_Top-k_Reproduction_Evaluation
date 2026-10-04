#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 4 ]]; then
  echo 'usage: e17_bridge_namespace.sh LAN|WAN output_dir source_root input_plan' >&2
  exit 2
fi
profile=$1
output=$2
source_root=$3
plan=$4
if [[ $(id -u) -ne 0 ]]; then echo 'root required for netns' >&2; exit 2; fi
case "$profile" in
  LAN) delay=0.5ms; rate=1000mbit; target_rtt=1; target_mbps=1000 ;;
  WAN) delay=25ms; rate=100mbit; target_rtt=50; target_mbps=100 ;;
  *) echo 'invalid profile' >&2; exit 2 ;;
esac
ns="m6a-e17-${profile,,}-$$"
cleanup() { ip netns del "$ns" 2>/dev/null || true; }
trap cleanup EXIT
ip netns add "$ns"
ip netns exec "$ns" ip link set lo up
ip netns exec "$ns" tc qdisc add dev lo root netem delay "$delay" rate "$rate"
mkdir -p "$output"
chown moeaudit:moeaudit "$output"
ip netns exec "$ns" tc -s qdisc show dev lo > "$output/${profile}_qdisc_before.txt"
ip netns exec "$ns" runuser -u moeaudit -- prlimit --as=805306368:805306368 -- python3 \
  "$source_root/experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_network_calibrate.py" \
  --profile "$profile" --target-rtt-ms "$target_rtt" --target-mbps "$target_mbps" \
  --output "$output/${profile}_calibration.json"
ip netns exec "$ns" tc -s qdisc show dev lo > "$output/${profile}_qdisc_after_calibration.txt"
ip netns exec "$ns" runuser -u moeaudit -- prlimit --as=805306368:805306368 -- python3 \
  "$source_root/experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_bridge_n128.py" \
  --source-root "$source_root" --output-dir "$output" --input-plan "$plan" \
  --calibration "$output/${profile}_calibration.json" --profile "$profile" \
  --aav86-binary /tmp/m6a16-emp-release/moe_topk_m6a7_aav86_small_e2e_test \
  --baseline-binary /tmp/m6a16-emp-release/moe_topk_m6a12_protocol_i_clique_benchmark_test
ip netns exec "$ns" tc -s qdisc show dev lo > "$output/${profile}_qdisc_after.txt"
chown -R moeaudit:moeaudit "$output"
