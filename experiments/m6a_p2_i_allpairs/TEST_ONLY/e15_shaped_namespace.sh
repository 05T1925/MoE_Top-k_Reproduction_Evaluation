#!/usr/bin/env bash
set -euo pipefail

# Run as root in Ubuntu-24.04 WSL2. The namespace and qdisc are removed on exit.
if [[ $# -ne 5 ]]; then
  echo 'usage: e15_shaped_namespace.sh LAN|WAN binary output_dir source_root input_plan' >&2
  exit 2
fi
profile=$1
binary=$2
output_dir=$3
source_root=$4
input_plan=$5
if [[ $(id -u) -ne 0 ]]; then
  echo 'network namespace setup requires root' >&2
  exit 2
fi
case "$profile" in
  LAN) delay=0.5ms; rate=1000mbit; target_rtt=1; target_mbps=1000 ;;
  WAN) delay=25ms; rate=100mbit; target_rtt=50; target_mbps=100 ;;
  *) echo 'invalid network profile' >&2; exit 2 ;;
esac
namespace="m6a-e15-${profile,,}-$$"
cleanup() { ip netns del "$namespace" 2>/dev/null || true; }
trap cleanup EXIT
ip netns add "$namespace"
ip netns exec "$namespace" ip link set lo up
ip netns exec "$namespace" tc qdisc add dev lo root netem delay "$delay" rate "$rate"
mkdir -p "$output_dir"
chown moeaudit:moeaudit "$output_dir"
ip netns exec "$namespace" tc -s qdisc show dev lo > "$output_dir/${profile}_qdisc_before.txt"
chown moeaudit:moeaudit "$output_dir/${profile}_qdisc_before.txt"
ip netns exec "$namespace" runuser -u moeaudit -- \
  prlimit --as=805306368:805306368 -- python3 \
  "$source_root/experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_network_calibrate.py" \
  --profile "$profile" --target-rtt-ms "$target_rtt" \
  --target-mbps "$target_mbps" --output "$output_dir/${profile}_calibration.json"
ip netns exec "$namespace" tc -s qdisc show dev lo > "$output_dir/${profile}_qdisc_after_calibration.txt"
chown moeaudit:moeaudit "$output_dir/${profile}_qdisc_after_calibration.txt"
ip netns exec "$namespace" runuser -u moeaudit -- \
  prlimit --as=805306368:805306368 -- python3 \
  "$source_root/experiments/m6a_p2_i_allpairs/TEST_ONLY/e15_run_matrix.py" \
  --binary "$binary" --source-root "$source_root" --profile "$profile" \
  --calibration "$output_dir/${profile}_calibration.json" --output-dir "$output_dir" \
  --input-plan "$input_plan"
ip netns exec "$namespace" tc -s qdisc show dev lo > "$output_dir/${profile}_qdisc_after.txt"
chown moeaudit:moeaudit "$output_dir/${profile}_qdisc_after.txt"
