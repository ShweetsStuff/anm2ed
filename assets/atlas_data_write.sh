#!/usr/bin/env bash
set -euo pipefail

INPUT="atlas.png"
OUTPUT="../src/PACKED.h"
TMP="atlas.bytes"

# Ensure deps
command -v optipng >/dev/null 2>&1 || { echo "error: optipng not found in PATH" >&2; exit 1; }
command -v xxd     >/dev/null 2>&1 || { echo "error: xxd not found in PATH" >&2; exit 1; }

# 1) Optimize PNG in place
optipng -o7 "$INPUT" >/dev/null

# 2) Extract ONLY the bytes from xxd -i (between '= {' and '};')
#    Using awk avoids the earlier '{' vs '= {' mismatch bug.
xxd -i "$INPUT" \
  | awk '
      /= *\{/ {inside=1; next}
      inside && /};/ {inside=0; exit}
      inside {print}
    ' > "$TMP"

# Sanity check: make sure we got something
if ! [ -s "$TMP" ]; then
  echo "error: failed to extract bytes from xxd output" >&2
  exit 1
fi

# 3) Replace ONLY the bytes inside TEXTURE_ATLAS[] initializer
#    - Find the exact declaration line for TEXTURE_ATLAS
#    - Print that line
#    - On the following line with just '{', print it, then insert bytes,
#      then skip everything until the matching '};' and print that once.
awk -v tmpfile="$TMP" '
  BEGIN { state=0 }  # 0=normal, 1=after decl waiting for {, 2=skipping old bytes until };

  # Match the TEXTURE_ATLAS declaration line precisely
  $0 ~ /^[[:space:]]*const[[:space:]]+u8[[:space:]]+TEXTURE_ATLAS\[\][[:space:]]*=/ {
    print; state=1; next
  }

  # After the decl, the next line with a lone "{" starts the initializer
  state==1 && $0 ~ /^[[:space:]]*{[[:space:]]*$/ {
    print               # print the opening brace line
    while ((getline line < tmpfile) > 0) print line  # insert fresh bytes
    close(tmpfile)
    state=2             # now skip old initializer content until we hit the closing "};"
    next
  }

  # While skipping, suppress lines until the closing "};", which we reprint once
  state==2 {
    if ($0 ~ /^[[:space:]]*};[[:space:]]*$/) {
      print             # print the closing brace+semicolon
      state=0
    }
    next
  }

  # Default: pass through unchanged
  { print }
' "$OUTPUT" > "$OUTPUT.tmp" && mv "$OUTPUT.tmp" "$OUTPUT"

rm -f "$TMP"
echo "Updated $OUTPUT with optimized bytes from $INPUT."
