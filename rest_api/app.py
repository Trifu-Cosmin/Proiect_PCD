#!/usr/bin/env python3

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, unquote
from pathlib import Path
import json
import os
import time

HOST = "127.0.0.1"
PORT = 8081

PROJECT_ROOT = Path(__file__).resolve().parents[1]
LOGS_DIR = PROJECT_ROOT / "logs"
REPORTS_DIR = PROJECT_ROOT / "reports"
UPLOADS_DIR = PROJECT_ROOT / "uploads"
CONFIG_DIR = PROJECT_ROOT / "config"

STATS_FILE = LOGS_DIR / "stats.txt"
JOBS_FILE = LOGS_DIR / "jobs.log"
SERVER_LOG_FILE = LOGS_DIR / "server.log"
USERS_FILE = CONFIG_DIR / "users.cfg"


def json_response(handler, status_code, data):
    body = json.dumps(data, indent=2).encode("utf-8")

    handler.send_response(status_code)
    handler.send_header("Content-Type", "application/json; charset=utf-8")
    handler.send_header("Content-Length", str(len(body)))
    handler.send_header("Access-Control-Allow-Origin", "*")
    handler.send_header("Access-Control-Allow-Methods", "GET, OPTIONS")
    handler.send_header("Access-Control-Allow-Headers", "Content-Type")
    handler.end_headers()
    handler.wfile.write(body)


def text_response(handler, status_code, text, content_type="text/plain; charset=utf-8"):
    body = text.encode("utf-8")

    handler.send_response(status_code)
    handler.send_header("Content-Type", content_type)
    handler.send_header("Content-Length", str(len(body)))
    handler.send_header("Access-Control-Allow-Origin", "*")
    handler.end_headers()
    handler.wfile.write(body)


def list_directory(path):
    if not path.exists():
        return []

    result = []

    for item in sorted(path.iterdir()):
        if item.name.startswith("."):
            continue

        if item.is_file():
            result.append({
                "name": item.name,
                "size": item.stat().st_size
            })

    return result


def read_text_file(path):
    if not path.exists():
        return ""

    return path.read_text(encoding="utf-8", errors="replace")


def read_last_lines(path, limit=200):
    if not path.exists():
        return []

    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    return lines[-limit:]


def safe_path(base_dir, name):
    safe_name = os.path.basename(unquote(name))
    path = (base_dir / safe_name).resolve()

    if not str(path).startswith(str(base_dir.resolve())):
        return None

    return path


def get_stats():
    if not STATS_FILE.exists():
        return {
            "server_status": "running",
            "analyzed_files": 0,
            "last_file": "none"
        }

    lines = STATS_FILE.read_text(encoding="utf-8", errors="replace").splitlines()

    analyzed_files = 0
    last_file = "none"

    if len(lines) >= 1:
        try:
            analyzed_files = int(lines[0].strip())
        except ValueError:
            analyzed_files = 0

    if len(lines) >= 2:
        last_file = lines[1].strip()

    return {
        "server_status": "running",
        "analyzed_files": analyzed_files,
        "last_file": last_file
    }


def get_users():
    if not USERS_FILE.exists():
        return []

    users = []

    for line in USERS_FILE.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()

        if not line or line.startswith("#"):
            continue

        parts = line.split(":")

        if len(parts) == 3:
            users.append({
                "username": parts[0],
                "role": parts[2]
            })

    return users


def get_jobs():
    if not JOBS_FILE.exists():
        return []

    jobs = []

    for line in JOBS_FILE.read_text(encoding="utf-8", errors="replace").splitlines():
        jobs.append(line)

    return jobs


def ui_page():
    return """<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>T17 REST API</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 30px; background: #f5f5f5; }
    h1 { color: #222; }
    button { margin: 5px; padding: 8px 12px; cursor: pointer; }
    pre { background: #111; color: #eee; padding: 15px; border-radius: 6px; white-space: pre-wrap; }
  </style>
</head>
<body>
  <h1>T17 - REST API </h1>
  <p>Interfata simpla pentru verificarea resurselor expuse de proiect.</p>

  <button onclick="load('/health')">Health</button>
  <button onclick="load('/stats')">Stats</button>
  <button onclick="load('/reports')">Reports</button>
  <button onclick="load('/uploads')">Uploads</button>
  <button onclick="load('/jobs')">Jobs</button>
  <button onclick="load('/users')">Users</button>
  <button onclick="load('/logs')">Logs</button>

  <pre id="output">Alege o optiune...</pre>

  <script>
    async function load(path) {
      const response = await fetch(path);
      const data = await response.json();
      document.getElementById("output").textContent = JSON.stringify(data, null, 2);
    }
  </script>
</body>
</html>
"""


class RestHandler(BaseHTTPRequestHandler):
    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path == "/":
            json_response(self, 200, {
                "project": "T17 - Analiza Semantica Cod Sursa",
                "service": "Minimal REST API",
                "endpoints": [
                    "/health",
                    "/stats",
                    "/reports",
                    "/reports/<name>",
                    "/uploads",
                    "/jobs",
                    "/users",
                    "/logs",
                    "/ui"
                ]
            })
            return

        if path == "/ui":
            text_response(self, 200, ui_page(), "text/html; charset=utf-8")
            return

        if path == "/health":
            json_response(self, 200, {
                "status": "running",
                "service": "T17 REST API",
                "host": HOST,
                "port": PORT,
                "time": time.strftime("%Y-%m-%d %H:%M:%S")
            })
            return

        if path == "/stats":
            json_response(self, 200, get_stats())
            return

        if path == "/reports":
            json_response(self, 200, {
                "directory": "reports",
                "files": list_directory(REPORTS_DIR)
            })
            return

        if path.startswith("/reports/"):
            name = path.removeprefix("/reports/")
            report_path = safe_path(REPORTS_DIR, name)

            if report_path is None or not report_path.exists() or not report_path.is_file():
                json_response(self, 404, {
                    "error": "report not found"
                })
                return

            json_response(self, 200, {
                "filename": report_path.name,
                "content": read_text_file(report_path)
            })
            return

        if path == "/uploads":
            json_response(self, 200, {
                "directory": "uploads",
                "files": list_directory(UPLOADS_DIR)
            })
            return

        if path == "/jobs":
            json_response(self, 200, {
                "file": "logs/jobs.log",
                "jobs": get_jobs()
            })
            return

        if path == "/users":
            json_response(self, 200, {
                "file": "config/users.cfg",
                "users": get_users()
            })
            return

        if path == "/logs":
            json_response(self, 200, {
                "file": "logs/server.log",
                "lines": read_last_lines(SERVER_LOG_FILE, 200)
            })
            return

        json_response(self, 404, {
            "error": "endpoint not found"
        })

    def log_message(self, format_string, *args):
        print("[REST]", format_string % args)


def main():
    server = ThreadingHTTPServer((HOST, PORT), RestHandler)

    print(f"REST API running on http://{HOST}:{PORT}")
    print(f"Web UI running on http://{HOST}:{PORT}/ui")
    print("Available endpoints:")
    print("  GET /health")
    print("  GET /stats")
    print("  GET /reports")
    print("  GET /reports/<name>")
    print("  GET /uploads")
    print("  GET /jobs")
    print("  GET /users")
    print("  GET /logs")
    print("  GET /ui")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nREST API stopped.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()