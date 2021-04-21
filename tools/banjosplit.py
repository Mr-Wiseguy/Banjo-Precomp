#!/usr/bin/env python3
# Script to split the output of objcopy into separate code and data files for rzip compression
import subprocess
import argparse
import sys

def get_addr(nm_line):
    addr_str = nm_line.split()[0]
    return int(addr_str, 16)

def main(binfile, elfpath, nm, code_out, data_out):

    # Get nm output
    nm_output = subprocess.run([nm, elfpath], stdout=subprocess.PIPE)

    # Check if nm ran successfully
    if nm_output.returncode != 0:
        print(f'Failed to run {nm} on {elfpath}')
        sys.exit(1)

    lines = nm_output.stdout.splitlines()

    data_start = -1

    for curline in lines:
        curline_str = str(curline)
        if 'data_ROM_START' in curline_str:
            data_start = get_addr(curline)
    
    if data_start == -1:
        printf(f'Elf file {elfpath} does not have a data_ROM_START symbol!')
        sys.exit(1)

    code = binfile.read(data_start)
    data = binfile.read()

    with open(code_out, "wb") as f:
        f.write(code)

    with open(data_out, "wb") as f:
        f.write(data)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Get objects for splat file',
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('bin', type=argparse.FileType('rb'), help='Binary output of objcopy')
    parser.add_argument('elf', type=str, help='Elf with symbols (for determining start/end of code/data')
    parser.add_argument('code_out', type=str, help='Output path for the bin file containing code')
    parser.add_argument('data_out', type=str, help='Output path for the bin file containing data')
    parser.add_argument('--nm', help="Splat files", default='mips-linux-gnu-nm')
    args = parser.parse_args()

    main(args.bin, args.elf, args.nm, args.code_out, args.data_out)
