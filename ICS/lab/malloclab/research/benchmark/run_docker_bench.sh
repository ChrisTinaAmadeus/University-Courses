#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RESULTS_DIR="$ROOT_DIR/benchmark/results"
mkdir -p "$RESULTS_DIR"

pick_runtime() {
  if command -v docker >/dev/null 2>&1 && docker version >/dev/null 2>&1; then echo docker; return 0; fi
  if command -v podman >/dev/null 2>&1 && podman version >/dev/null 2>&1; then echo podman; return 0; fi
  return 1
}

if ! RUNTIME="$(pick_runtime)"; then
  cat <<'EOF' >&2
ERROR: Neither Docker nor Podman is usable.
If you are on WSL2 + Docker Desktop:
  1) Docker Desktop -> Settings -> Resources -> WSL integration
  2) Enable integration for your distro
  3) Reopen terminal and verify: docker version
EOF
  exit 1
fi

echo "[info] Using container runtime: $RUNTIME" >&2

IMAGES=("ubuntu:16.04" "ubuntu:18.04" "ubuntu:20.04")
if [[ $# -gt 0 ]]; then IMAGES=("$@"); fi

# Benchmark parameters
SMALL_ITERS="${SMALL_ITERS:-2000000}"
SMALL_SIZE="${SMALL_SIZE:-64}"
BURST_ROUNDS="${BURST_ROUNDS:-20000}"
BURST_COUNT="${BURST_COUNT:-1024}"
LARGE_ITERS="${LARGE_ITERS:-20}"
LARGE_SIZE="${LARGE_SIZE:-64M}"
THREAD_SPAWN_COUNT="${THREAD_SPAWN_COUNT:-2000}"
THREAD_SPAWN_ITERS="${THREAD_SPAWN_ITERS:-1000}"

sanitize_image_name() { echo "$1" | tr ':/' '__'; }

run_one_image() {
  local image="$1" out_file="$2"
  echo "[info] Running $image -> $out_file" >&2

  "$RUNTIME" run --rm -i \
    -e IMAGE="$image" \
    -e SMALL_ITERS="$SMALL_ITERS" \
    -e SMALL_SIZE="$SMALL_SIZE" \
    -e BURST_ROUNDS="$BURST_ROUNDS" \
    -e BURST_COUNT="$BURST_COUNT" \
    -e LARGE_ITERS="$LARGE_ITERS" \
    -e LARGE_SIZE="$LARGE_SIZE" \
    -e THREAD_SPAWN_COUNT="$THREAD_SPAWN_COUNT" \
    -e THREAD_SPAWN_ITERS="$THREAD_SPAWN_ITERS" \
    -v "$ROOT_DIR:/work:ro" \
    "$image" bash -s >"$out_file" <<'INNER_EOF'
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

fix_old_sources() {
  if [[ -f /etc/apt/sources.list ]]; then
    sed -i \
      -e 's|http://archive.ubuntu.com/ubuntu|http://old-releases.ubuntu.com/ubuntu|g' \
      -e 's|http://security.ubuntu.com/ubuntu|http://old-releases.ubuntu.com/ubuntu|g' \
      -e 's|https://archive.ubuntu.com/ubuntu|http://old-releases.ubuntu.com/ubuntu|g' \
      -e 's|https://security.ubuntu.com/ubuntu|http://old-releases.ubuntu.com/ubuntu|g' \
      /etc/apt/sources.list || true
  fi
}

if ! apt-get update >/dev/null 2>&1; then
  fix_old_sources
  apt-get -o Acquire::Check-Valid-Until=false update
fi
apt-get install -y --no-install-recommends build-essential ca-certificates >/dev/null

echo "image=${IMAGE:-unknown}"
if [[ -f /etc/os-release ]]; then
  echo "os=$(source /etc/os-release && echo "${PRETTY_NAME}")"
fi
echo "kernel=$(uname -r)"
glibc_ver="$(getconf GNU_LIBC_VERSION | awk '{print $2}')"
echo "glibc=${glibc_ver}"
echo "gcc=$(gcc --version | head -n1)"

glibc_major="${glibc_ver%%.*}"
glibc_minor="${glibc_ver#*.}"
glibc_minor="${glibc_minor%%.*}"

supports_tcache=0
if [[ "$glibc_major" -gt 2 || ( "$glibc_major" -eq 2 && "$glibc_minor" -ge 26 ) ]]; then
  supports_tcache=1
fi

gcc -O2 -pthread /work/benchmark/bench_malloc.c -o /tmp/bench

# === baseline ===
echo "variant=baseline"

# small: t=1 and t=8
for thr in 1 8; do
  /tmp/bench --mode small --threads "$thr" --iters "${SMALL_ITERS}" --size "${SMALL_SIZE}"
done

# sweep: size-latency curve (t=1)
/tmp/bench --mode sweep --threads 1 --iters "${SMALL_ITERS}"

# burst: t=1 and t=8
for thr in 1 8; do
  /tmp/bench --mode burst --threads "$thr" --iters "${BURST_ROUNDS}" --size "${SMALL_SIZE}" --burst "${BURST_COUNT}"
done

# mixed: random interleaved sizes (t=1 and t=8)
for thr in 1 8; do
  /tmp/bench --mode mixed --threads "$thr" --iters "${SMALL_ITERS}"
done

# thread_spawn: short-lived threads stress init path
/tmp/bench --mode thread_spawn --threads 1

# large_touch: memory bandwidth / page behaviour (control)
/tmp/bench --mode large_touch --threads 1 --iters "${LARGE_ITERS}" --size "${LARGE_SIZE}"

# === tcache_off ===
echo "variant=tcache_off"
if [[ "$supports_tcache" == "1" ]]; then
  TCOFF="GLIBC_TUNABLES=glibc.malloc.tcache_count=0"
  for thr in 1 8; do
    env $TCOFF /tmp/bench --mode small --threads "$thr" --iters "${SMALL_ITERS}" --size "${SMALL_SIZE}"
  done
  env $TCOFF /tmp/bench --mode sweep --threads 1 --iters "${SMALL_ITERS}"
  for thr in 1 8; do
    env $TCOFF /tmp/bench --mode burst --threads "$thr" --iters "${BURST_ROUNDS}" --size "${SMALL_SIZE}" --burst "${BURST_COUNT}"
  done
  for thr in 1 8; do
    env $TCOFF /tmp/bench --mode mixed --threads "$thr" --iters "${SMALL_ITERS}"
  done
  env $TCOFF /tmp/bench --mode thread_spawn --threads 1
else
  echo "skip=tcache_off (glibc<2.26, no tcache)"
fi
INNER_EOF
}

for img in "${IMAGES[@]}"; do
  out="$RESULTS_DIR/$(sanitize_image_name "$img").txt"
  run_one_image "$img" "$out"
done

echo "[done] Results written to: $RESULTS_DIR" >&2
