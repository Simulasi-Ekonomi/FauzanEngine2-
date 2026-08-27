from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from main import app  # noqa: E402

paths = {route.path for route in app.routes if hasattr(route, "path")}
required = {"/", "/health"}
missing = required - paths
if missing:
    raise SystemExit(f"missing required routes: {sorted(missing)}")
print(f"BACKEND_IMPORT_SMOKE_OK routes={len(paths)} health=1 root=1")
