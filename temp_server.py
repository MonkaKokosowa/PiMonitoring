from http.server import BaseHTTPRequestHandler, HTTPServer

from RaspberryPiVcgencmd import Vcgencmd

# Initialize the Vcgencmd object
vcgencmd = Vcgencmd()


class TempHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        # Fetch the temperature
        # Note: Depending on your version, you may need to specify 'c' or 'f'
        temp = vcgencmd.get_cpu_temp()

        # Send response headers
        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()

        # Send the temperature data
        response_text = f"{temp}"
        self.wfile.write(response_text.encode("utf-8"))


def run(server_class=HTTPServer, handler_class=TempHandler, port=8080):
    server_address = ("", port)
    httpd = server_class(server_address, handler_class)
    print(f"Starting server on port {port}...")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server.")
        httpd.server_close()


if __name__ == "__main__":
    run()
