import os
import subprocess
import sys
import signal
import requests # type: ignore

from make import make, clean


"""

Write individual functions for each example in /examples folder

"""


SRC = "src"
OUT = "./"
EXAMPLES_PATH = "examples"


HOST = "127.0.0.1"
PORT = 8080
URL  = f"http://{HOST}:{PORT}"


def failed(name, process):
    
    print(f"🔴 Test {name} failed!")
    
    # kill example process
    os.kill(process.pid, signal.SIGINT)
        
    # clean example
    clean(name, "./")
    
    
def test(name, process, url, method, exp_status, exp_body):
    
    res = requests.request(method, url)
    
    if (res.status_code != exp_status): failed(name, process)
    if (res.text != exp_body): failed(name, process)
    

def hello():
    
    """
    Source: examples/hello.c
    
    Test              Exp. Stat.Co. Body
    -------------------------------------------
    1) GET  /              200      Hello, World!
    2) POST /              405      405 Method Not Allowed
    3) GET  /admin         404      404 Not Found
    4) POST /admin         404      404 Not Found
    """
    
    path = EXAMPLES_PATH+"/hello.c"
    
    # compile example
    exec_path = make(path, SRC, OUT, 0)
    
    # run example
    process = subprocess.Popen([exec_path], stdout=subprocess.DEVNULL)
    
    # 1)
    test(path, process, URL+"/", "GET", 200, "Hello, World!")
    
    # 2)
    test(path, process, URL+"/", "POST", 405, "405 Method Not Allowed")
    
    # 3)
    test(path, process, URL+"/admin", "GET", 404, "404 Not Found")
    
    # 4)
    test(path, process, URL+"/admin", "POST", 404, "404 Not Found")
    
    print(f"🟢 Test {path} succeed!")
    
    # kill example process
    os.kill(process.pid, signal.SIGINT)
        
    # clean example
    clean(path, OUT, 0)
    

def json():
    
    """
    Source: examples/json.c
    
    Test      Exp. Stat.Co. Body            Header
    ----------------------------------------------
    1) GET  /      200      {\"message\": \\ Content-Type: application/json
        \"Hello, this a json message!\"} 
    """
    
    path = EXAMPLES_PATH+"/json.c"
    
    # compile example
    exec_path = make(path, SRC, OUT, 0)
    
    # run example
    process = subprocess.Popen([exec_path], stdout=subprocess.DEVNULL)
    
    # 1)
    res = requests.get(URL+"/")
    if (res.status_code != 200): failed(path, process)
    if (res.headers["Content-Type"] != "application/json"): failed(path, process)
    if (res.text != "{\"message\": \"Hello, this a json message!\"}"): failed(path, process)
    
    print(f"🟢 Test {path} succeed!")
    
    # kill example process
    os.kill(process.pid, signal.SIGINT)
        
    # clean example
    clean(path, OUT, 0)


if __name__ == "__main__":
    
    if (len(sys.argv) > 1):
        
        match sys.argv[1]:
            case "hello":
                hello()
            case "json":
                json()
    
    else:
        hello()
        json()
