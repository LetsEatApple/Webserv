_This project has been created as part of the 42 curriculum by lhagemos, mkugan, pschneid._

## Description

## Instructions

### Build with Makefile
```bash
make # or
make all # or
make webserv
```

### Run
```bash
./webserv [configuration_file.conf]
```

### Clean
```bash
make clean # Objects and dependencies
make fclean # Objects, dependencies, and executable
make re # Delete everything and rebuild
```

### Test
```bash
# curl
curl -v http://localhost:8000/ # GET request
curl -v http://localhost:8000/file.html # GET with specific file
curl -v -X POST http://localhost:8000/ # POST request
curl -v -X DELETE http://localhost:8000/resource # DELETE request

curl -v http://localhost:8080/ \
  -H "Host: localhost"  # test header
  
# siege
siege -b http://localhost:8000/ # runs the test with NO DELAY for throughput benchmarking.
siege -c 50 -t 30s http://localhost:8000/ # 50 concurrent users for 30 sec
```
## Resources
- [RFC 9110 - HTTP Semantics](https://datatracker.ietf.org/doc/html/rfc9110)
- [RFC 9112 - HTTP/1.1](https://datatracker.ietf.org/doc/html/rfc9112)
- [RFC 6265 - HTTP State Management Mechanism](https://datatracker.ietf.org/doc/html/rfc6265)
- [RFC 3875 - The Common Gateway Interface (CGI) Version 1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- [RFC 7578 - Returning Values from Forms: multipart/form-data](https://datatracker.ietf.org/doc/html/rfc7578)
- [nginx documentation](https://nginx.org/en/docs/)