# Web Server

This is a multithreaded web server that listens on a specified port and handles various endpoints. The server supports the following endpoints:

- `/static`: Serves static files from the `/static` directory.
- `/stats`: Returns an HTML document with server statistics.
- `/calc`: Returns the sum of two numbers provided in the URL path.

## Usage

- `/static`: server_ip:port/static/images/jolie.jpg
- `/stats`: server_ip:port/stats
- `/calc`: server_ip:port/calc/'a'/'b' - where a and b are integers of your choice

### Starting the Server

To start the server, compile the code and run the executable. You can specify the port using the `-p` option. If no port is specified, the server will default to port 80.

```sh
gcc -o webserver webserver.c
./webserver -p <port>
