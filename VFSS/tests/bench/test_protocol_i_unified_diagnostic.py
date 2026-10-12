from __future__ import annotations

import contextlib
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parent))
import protocol_i_unified_diagnostic as diagnostic


class UnifiedPreflightTest(unittest.TestCase):
    def preflight_result(self, n: int, k: int) -> dict:
        with tempfile.TemporaryDirectory() as scratch:
            args = SimpleNamespace(n=n, k=k, scratch_path=scratch)
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                self.assertEqual(diagnostic.preflight(args), 0)
            return json.loads(output.getvalue())

    def test_small_supported_shape_requires_resource_review(self) -> None:
        result = self.preflight_result(128, 2)
        self.assertEqual(result["campaign_admission"], "RESOURCE_REVIEW_REQUIRED")
        self.assertTrue(all(route["status"] == "RESOURCE_REVIEW_REQUIRED"
                            for route in result["routes"].values()))

    def test_n1000_reports_route_specific_rejections(self) -> None:
        result = self.preflight_result(1000, 80)
        self.assertEqual(result["routes"][diagnostic.ROUTES[0]]["status"], "PRECHECK_REJECTED")
        self.assertIn("only", result["routes"][diagnostic.ROUTES[0]]["reason"])
        self.assertIn("D=1024", result["routes"][diagnostic.ROUTES[1]]["reason"])
        self.assertEqual(result["routes"][diagnostic.ROUTES[2]]["status"], "RESOURCE_REVIEW_REQUIRED")
        self.assertEqual(result["campaign_admission"], "PRECHECK_REJECTED")

    def test_n10000_rejects_conditional_secure_v2_bound(self) -> None:
        result = self.preflight_result(10000, 80)
        self.assertEqual(result["routes"][diagnostic.ROUTES[2]]["status"], "PRECHECK_REJECTED")
        self.assertIn("n=10000", result["routes"][diagnostic.ROUTES[2]]["reason"])
        self.assertIn("n<=1000", result["routes"][diagnostic.ROUTES[2]]["reason"])


class CampaignIdentityAuditTest(unittest.TestCase):
    input_hash = "a" * 64
    oracle_hash = "b" * 64

    def make_attempt(self, root: Path, route: str, serial: int,
                     input_hash: str | None = None) -> dict:
        evidence_dir = root / route
        evidence_dir.mkdir(parents=True)
        observed_input = input_hash or self.input_hash
        command = {
            "argv": ["protocol-benchmark", "bench", "128", "2", "771337", str(serial)],
            "shared_input_identity": {
                "n": "128", "K": "2", "input_sha256": observed_input,
                "oracle_mask_sha256": self.oracle_hash,
            },
        }
        (evidence_dir / "command.json").write_text(json.dumps(command), encoding="utf-8")
        return {
            "route": route,
            "input": {"n": 128, "k": 2, "input_sha256": observed_input,
                      "oracle_sha256": self.oracle_hash},
            "counts": {"protocol_attempts_started": True},
            "evidence": {"directory": str(evidence_dir), "files": [{"path": "command.json"}]},
        }

    def make_campaign(self, root: Path, serials=(101, 202, 303), bad_route=None) -> tuple[dict, list[dict]]:
        attempts = [self.make_attempt(root, route, serials[index],
                    "c" * 64 if route == bad_route else None)
                    for index, route in enumerate(diagnostic.ROUTES)]
        campaign = {"configuration": {"n": 128, "k": 2,
                    "input_sha256": self.input_hash, "oracle_sha256": self.oracle_hash}}
        return campaign, attempts

    def test_unique_sessions_and_shared_identity_pass(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            campaign, attempts = self.make_campaign(Path(temporary))
            self.assertEqual(diagnostic.audit_campaign_identity(campaign, attempts), [])

    def test_reused_session_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            campaign, attempts = self.make_campaign(Path(temporary), (101, 101, 303))
            self.assertIn("campaign reused a fresh-session/material serial",
                          diagnostic.audit_campaign_identity(campaign, attempts))

    def test_attempt_input_different_from_campaign_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            campaign, attempts = self.make_campaign(Path(temporary), bad_route=diagnostic.ROUTES[1])
            self.assertIn("campaign attempt input/oracle SHA-256 differs from campaign configuration",
                          diagnostic.audit_campaign_identity(campaign, attempts))

    def test_command_input_identity_must_match_attempt(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            campaign, attempts = self.make_campaign(Path(temporary))
            command_path = Path(attempts[0]["evidence"]["directory"]) / "command.json"
            command = json.loads(command_path.read_text(encoding="utf-8"))
            command["shared_input_identity"]["input_sha256"] = "c" * 64
            command_path.write_text(json.dumps(command), encoding="utf-8")
            errors = diagnostic.audit_campaign_identity(campaign, attempts)
            self.assertTrue(any("command.json input/oracle identity differs" in error for error in errors))

    def test_pre_route_identity_failure_does_not_require_session_command(self) -> None:
        attempts = [{"input": {"n": 128, "k": 2, "input_sha256": None, "oracle_sha256": None},
                     "counts": {"protocol_attempts_started": False}, "evidence": {"directory": "", "files": []}}
                    for _ in diagnostic.ROUTES]
        campaign = {"configuration": {"n": 128, "k": 2, "input_sha256": None, "oracle_sha256": None}}
        self.assertEqual(diagnostic.audit_campaign_identity(campaign, attempts), [])


if __name__ == "__main__":
    unittest.main()
