import os
import sys
import glob


"""

WHO NEEDS MAKEFILE OR SOMETHING WHEN WE HAVE PYTHON ???


Usage: python make.py [op] [program]

[op]:
    make: compile program
    clean: clean .o and exec files
    
[program]: extensionless filename to compile "main" by default if not given

"""


CC = "gcc"
FLAGS = ""

INCLUDE = "include"
SRC     = "src"
OUT     = "out" # TODO: compile into OUT 

disable_err = 0

def make(program):
    
    print("Make program:", program)
    
    sources = glob.glob(f"./{SRC}/*.c")
    
    for f in sources:
        # compile .o files
        os.system(f"{CC} {FLAGS} -c {f} {'> /dev/null 2>&1' if disable_err else ''}")
    
    objects = glob.glob(f"./*.o")
    
    # compile
    os.system(f"{CC} {FLAGS} -o {program} {program}.c {' '.join(objects)}\
{'> /dev/null 2>&1' if disable_err else ''}")
    

def clean(program):
    
    # remove objects
    objects = glob.glob(f"./*.o")
    
    for f in objects:
        os.system(f"rm {f}")
        
    os.system(f"rm {program}")


if __name__ == "__main__":
    
    op = sys.argv[1]
    program = "main"
    if (len(sys.argv) > 2):
        program = sys.argv[2]
    
    if op == "make":
        make(program)
    
    elif op == "clean":
        clean(program)
