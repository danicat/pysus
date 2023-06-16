# __main__.py
import sys
from pysus import datasus

def main():
    if len(sys.argv) != 3:
        print("this program requires an input file and an output file")
        return -1

    return datasus.decompress(sys.argv[1], sys.argv[2])

if __name__ == "__main__":
    main()