import hashlib
import json
import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from bmw16_run_record import METRIC_UNITS, MEASURED_METRICS, PROTOCOL_TAG, audit_record


def fixture_record(root: Path) -> Path:
    evidence = root / "evidence.txt"
    evidence.write_text("safe diagnostic output\n", encoding="utf-8")
    stdout = root / "e2e_stdout.txt"
    input_hash = "d" * 64
    stdout.write_text(
        f"benchmark_fixture n=8 K=2 session=10 input_seed_id=seed input_sha256={input_hash} oracle_mask_sha256={'e' * 64}\n" +
        f"benchmark_attempt n=8 K=2 session=10 status=SUCCESS_ORACLE_CHECKED input_sha256={input_hash} oracle_mask_sha256={'e' * 64}\n",
        encoding="utf-8")
    entries = [{"path": item.name, "sha256": hashlib.sha256(item.read_bytes()).hexdigest(),
                "bytes": item.stat().st_size} for item in (evidence, stdout)]
    manifest = root / "evidence_manifest.json"
    manifest_bytes = (json.dumps({"manifest_version": 1, "files": entries}, sort_keys=True,
                                 separators=(",", ":")) + "\n").encode()
    manifest.write_bytes(manifest_bytes)
    record = {
        "schema_version": "2.0", "run_id": "unit-test", "attempt_type": "EXPERIMENTAL_DIAGNOSTIC",
        "protocol_tag": PROTOCOL_TAG, "implementation_label": "conditional_secure_v2",
        "source_commit": "a" * 40, "binary_sha256": "b" * 64, "build_options_sha256": "c" * 64,
        "host": {"os": "test", "kernel": "test", "cpu_model": "test", "vcpu": 1,
                 "ram_bytes": 1, "scratch_bytes": 1, "compiler": "test", "cmake": "test", "openssl": "test"},
        "network": {"mode": "loopback_diagnostic", "rtt_ms": None, "bandwidth_mbit_s": None,
                    "netem_id": None, "online_tls_wire_bytes": None}, "n": 8, "k": 2,
        "dataset_id": "fixture", "input_sha256": "d" * 64, "oracle_sha256": "e" * 64,
        "oracle_implementation_sha256": "f" * 64,
        "input_seed_id": "mt19937_64:seed", "share_split_seed_id": "split",
        "algorithm_randomness": "OS_CSPRNG_fresh_per_attempt", "session_id": "10",
        "terminal_status": "SUCCESS",
        "correctness": {"oracle_checked": True, "oracle_equal": True, "mask_length": 8,
                        "mask_binary": True, "mask_weight_k": True, "mask_published": True},
        "metrics": {"offline_time_ms": 1.0, "offline_material_total_bits": 800,
                    "online_time_ms": 2.0, "online_comm_total_bits": 800,
                    "online_comm_per_party_bits": 400, "online_rounds": 12,
                    "online_prg_calls_total": 50, "comparison_edges_total": 24,
                    "total_time_ms": 3.0},
        "metric_units": METRIC_UNITS,
        "metric_provenance": {name: "MEASURED" for name in MEASURED_METRICS},
        "counts": {"p0_sent_bytes": 50, "p1_sent_bytes": 50,
                   "p0_received_bytes": 50, "p1_received_bytes": 50,
                   "raw_score_ucmp_calls": 16, "select_unique_real_edges": 6,
                   "membership_secure_edges": 2,
                   "p0_dcf_eval_node_expansions": 25, "p1_dcf_eval_node_expansions": 25},
        "auxiliary": {"padded_n": 8, "raw_score_real_ucmp_calls": 16,
                      "raw_score_padding_ucmp_calls": 0}, "raw_evidence_manifest": manifest.name,
        "raw_record_sha256": hashlib.sha256(manifest_bytes).hexdigest(),
    }
    record["metric_provenance"].update({"network.rtt_ms": "NOT_MEASURED",
                                        "network.bandwidth_mbit_s": "NOT_MEASURED",
                                        "network.netem_id": "NOT_MEASURED",
                                        "network.online_tls_wire_bytes": "NOT_MEASURED"})
    path = root / "run_record.json"
    path.write_text(json.dumps(record), encoding="utf-8")
    return path


