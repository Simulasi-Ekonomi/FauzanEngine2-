#!/usr/bin/env python3
"""Execute only approved read/build/test operations from a validated dry-run prompt plan.

This helper never shells through user/model text, mutates source, deploys, publishes,
or touches credentials, economy, bans, sessions, or game runtime authority.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any

from prompt_to_game import validate_plan_contract


PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUILD_DIR = PROJECT_ROOT.parents[1] / "build" / "neoengine"
ALLOWED_TEST_TARGETS = {
    "runtime_clock_smoke",
    "runtime_world_vertical_slice_smoke",
    "runtime_authoring_integration_smoke",
    "grid_navigation_smoke",
    "world_authoring_smoke",
    "authoring_catalog_smoke",
    "agent_manual_repair_protocol_smoke",
    "agent_autonomy_policy_smoke",
    "prompt_tool_graph_smoke",
}
ALLOWED_BUILD_TARGETS = {"neo_core"}


def _run(argv: list[str], cwd: Path, timeout_seconds: int = 120) -> dict[str, Any]:
    completed = subprocess.run(argv, cwd=cwd, capture_output=True, text=True, timeout=timeout_seconds, check=False)
    return {
        "argv": argv,
        "exit_code": completed.returncode,
        "stdout_tail": completed.stdout[-4096:],
        "stderr_tail": completed.stderr[-4096:],
    }


def _planned(node: dict[str, Any], reason: str) -> dict[str, Any]:
    return {"node_id": node["node_id"], "request_id": node["request_id"], "kind": node["kind"], "target": node["target"], "status": "requires_human_review", "reason": reason}


def _execute(node: dict[str, Any], build_dir: Path) -> dict[str, Any]:
    kind, target = node["kind"], node["target"]
    if kind == "AuditRuntime":
        result = _run(["git", "diff", "--check", "--", "Source/NeoEngine"], PROJECT_ROOT, 30)
        return {"node_id": node["node_id"], "request_id": node["request_id"], "kind": kind, "target": target, "status": "executed" if result["exit_code"] == 0 else "failed", "receipt": result}
    if kind == "RequestBuild" and target in ALLOWED_BUILD_TARGETS:
        result = _run(["cmake", "--build", str(build_dir), "--target", target, "-j2"], PROJECT_ROOT)
        return {"node_id": node["node_id"], "request_id": node["request_id"], "kind": kind, "target": target, "status": "executed" if result["exit_code"] == 0 else "failed", "receipt": result}
    if kind == "RequestTest" and target in ALLOWED_TEST_TARGETS:
        build = _run(["cmake", "--build", str(build_dir), "--target", target, "-j2"], PROJECT_ROOT)
        if build["exit_code"] != 0:
            return {"node_id": node["node_id"], "request_id": node["request_id"], "kind": kind, "target": target, "status": "failed", "receipt": build}
        test = _run([str(build_dir / target)], build_dir, 60)
        return {"node_id": node["node_id"], "request_id": node["request_id"], "kind": kind, "target": target, "status": "executed" if test["exit_code"] == 0 else "failed", "receipt": {"build": build, "test": test}}
    return _planned(node, "operation_or_target_not_allowlisted")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run confirmed allowlisted prompt-plan operations; source mutation and deploy are impossible here.")
    parser.add_argument("--plan", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--execute", action="store_true", help="Actually run only allowlisted inspect/build/test operations.")
    parser.add_argument("--confirm-prompt-id", default="", help="Must exactly match plan.prompt_id when --execute is used.")
    parser.add_argument("--max-executions", type=int, default=3)
    args = parser.parse_args()
    if not 1 <= args.max_executions <= 5:
        print("PROMPT_OPERATION_REJECTED reason=max_executions", file=sys.stderr)
        return 2
    try:
        payload = json.loads(args.plan.read_text(encoding="utf-8"))
        if payload.get("version") != 1 or payload.get("dry_run") is not True or not isinstance(payload.get("plan"), dict):
            raise ValueError("invalid_plan_payload")
        plan: dict[str, Any] = payload["plan"]
        validate_plan_contract(plan)
        if args.execute and args.confirm_prompt_id != plan["prompt_id"]:
            raise ValueError("confirmation_mismatch")
        build_dir = Path(os.environ.get("FAUZAN_BUILD_DIR", str(DEFAULT_BUILD_DIR))).resolve()
        results: list[dict[str, Any]] = []
        executions = 0
        for node in plan["nodes"]:
            executable = node["kind"] == "AuditRuntime" or (node["kind"] == "RequestBuild" and node["target"] in ALLOWED_BUILD_TARGETS) or (node["kind"] == "RequestTest" and node["target"] in ALLOWED_TEST_TARGETS)
            if not args.execute:
                results.append(_planned(node, "dry_run"))
            elif executable and executions < args.max_executions:
                results.append(_execute(node, build_dir))
                executions += 1
            elif executable:
                results.append(_planned(node, "execution_budget_exhausted"))
            else:
                results.append(_planned(node, "source_mutation_or_rollback_requires_human_review"))
        receipt = {"version": 1, "prompt_id": plan["prompt_id"], "executed": args.execute, "source_mutation": False, "deploy": False, "operations": results}
        args.receipt.write_text(json.dumps(receipt, ensure_ascii=False, indent=2), encoding="utf-8")
        failed = sum(item["status"] == "failed" for item in results)
        print(f"PROMPT_OPERATION_RECEIPT_OK operations={len(results)} executed={executions} failed={failed} source_mutation=0 deploy=0")
        return 1 if failed else 0
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"PROMPT_OPERATION_REJECTED reason={error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
