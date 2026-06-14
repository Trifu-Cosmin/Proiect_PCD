#!/usr/bin/env python3

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import xml.etree.ElementTree as ET
import html
import time

HOST = "127.0.0.1"
PORT = 8082

PROJECT_ROOT = Path(__file__).resolve().parents[1]
LOGS_DIR = PROJECT_ROOT / "logs"
REPORTS_DIR = PROJECT_ROOT / "reports"
UPLOADS_DIR = PROJECT_ROOT / "uploads"
CONFIG_DIR = PROJECT_ROOT / "config"

STATS_FILE = LOGS_DIR / "stats.txt"
JOBS_FILE = LOGS_DIR / "jobs.log"
SERVER_LOG_FILE = LOGS_DIR / "server.log"
USERS_FILE = CONFIG_DIR / "users.cfg"


def local_name(tag):
    if "}" in tag:
        return tag.split("}", 1)[1]
    return tag


def xml_escape(value):
    return html.escape(str(value), quote=True)


def soap_response(operation, inner_xml):
    body = f"""<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <{operation}Response>
{inner_xml}
    </{operation}Response>
  </soap:Body>
</soap:Envelope>
"""
    return body


def soap_fault(message):
    body = f"""<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <soap:Fault>
      <faultcode>SOAP-ERROR</faultcode>
      <faultstring>{xml_escape(message)}</faultstring>
    </soap:Fault>
  </soap:Body>
</soap:Envelope>
"""
    return body


def get_stats():
    analyzed_files = 0
    last_file = "none"

    if STATS_FILE.exists():
        lines = STATS_FILE.read_text(encoding="utf-8", errors="replace").splitlines()

        if len(lines) >= 1:
            try:
                analyzed_files = int(lines[0].strip())
            except ValueError:
                analyzed_files = 0

        if len(lines) >= 2:
            last_file = lines[1].strip()

    return analyzed_files, last_file


def list_files(directory):
    if not directory.exists():
        return []

    result = []

    for item in sorted(directory.iterdir()):
        if item.name.startswith("."):
            continue

        if item.is_file():
            result.append((item.name, item.stat().st_size))

    return result


def get_jobs():
    if not JOBS_FILE.exists():
        return []

    return JOBS_FILE.read_text(encoding="utf-8", errors="replace").splitlines()


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
            users.append((parts[0], parts[2]))

    return users


def get_logs(limit=50):
    if not SERVER_LOG_FILE.exists():
        return []

    lines = SERVER_LOG_FILE.read_text(encoding="utf-8", errors="replace").splitlines()
    return lines[-limit:]


def get_operation_from_soap(xml_body):
    try:
        root = ET.fromstring(xml_body)
    except ET.ParseError:
        return None

    body = None

    for child in root.iter():
        if local_name(child.tag) == "Body":
            body = child
            break

    if body is None:
        return None

    for child in body:
        return local_name(child.tag)

    return None


def wsdl_document():
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<definitions name="T17SoapService"
             targetNamespace="http://t17.pcd.local/soap"
             xmlns="http://schemas.xmlsoap.org/wsdl/"
             xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/"
             xmlns:tns="http://t17.pcd.local/soap">
  <documentation>
    Minimal SOAP service for the T17 Semantic Source Code Analysis project.
    Endpoint: http://{HOST}:{PORT}/soap
  </documentation>

  <service name="T17SoapService">
    <port name="T17SoapPort" binding="tns:T17SoapBinding">
      <soap:address location="http://{HOST}:{PORT}/soap"/>
    </port>
  </service>

  <binding name="T17SoapBinding" type="tns:T17SoapPortType">
    <soap:binding style="document" transport="http://schemas.xmlsoap.org/soap/http"/>
  </binding>

  <portType name="T17SoapPortType">
    <operation name="Health"/>
    <operation name="Stats"/>
    <operation name="Reports"/>
    <operation name="Uploads"/>
    <operation name="Jobs"/>
    <operation name="Users"/>
    <operation name="Logs"/>
  </portType>
