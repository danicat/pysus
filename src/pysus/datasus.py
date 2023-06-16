from ctypes import *

c_lib = CDLL("./dbc2dbf/dbc2dbf.so")

def decompress(input_file: str, output_file: str):
    input = input_file.encode()
    output = output_file.encode()
    
    res = c_lib.dbc2dbf(input, output)
    if res != 0:
        raise Exception("error decompressing file")
    
