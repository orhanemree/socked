# 🧦 Socked
A minimalistic HTTP library written in pure C.

## `Hello, World!`
* Simple `Hello, World!` example in Socked. See [`/examples`](https://github.com/orhanemree/socked/tree/master/examples) for more example.

```c
// app.c

#include "include/socked.h"

void handler(Sc_Request *req, Sc_Response *res) {
    sc_set_body(res, "Hello, World!");
}

int main() {
    Sc_Server *server = sc_server();

    sc_get(server, "/", handler);

    sc_listen(server, "127.0.0.1", 8080);
}
```

## Quick Start
* Coming Soon.

## Features
* Coming Soon.

## API Reference
* Coming Soon.

## Tests
* You need Python and `requests` module to run tests. Examples (`/examples`) also used as test.
* Run all tests.
```
$ python test.py
```
* Run a single test. Test name is extensionless file name in `/examples` folder eg. hello, json.
```
$ python test.py <test_name>
```


## References
* [Listen to many sockets with select in C [Video]](https://www.youtube.com/watch?v=Y6pFtgRdUts)


## License
* MIT License.