</definitions>
"""


class SoapHandler(BaseHTTPRequestHandler):
    def send_xml(self, status_code, text):
        body = text.encode("utf-8")

        self.send_response(status_code)
        self.send_header("Content-Type", "text/xml; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path in ["/", "/wsdl", "/?wsdl"]:
            self.send_xml(200, wsdl_document())
            return

        self.send_xml(404, soap_fault("Endpoint not found"))

    def do_POST(self):
        if self.path != "/soap":
            self.send_xml(404, soap_fault("Endpoint not found"))
            return

        content_length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(content_length).decode("utf-8", errors="replace")

        operation = get_operation_from_soap(body)

        if operation is None:
            self.send_xml(400, soap_fault("Invalid SOAP request"))
            return

        operation_lower = operation.lower()

        if operation_lower == "health":
            inner = f"""      <status>running</status>
      <service>T17 SOAP API</service>
      <host>{HOST}</host>
      <port>{PORT}</port>
      <time>{time.strftime("%Y-%m-%d %H:%M:%S")}</time>"""
            self.send_xml(200, soap_response("Health", inner))
            return

        if operation_lower == "stats":
            analyzed_files, last_file = get_stats()
            inner = f"""      <serverStatus>running</serverStatus>
      <analyzedFiles>{analyzed_files}</analyzedFiles>
      <lastFile>{xml_escape(last_file)}</lastFile>"""
            self.send_xml(200, soap_response("Stats", inner))
            return

        if operation_lower == "reports":
            files_xml = []
            for name, size in list_files(REPORTS_DIR):
                files_xml.append(f"""        <report>
          <name>{xml_escape(name)}</name>
          <size>{size}</size>
        </report>""")

            inner = "      <reports>\n" + "\n".join(files_xml) + "\n      </reports>"
            self.send_xml(200, soap_response("Reports", inner))
            return

        if operation_lower == "uploads":
            files_xml = []
            for name, size in list_files(UPLOADS_DIR):
                files_xml.append(f"""        <upload>
          <name>{xml_escape(name)}</name>
          <size>{size}</size>
        </upload>""")

            inner = "      <uploads>\n" + "\n".join(files_xml) + "\n      </uploads>"
            self.send_xml(200, soap_response("Uploads", inner))
            return

        if operation_lower == "jobs":
            jobs_xml = []
            for job in get_jobs():
                jobs_xml.append(f"        <job>{xml_escape(job)}</job>")

            inner = "      <jobs>\n" + "\n".join(jobs_xml) + "\n      </jobs>"
            self.send_xml(200, soap_response("Jobs", inner))
            return

        if operation_lower == "users":
            users_xml = []
            for username, role in get_users():
                users_xml.append(f"""        <user>
          <username>{xml_escape(username)}</username>
          <role>{xml_escape(role)}</role>
        </user>""")

            inner = "      <users>\n" + "\n".join(users_xml) + "\n      </users>"
            self.send_xml(200, soap_response("Users", inner))
            return

        if operation_lower == "logs":
            logs_xml = []
            for line in get_logs():
                logs_xml.append(f"        <line>{xml_escape(line)}</line>")

            inner = "      <logs>\n" + "\n".join(logs_xml) + "\n      </logs>"
            self.send_xml(200, soap_response("Logs", inner))
            return

        self.send_xml(400, soap_fault(f"Unknown SOAP operation: {operation}"))

    def log_message(self, format_string, *args):
        print("[SOAP]", format_string % args)


def main():
    server = ThreadingHTTPServer((HOST, PORT), SoapHandler)

    print(f"SOAP API running on http://{HOST}:{PORT}/soap")
    print(f"WSDL available at http://{HOST}:{PORT}/wsdl")
    print("Supported SOAP operations:")
    print("  Health")
    print("  Stats")
    print("  Reports")
    print("  Uploads")
    print("  Jobs")
    print("  Users")
    print("  Logs")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nSOAP API stopped.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()