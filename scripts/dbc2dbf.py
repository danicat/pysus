# datasus
import sys

from datasus import decompress

def usage():
    print("dbc2dbf.py: convert datasus *.dbc file to *.dbf")
    print("usage:")
    print("dbc2dbf.py file.dbc file.dbf")

def main() -> int:
    """decompress a *.dbc file into a *.dbf file"""
    if len(sys.argv) < 3:
        usage()
        return -1
    
    return decompress(sys.argv[1], sys.argv[2])

if __name__ == '__main__':
    sys.exit(main())