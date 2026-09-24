#!/bin/bash
set -euo pipefail
candidate="$(cd "$(dirname "$0")" && pwd)"
project="$(dirname "$candidate")"
cd "$project"
export CXX=clang++
opponent="Version07_frozen"
maps=("$@")
if [ ${#maps[@]} -eq 0 ]; then maps=(default trophy); fi
for map in "${maps[@]}"; do
  [[ "$map" =~ ^[a-zA-Z0-9_-]+$ ]] && [ -f "maps/$map.map" ] || { echo "Invalid map: $map"; exit 1; }
done
mkdir -p "$project/replays"
output="$(mktemp -d "$project/replays/Checkpoint9-$(date +%Y%m%d-%H%M%S)-XXXXXX")"
printf 'candidate=%s\nopponent=%s\n' "$candidate" "$opponent" > "$output/config.txt"
printf 'map=%s\n' "${maps[@]}" >> "$output/config.txt"
cp "$candidate/CHECKPOINT9_MANIFEST.json" "$output/"
echo "Output: $output"
for map in "${maps[@]}"; do
  echo "Running $map: Expansion+AntiOscillation A vs V07 B"
  unswbc run "maps/$map.map" "$candidate" "$opponent" -o "$output/$map-candidate-A.replay" > "$output/$map-candidate-A.log" 2>&1
  echo "Running $map: V07 A vs Expansion+AntiOscillation B"
  unswbc run "maps/$map.map" "$opponent" "$candidate" -o "$output/$map-candidate-B.replay" > "$output/$map-candidate-B.log" 2>&1
done
python3 "$candidate/summarize_checkpoint9.py" "$output"
