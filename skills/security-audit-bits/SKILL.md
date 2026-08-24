---
name: security-audit-bits
description: Security auditing and testing skills for code auditing, variant analysis, and fix verification using industry-standard tools.
---

# Security Testing and Analysis

This skill provides a suite of capabilities for auditing codebases, identifying security vulnerabilities, and verifying fixes.

## Core Capabilities

### 1. Static Analysis Integration
- **CodeQL & Semgrep**: Perform deep semantic analysis to find complex logic flaws.
- **Vulnerability Pattern Matching**: Detect common C++ pitfalls like buffer overflows, use-after-free, and integer overflows.

### 2. Code Auditing Workflow
- **Security Review**: Analyze new pull requests for FauzanEngine to ensure no security regressions.
- **Variant Analysis**: Once a bug is found, automatically scan the entire engine for similar patterns.

### 3. Verification & Fixes
- **Remediation Advice**: Provide specific C++ code fixes that follow secure coding standards.
- **Fix Verification**: Run regression tests to ensure the patch doesn't break other parts of the engine.

## Usage for FauzanEngine
Aries must invoke this skill before any major `git commit` or `make` command to ensure the integrity of the sovereign core.
