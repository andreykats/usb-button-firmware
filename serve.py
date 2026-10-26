#!/usr/bin/env python3
"""
Localhost server for the USB Button settings page (index.html) and debugger (debug.html).

Web Serial API requires a secure context (HTTPS or localhost).
This serves the pages from localhost so Chrome allows Web Serial.

Usage:
    python3 serve.py             # serves on http://127.0.0.1:8080/index.html
    python3 serve.py 9000        # custom port
    python3 serve.py --no-open   # disable Chrome auto-open
"""
import argparse
import http.server
import os
import socketserver
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request

# Serve from the directory that contains this script
os.chdir(os.path.dirname(os.path.abspath(__file__)))


def parse_args():
    parser = argparse.ArgumentParser(
        description="Serve the USB Button settings page on localhost."
    )
    parser.add_argument(
        "port",
        nargs="?",
        type=int,
        default=8080,
        help="Port to bind (default: 8080).",
    )
    parser.add_argument(
        "--no-open",
        action="store_true",
        help="Do not auto-open Google Chrome.",
    )
    return parser.parse_args()


def wait_for_web_ready(url: str, timeout: float = 10.0) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with urllib.request.urlopen(url, timeout=1.0) as resp:
                if 200 <= resp.status < 500:
                    return True
        except (urllib.error.URLError, TimeoutError, OSError, ValueError):
            pass
        time.sleep(0.25)
    return False


def launch_chrome(url: str) -> bool:
    commands: list[list[str]]
    if sys.platform == "darwin":
        commands = [["open", "-a", "Google Chrome", url]]
    elif sys.platform.startswith("linux"):
        commands = [
            ["google-chrome", url],
            ["google-chrome-stable", url],
        ]
    elif sys.platform.startswith("win"):
        commands = [["cmd", "/c", "start", "", "chrome", url]]
    else:
        commands = []

    for cmd in commands:
        try:
            subprocess.run(
                cmd,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=True,
                timeout=5,
            )
            return True
        except FileNotFoundError:
            continue
        except Exception:
            continue
    return False


def open_browser_when_ready(url: str):
    if not wait_for_web_ready(url):
        print("  Warning: server did not become ready; skipping Chrome auto-open.")
        return

    if launch_chrome(url):
        print(f"  Opened Chrome:  {url}")
    else:
        print("  Warning: could not launch Google Chrome automatically.")


class Handler(http.server.SimpleHTTPRequestHandler):
    # Ensure correct MIME types
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        '.html': 'text/html; charset=utf-8',
        '.js':   'application/javascript',
        '.css':  'text/css',
    }

    def log_message(self, fmt, *args):
        # Clean up the default log format
        print(f"  {self.address_string()} — {fmt % args}")

    def do_GET(self):
        # Browsers request favicon by default; respond with 204 to avoid 404 noise.
        if self.path == "/favicon.ico":
            self.send_response(204)
            self.end_headers()
            return
        super().do_GET()


class ReusableTCPServer(socketserver.TCPServer):
    allow_reuse_address = True

    def handle_error(self, request, client_address):
        # Ignore expected disconnects (tab close, refresh cancel) to avoid noisy tracebacks.
        _, exc, _ = sys.exc_info()
        if isinstance(exc, (BrokenPipeError, ConnectionResetError, ConnectionAbortedError)):
            return
        super().handle_error(request, client_address)


def main():
    args = parse_args()
    port = args.port
    open_url = f"http://127.0.0.1:{port}/index.html"

    print(f"\n  USB Button Settings Server")
    print(f"  ──────────────────────────")
    print(f"  Open in Chrome: {open_url}")
    if args.no_open:
        print(f"  Auto-open:      disabled (--no-open)")
    else:
        print(f"  Auto-open:      enabled")
    print(f"  Serving from:   {os.getcwd()}")
    print(f"  Press Ctrl+C to stop\n")

    with ReusableTCPServer(("", port), Handler) as httpd:
        if not args.no_open:
            threading.Thread(
                target=open_browser_when_ready,
                args=(open_url,),
                daemon=True,
                name="open-browser",
            ).start()

        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n  Server stopped.")


if __name__ == "__main__":
    try:
        main()
    except OSError as err:
        print(f"\n  Error: {err}")
        sys.exit(1)