class RunRecordAuditorTest(unittest.TestCase):
    def test_valid_scalar_metrics_and_provenance(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(audit_record(fixture_record(Path(tmp))), [])

    def test_per_party_must_be_scalar_total_div_two(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = fixture_record(Path(tmp))
            data = json.loads(path.read_text())
            data["metrics"]["online_comm_per_party_bits"] = {"p0_sent_bits": 400, "p1_sent_bits": 400}
            path.write_text(json.dumps(data))
            self.assertTrue(any("per-party communication" in e for e in audit_record(path)))

    def test_null_requires_not_measured(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = fixture_record(Path(tmp))
            data = json.loads(path.read_text())
            data["metrics"]["online_prg_calls_total"] = None
            path.write_text(json.dumps(data))
            self.assertTrue(any("null/provenance mismatch" in e for e in audit_record(path)))

    def test_received_bytes_are_not_added_to_total(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = fixture_record(Path(tmp))
            data = json.loads(path.read_text())
            data["metrics"]["online_comm_total_bits"] = 1600
            data["metrics"]["online_comm_per_party_bits"] = 800
            path.write_text(json.dumps(data))
            self.assertTrue(any("sum(sent)" in e for e in audit_record(path)))

    def test_edges_count_real_executions_once_per_party_pair(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = fixture_record(Path(tmp))
            data = json.loads(path.read_text())
            data["metrics"]["comparison_edges_total"] = 18
            path.write_text(json.dumps(data))
            self.assertTrue(any("comparison_edges_total" in e for e in audit_record(path)))

    def test_non_success_cannot_publish_mask(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = fixture_record(Path(tmp))
            data = json.loads(path.read_text())
            data["terminal_status"] = "PEER_AGREED_ALGORITHM_ABORT"
            data["correctness"]["mask_published"] = False
            path.write_text(json.dumps(data))
            self.assertFalse(any("non-success attempt published" in e for e in audit_record(path)))
            data["correctness"]["mask_published"] = True
            path.write_text(json.dumps(data))
            self.assertTrue(any("non-success attempt published" in e for e in audit_record(path)))

    def test_manifest_hash_is_external_evidence_not_self_hash(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = fixture_record(Path(tmp))
            data = json.loads(path.read_text())
            data["raw_record_sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
            path.write_text(json.dumps(data))
            self.assertTrue(any("evidence manifest bytes" in e for e in audit_record(path)))

    def test_session_must_match_frozen_e2e_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = fixture_record(Path(tmp))
            data = json.loads(path.read_text())
            data["session_id"] = "11"
            path.write_text(json.dumps(data))
            self.assertTrue(any("fixture identity mismatch: session" in e
                                for e in audit_record(path)))

    def test_manifest_path_cannot_escape_record_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = fixture_record(Path(tmp))
            manifest_path = Path(tmp) / "evidence_manifest.json"
            manifest = json.loads(manifest_path.read_text())
            manifest["files"][0]["path"] = "../outside.txt"
            manifest_bytes = (json.dumps(manifest, sort_keys=True, separators=(",", ":")) + "\n").encode()
            manifest_path.write_bytes(manifest_bytes)
            data = json.loads(path.read_text())
            data["raw_record_sha256"] = hashlib.sha256(manifest_bytes).hexdigest()
            path.write_text(json.dumps(data))
            self.assertTrue(any("record-local filename" in e for e in audit_record(path)))

    def test_oracle_hash_must_match_canonical_test_controller_marker(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = fixture_record(Path(tmp))
            data = json.loads(path.read_text())
            data["oracle_sha256"] = "a" * 64
            path.write_text(json.dumps(data))
            self.assertTrue(any("oracle mask hash mismatch" in e for e in audit_record(path)))


if __name__ == "__main__":
    unittest.main()
