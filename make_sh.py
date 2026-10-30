#!/usr/bin/env python3
"""
Wrap ARM64 ELF into a self-extracting shell script.
Usage:
  python3 make_sh.py <input_elf> <output.sh>          # kernel build (checks /dev/BY* driver)
  python3 make_sh.py --ext <input_elf> <output.sh>    # external build (no driver check)
"""
import sys, os, base64, textwrap

def main():
    args = sys.argv[1:]
    ext_mode = False
    if args and args[0] == '--ext':
        ext_mode = True
        args = args[1:]

    if len(args) != 2:
        print("Usage: make_sh.py [--ext] <elf> <out.sh>")
        sys.exit(1)

    elf_path, out_path = args

    with open(elf_path, 'rb') as f:
        data = f.read()

    if data[:4] != b'\x7fELF':
        print("WARNING: Input is not an ELF file")

    b64 = base64.b64encode(data).decode()
    b64_lines = '\n'.join(textwrap.wrap(b64, 76))

    driver_check = '' if ext_mode else '''\
# Check driver
DRV=$(ls /dev/BY* 2>/dev/null | head -1)
if [ -z "$DRV" ]; then
  echo "[KERNEL] Driver not loaded. Run 5.10.sh first."
  exit 1
fi

'''

    tag = 'EXT' if ext_mode else 'KERNEL'

    shell = f'''\
#!/system/bin/sh
#BY x Kernel x FF
# Auto-generated wrapper — do not edit

{driver_check}# Extract binary
TMP=$(mktemp /data/local/tmp/.XXXXXX 2>/dev/null || echo /data/local/tmp/.ff$$)
sed -n '/^#PAYLOAD_START$/,$p' "$0" | tail -n +2 | base64 -d 2>/dev/null > "$TMP"
if [ $? -ne 0 ] || [ ! -s "$TMP" ]; then
  echo "[{tag}] Extraction failed."
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
    print(f"[+] Packaged ({tag}): {out_path} ({len(data)//1024}KB ELF, {len(shell)//1024}KB total)")

if __name__ == '__main__':
    main()
