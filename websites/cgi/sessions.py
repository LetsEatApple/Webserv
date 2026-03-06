#!/usr/bin/env python3
import os
import sys
import uuid
import urllib.parse

# ---------- In-memory "database" ----------
USERS = {
    "admin": {"password": "admin123", "role": "admin"},
    "user":  {"password": "user123",  "role": "user"},
}

COLORS = [
    "primary", "secondary", "success", "danger", "warning", "info", "light", "dark"
]

# session_id -> {"username": ..., "role": ...}
# NOTE: this dict resets when the CGI process exits.
# For persistence across requests, use a file.
SESSION_FILE = "/tmp/webserv_sessions"

def page_header():
    return f"""<!DOCTYPE html>
<html lang="en" data-bs-theme="dark">
    <head>
        <title>Login</title>
        <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-sRIl4kxILFvY47J16cr9ZwB07vP4J8+LH7qKQnuqkuIAvNWLzeN8tE5YBujZqJLB" crossorigin="anonymous">
        <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/js/bootstrap.bundle.min.js" integrity="sha384-FKyoEForCGlyvwx9Hj09JcYn3nv7wiPVlz7YYwJrWVcXK/BmnVDxM+D2scQbITxI" crossorigin="anonymous"></script>
        <style>
            html, body {{ height: 100% }}
            .div-center {{ max-width: 330px; padding 1rem; }}
        </style>
    </head>"""
def load_sessions():
    sessions = {}
    try:
        with open(SESSION_FILE, "r") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                sid, username, role = line.split(":", 2)
                sessions[sid] = {"username": username, "role": role}
    except FileNotFoundError:
        pass
    return sessions

def save_sessions(sessions):
    with open(SESSION_FILE, "w") as f:
        for sid, data in sessions.items():
            f.write(f"{sid}:{data['username']}:{data['role']}\n")

# ---------- Helpers ----------
def get_cookies():
    cookies = {}
    cookie_header = os.environ.get("HTTP_COOKIE", "")
    if cookie_header:
        for pair in cookie_header.split(";"):
            pair = pair.strip()
            if "=" in pair:
                k, v = pair.split("=", 1)
                cookies[k.strip()] = v.strip()
    return cookies

def get_post_data():
    content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
    if content_length == 0:
        return {}
    body = sys.stdin.read(content_length)
    return dict(urllib.parse.parse_qsl(body))

def redirect(location, set_cookie=None):
    print(f"Status: 302 Found")
    if set_cookie:
        for cookie in set_cookie if isinstance(set_cookie, list) else [set_cookie]:
            print(f"Set-Cookie: {cookie}")
    print(f"Location: {location}")
    print()

def html_response(body, set_cookie=None):
    if set_cookie:
        for cookie in set_cookie if isinstance(set_cookie, list) else [set_cookie]:
            print(f"Set-Cookie: {cookie}")
    print("Content-Type: text/html")
    print()
    print(body)

def get_color_options(accent_color):
    options = ""
    for color in COLORS:
        selected = " selected" if accent_color == color else ""
        options += f'<option value="{color}"{selected}>{color}</option>\n'
    return options

# ---------- Pages ----------
def login_page(error=""):
    error_html = f'<p style="color:red">{error}</p>' if error else ""
    return f"""{page_header()}
    <body class="d-flex align-items-center py-4 bg-body-tertiary">
        <main class="div-center w-100 m-auto">
            <form method="POST" action="/sessions.py">
                <input type="hidden" name="action" value="login">
                <h1 class="h3 mb-3 fw-normal">Login</h1>
                <div id="userHelp" class="form-text">Users: admin/admin123 user/user123</div>
                {error_html}
                <div class="form-floating">
                    <input type="text" class="form-control" id="inputUsername" placeholder="username" name="username" required>
                    <label for="inputUsername">Username</label>
                </div>
                <div class="form-floating">
                    <input type="password" class="form-control" id="inputPassword" placeholder="password" name="password" required>
                    <label for="inputPassword">Password</label>
                </div>
                <button type="submit" class="btn btn-primary w-100 py-2 my-3">Log In</button>
                <p class="mt-5 mb-3">&copy; lhagemos, pschneid, mkugan 2026</p>
            </form>
        </main> 
    </body>
</html>"""

def dashboard_page(username, role, accent_color):
    if role == "admin":
        content = """
        <h2>Admin Panel</h2>
        <p>Welcome, <b>{username}</b>. You have <b>full access</b>.</p>
        <ul>
          <li>Manage users</li>
          <li>View server logs</li>
          <li>System configuration</li>
        </ul>""".format(username=username)
    else:
        content = """
        <h2>User Dashboard</h2>
        <p>Welcome, <b>{username}</b>. You have <b>standard access</b>.</p>
        <ul>
          <li>View your profile</li>
          <li>Browse content</li>
        </ul>""".format(username=username)

    return f"""{page_header()}
    <body class="d-flex align-items-center py-4 bg-body-tertiary">
        <main class="div-center w-100 m-auto">
            <h1 class="h3 mb-3 fw-normal">Dashboard <span class="badge text-bg-{accent_color}">{role}</span></h1>
            {content}
            <hr>
            <form method="POST" action="/sessions.py">
                <input type="hidden" name="action" value="settings">
                <label for="colorSelect">Change accent color</label>
                <div class="input-group">
                    <select class="form-select" name="accent_color" id="colorSelect">
                        {get_color_options(accent_color)}
                    </select>
                    <button type="submit" class="btn btn-primary">Save</button>
                </div>
            </form>
            <hr>
            <form method="POST" action="/sessions.py">
                <input type="hidden" name="action" value="logout">
                <button type="submit" class="btn btn-primary w-100 py-2 my-3">Logout</button>
            </form>
        </main>
    </body>
</html>"""

# ---------- Main ----------
def main():
    sessions = load_sessions()
    cookies = get_cookies()
    method = os.environ.get("REQUEST_METHOD", "GET")

    # Check existing session
    session_id = cookies.get("session_id", "")
    session = sessions.get(session_id)
    accent_color = cookies.get("accent_color", "secondary")

    if method == "POST":
        post = get_post_data()
        action = post.get("action", "")

        if action == "login":
            username = post.get("username", "")
            password = post.get("password", "")
            user = USERS.get(username)

            if user and user["password"] == password:
                sid = uuid.uuid4().hex
                sessions[sid] = {"username": username, "role": user["role"]}
                save_sessions(sessions)
                redirect("/sessions.py",
                         f"session_id={sid}; Path=/; HttpOnly; Max-Age=3600")
                return
            else:
                html_response(login_page("Invalid username or password"))
                return

        elif action == "logout":
            if session_id in sessions:
                del sessions[session_id]
                save_sessions(sessions)
            redirect("/sessions.py", [
                     "session_id=deleted; Path=/; HttpOnly; Max-Age=0",
                     "accent_color=deleted; Path=/; Max-Age=0"
            ])
            return

        elif action == "settings":
            new_color = post.get("accent_color", "secondary")
            if new_color not in COLORS:
                new_color = "secondary"
            redirect("/sessions.py",
                     f"accent_color={new_color}; Path=/; Max-Age=3600")
            return

    # GET or no valid action
    if session:
        html_response(dashboard_page(session["username"], session["role"], accent_color))
    else:
        html_response(login_page())

if __name__ == "__main__":
    main()
