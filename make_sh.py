#!/usr/bin/env python3
"""
Wrap ARM64 ELF into a self-extracting shell script.
Usage: python3 make_sh.py <input_elf> <output.sh>

The generated .sh:
  1. Checks /dev/BY* driver exists
  2. Reads key.txt
  3. Extracts the embedded ELF to /data/local/tmp/.<random>
  4. chmod 755 + exec
"""
import sys, os, base64, textwrap

def main():
    if len(sys.argv) != 3:
        print("Usage: make_sh.py <elf> <out.sh>")
        sys.exit(1)

    elf_path = sys.argv[1]
    out_path = sys.argv[2]

    with open(elf_path, 'rb') as f:
        data = f.read()

    # Verify it's an ELF
    if data[:4] != b'\x7fELF':
        print("WARNING: Input is not an ELF file")

    b64 = base64.b64encode(data).decode()
    # Split into 76-char lines for readability
    b64_lines = '\n'.join(textwrap.wrap(b64, 76))

    shell = f'''\
#!/system/bin/sh
#BY x Kernel x FF
# Auto-generated wrapper — do not edit

# Check driver
DRV=$(ls /dev/BY* 2>/dev/null | head -1)
if [ -z "$DRV" ]; then
  echo "[KERNEL] Driver not loaded. Run 5.10.sh first."
  exit 1
fi

# Extract binary
TMP=$(mktemp /data/local/tmp/.XXXXXX 2>/dev/null || echo /data/local/tmp/.ff$$)
sed -n '/^#PAYLOAD_START$/,$p' "$0" | tail -n +2 | base64 -d 2>/dev/null > "$TMP"
if [ $? -ne 0 ] || [ ! -s "$TMP" ]; then
  echo "[KERNEL] Extraction failed."
  rm -f "$TMP"
  exit 1
fi
chmod 755 "$TMP"
exec "$TMP" "$@"
exit 0
#PAYLOAD_START
{b64_lines}
'''

    with open(out_path, 'w', newline='\n') as f:
        f.write(shell)

    os.chmod(out_path, 0o755)
    print(f"[+] Packaged: {out_path} ({len(data)//1024}KB ELF, {len(shell)//1024}KB total)")

if __name__ == '__main__':
    main()
