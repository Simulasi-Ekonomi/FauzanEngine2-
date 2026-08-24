#!/bin/bash
PROJECT_ROOT=$(pwd) if "os" in locals() else "."
export PYTHONPATH="$PROJECT_ROOT/backend"

echo "--- ARIES S7 PRO (S6 INTEGRATED) TEST ---"
python3 -c "
import asyncio
from aries.autonomous_brain import AriesAutonomousSystem

async def main():
    aries = AriesAutonomousSystem()
    # Tes Belajar
    res = await aries.s7_reasoning_cycle('https://ue5-docs.com', 'Memory_Management')
    print(f'\n[RESULT]: {res}')
    
    # Tes Audit (Implementasi Standar S7 ke Kode Lokal)
    sample_code = 'std::vector<int> myData; myData.push_back(10);'
    audit = aries.audit_engine_source(sample_code)
    print(f'[AUDIT]: {audit}')

if __name__ == '__main__':
    asyncio.run(main())
"
