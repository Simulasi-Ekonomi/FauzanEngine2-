#!/usr/bin/env python3
"""Smoke prompt operation receipts without model invocation or source mutation."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
RUNNER = ROOT / "prompt_operation_runner.py"


def main() -> int:
    plan = {
        "version": 1,
        "dry_run": True,
        "plan": {
            "prompt_id": "prompt-op-001",
            "summary": "Validated operation sample",
            "risks": [],
            "nodes": [
                {"node_id": "audit", "agent": "CobaAuditor", "kind": "AuditRuntime", "request_id": "request-001", "target": "canonical-source", "dependencies": [], "rationale": "inspect only"},
                {"node_id": "test", "agent": "CobaAuditor", "kind": "RequestTest", "request_id": "request-002", "target": "grid_navigation_smoke", "dependencies": ["audit"], "rationale": "run allowlisted smoke"},
                {"node_id": "template", "agent": "AriesCreator", "kind": "CreateGameTemplate", "request_id": "request-003", "target": "farm", "dependencies": ["test"], "rationale": "must remain review only"},
            ],
        },
    }
    with tempfile.TemporaryDirectory() as directory:
        folder = Path(directory); input_path = folder / "plan.json"; dry_receipt = folder / "dry.json"; execute_receipt = folder / "execute.json"; input_path.write_text(json.dumps(plan), encoding="utf-8")
        dry = subprocess.run([sys.executable, str(RUNNER), "--plan", str(input_path), "--receipt", str(dry_receipt)], capture_output=True, text=True, check=False)
        if dry.returncode != 0: return 1
        dry_data = json.loads(dry_receipt.read_text(encoding="utf-8"))
        if dry_data["executed"] or any(item["status"] != "requires_human_review" for item in dry_data["operations"]): return 1
        execute = subprocess.run([sys.executable, str(RUNNER), "--plan", str(input_path), "--receipt", str(execute_receipt), "--execute", "--confirm-prompt-id", "prompt-op-001"], capture_output=True, text=True, check=False)
        if execute.returncode != 0: return 1
        execute_data = json.loads(execute_receipt.read_text(encoding="utf-8"))
        if not execute_data["executed"] or execute_data["source_mutation"] or execute_data["deploy"] or execute_data["operations"][0]["status"] != "executed" or execute_data["operations"][1]["status"] != "executed" or execute_data["operations"][2]["status"] != "requires_human_review": return 1
    print("PROMPT_OPERATION_RUNNER_SMOKE_OK dry_run=1 audit=executed test=executed template=review source_mutation=0 deploy=0")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
