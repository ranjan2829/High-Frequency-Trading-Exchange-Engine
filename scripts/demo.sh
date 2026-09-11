#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${BUILD_DIR:-/tmp/hft-demo-build}"
mkdir -p "$BUILD"
cd "$BUILD"

if command -v cmake >/dev/null 2>&1; then
  cmake "$ROOT" -DCMAKE_BUILD_TYPE=Release
  cmake --build . -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)"
else
  echo "cmake not found; compile with clang++ via README manual steps" >&2
  exit 1
fi

echo "Starting exchange..."
./exchange_main &
EX_PID=$!
sleep 1
echo "Starting maker client..."
./trading_main 1 MAKER 10 0.5 100 1000 -10000 &
CL_PID=$!
sleep 5
kill "$CL_PID" "$EX_PID" 2>/dev/null || true
wait 2>/dev/null || true
echo "Demo finished."
