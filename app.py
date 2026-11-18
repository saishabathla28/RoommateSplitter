import argparse
import json
import re
from datetime import date
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

import mysql.connector

import database as db

STATIC = Path(__file__).with_name("static")
ASSETS = {
    "/": ("index.html", "text/html"),
    "/styles.css": ("styles.css", "text/css"),
    "/dashboard.js": ("dashboard.js", "text/javascript"),
}


def text(data, key, length, required=True):
    value = data.get(key, "")
    if not isinstance(value, str) or len(value.strip()) > length or (required and not value.strip()):
        raise ValueError(f"Enter a valid {key.replace('_', ' ')}.")
    return value.strip()


def integer(data, key):
    value = data.get(key)
    if type(value) is not int or not 0 < value <= 2_147_483_647:
        raise ValueError(f"Choose a valid {key.replace('_', ' ')}.")
    return value


def paise(data):
    value = text(data, "amount", 12)
    if not re.fullmatch(r"[0-9]{1,7}(?:\.[0-9]{1,2})?", value):
        raise ValueError("Enter a rupee amount with up to two digits after the decimal.")
    whole, _, fraction = value.partition(".")
    amount = int(whole) * 100 + int(fraction.ljust(2, "0"))
    if not 0 < amount <= 100_000_000:
        raise ValueError("Enter an amount between Rs. 0.01 and Rs. 10 lakh.")
    return amount


def entry_date(data):
    value = text(data, "date", 10)
    try:
        if not re.fullmatch(r"\d{4}-\d{2}-\d{2}", value):
            raise ValueError
        parsed = date.fromisoformat(value)
        if parsed.year < 1000:
            raise ValueError
        return parsed
    except ValueError:
        raise ValueError("Choose a valid calendar date.") from None


class Handler(BaseHTTPRequestHandler):
    def respond(self, status, content, mime):
        self.send_response(status)
        self.send_header("Content-Type", mime + "; charset=utf-8")
        self.send_header("Content-Length", str(len(content)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(content)

    def json(self, status, payload):
        self.respond(status, json.dumps(payload, default=str).encode("utf-8"), "application/json")

    def do_GET(self):
        path = urlsplit(self.path).path
        if path == "/api/dashboard":
            try:
                self.json(200, db.dashboard())
            except mysql.connector.Error as error:
                self.log_error("%s", error)
                self.json(503, {"error": "Couldn't read the household database."})
        elif path in ASSETS:
            name, mime = ASSETS[path]
            self.respond(200, (STATIC / name).read_bytes(), mime)
        else:
            self.json(404, {"error": "Page not found."})

    def do_POST(self):
        try:
            if self.headers.get_content_type() != "application/json":
                raise ValueError("Send data as JSON.")
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 8192:
                raise ValueError("Invalid request size.")
            data = json.loads(self.rfile.read(length))
            if not isinstance(data, dict):
                raise ValueError("Invalid form data.")
            path = urlsplit(self.path).path
            if path == "/api/roommates":
                db.add_roommate(text(data, "name", 80), text(data, "room", 30))
            elif path == "/api/roommates/remove":
                db.remove_roommate(integer(data, "roommate"))
            elif path == "/api/expenses":
                category = text(data, "category", 40)
                if category not in db.CATEGORIES:
                    raise ValueError("Choose an expense category.")
                db.add_expense(category, text(data, "note", 150, False), paise(data),
                               integer(data, "paid_by"), entry_date(data))
            elif path == "/api/payments":
                db.add_payment(integer(data, "sender"), integer(data, "receiver"),
                               paise(data), entry_date(data))
            else:
                self.json(404, {"error": "Action not found."})
                return
            self.json(201, {"ok": True})
        except (ValueError, UnicodeError) as error:
            self.json(400, {"error": str(error)})
        except mysql.connector.Error as error:
            self.log_error("%s", error)
            self.json(503, {"error": "Couldn't save your changes. Please try again."})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run the Roommate Splitter dashboard.")
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()
    try:
        db.initialize()
        with ThreadingHTTPServer(("127.0.0.1", args.port), Handler) as server:
            print(f"Roommate Splitter is running at http://127.0.0.1:{args.port}", flush=True)
            server.serve_forever()
    except KeyboardInterrupt:
        pass
    except (mysql.connector.Error, OSError, ValueError) as error:
        raise SystemExit(f"Startup failed: {error}") from None
