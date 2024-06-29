import os
import sys
import glob


"""

WHO NEEDS MAKEFILE OR SOMETHING WHEN WE HAVE PYTHON ???


Usage: python make.py [op] [program]

[op]:
    make: compile program
    clean: clean .o and exec files
    
[program]: filename to compile "main.c" by default if not given

"""


CC = "gcc"
FLAGS = ""

SRC = "src"
OUT = "./"


def make(program_path, src_folder, out_folder, log=1):
    
    """
    Compile given (program_path) .c file with sources in (src_folder) folder
    Return compiled executable path
    """
    
    abs_program_path = os.path.abspath(program_path)
    abs_src_folder   = os.path.abspath(src_folder)
    abs_out_folder   = os.path.abspath(out_folder)
    
    program_name = abs_program_path.rsplit(".")[0].split("/")[-1]
    
    # LET HIM COOK!!!
    if (log): print(f"Cooking: {program_name}")
    
    sources = glob.glob(f"{abs_src_folder}/*.c")
    
    for s in sources:
        # compile .o files
        s_name = s.rsplit(".")[0].split("/")[-1]
        os.system(f"{CC} {FLAGS} -c {s} -o {abs_out_folder}/{s_name}.o")
    
    objects = glob.glob(f"{abs_out_folder}/*.o")
    
    # compile
    os.system(f"{CC} {FLAGS} -o {abs_out_folder}/{program_name} {abs_program_path} {' '.join(objects)}")
    
    return f"{abs_out_folder}/{program_name}"
    

def clean(program_path, out_folder, log=1):
    
    """
    Clean .o files and executable in (out_folder) folder
    specified with (program_path) .c file generated with make()
    """
    
    abs_program_path = os.path.abspath(program_path)
    abs_out_folder   = os.path.abspath(out_folder)
    
    program_name = abs_program_path.rsplit(".")[0].split("/")[-1]
    
    if log: print(f"Cleaning: {program_name}")    
    
    # remove objects
    objects = glob.glob(f"{abs_out_folder}/*.o")
    
    for o in objects:
        os.system(f"rm {o}")
        
    os.system(f"rm {abs_out_folder}/{program_name}")


if __name__ == "__main__":
    
    op = sys.argv[1]
    program = "main.c"
    if (len(sys.argv) > 2):
        program = sys.argv[2]
    
    if op == "make":
        make(program, SRC, OUT)
    
    elif op == "clean":
        clean(program, OUT)
