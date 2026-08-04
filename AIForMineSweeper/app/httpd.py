# -*- coding: utf-8 -*-
"""HTTP 服务：GET /health 与 POST /solve。"""

import json
import sys
import traceback
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from .solvers import solve_payload
from .validation import validate_payload

DRY_RUN = False


def set_dry_run(value):
    global DRY_RUN
    DRY_RUN = bool(value)


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, fmt, *args):
        sys.stderr.write("[%s] %s\n" % (self.log_date_time_string(), fmt % args))

    def _send_json(self, code, obj):
        data = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path.rstrip("/") == "/health":
            self._send_json(200, {"status": "ok", "dry_run": DRY_RUN})
        else:
            self._send_json(404, {"error": "not found"})

    def do_POST(self):
        path = self.path.rstrip("/")
        try:
            length = int(self.headers.get("Content-Length", 0))
            body = json.loads(self.rfile.read(length).decode("utf-8"))
        except Exception as e:
            self._send_json(400, {"error": "bad request: %s" % e})
            return

        if path == "/solve":
            ok, err = validate_payload(body)
            if not ok:
                self._send_json(400, {"error": err})
                return
            try:
                move = solve_payload(body, force_heuristic=DRY_RUN)
                self._send_json(200, move)
            except Exception as e:
                traceback.print_exc()
                self._send_json(500, {"error": str(e)})
        else:
            self._send_json(404, {"error": "not found"})


class Server(ThreadingHTTPServer):
    # Windows 下 allow_reuse_address 会导致两个服务同时绑定 8765 抢请求，
    # 关闭它以保证同一时刻只有一个本地服务在监听。
    allow_reuse_address = False


def run_server(host, port):
    server = Server((host, port), Handler)
    print("AIForMineSweeper server listening on http://%s:%d  (dry_run=%s)"
          % (host, port, DRY_RUN))
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
