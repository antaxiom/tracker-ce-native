#!/usr/bin/env python3
"""Live GUI control protocol for the tracker replica.

Lets an agent (or a vision model acting as an operator) drive the real app one
action at a time: start -> key/shot -> key/shot ... -> stop. Each call prints
only the information needed for the next decision, keeping context small.

Commands:
  start --screen loader [--out DIR]
  key Down
  shot name
  stop

Session state lives in <out>/session.json (pid, fifo, screen).
"""

import argparse
import json
import os
import signal
import subprocess
import sys
import time

DEFAULT_OUT = "/tmp/gui_session"


def session_paths(out):
    return (os.path.join(out, "session.json"), os.path.join(out, "session.fifo"))


def load_session(out):
    sp, _ = session_paths(out)
    if not os.path.exists(sp):
        sys.exit("no session; run start first")
    with open(sp) as f:
        return json.load(f)


def alive(pid):
    try:
        os.kill(pid, 0)
        return True
    except OSError:
        return False


def send(out, cmd):
    sess = load_session(out)
    _, fifo = session_paths(out)
    with open(fifo, "w") as f:
        f.write(cmd + "\n")


def cmd_start(args):
    out = args.out
    os.makedirs(out, exist_ok=True)
    sp, fifo = session_paths(out)
    if os.path.exists(sp):
        with open(sp) as f:
            old = json.load(f)
        if alive(old.get("pid", -1)):
            sys.exit("session already running; stop it first")
        os.remove(sp)
    if os.path.exists(fifo):
        os.remove(fifo)
    os.mkfifo(fifo)

    app = args.app
    extra = ["--loader-dir", args.loader_dir] if getattr(args, "loader_dir", None) else []
    mode = [] if getattr(args, "live", False) else ["--lcd-shot"]
    proc = subprocess.Popen(
        [app, "--screen", args.screen, "--polyend-font", *mode, "--remote", fifo] + extra,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
    with open(sp, "w") as f:
        json.dump({"pid": proc.pid, "fifo": fifo, "out": out, "screen": args.screen}, f)

    # Wait for the app to render and process a command before accepting input.
    # A fixed sleep raced with the first (sometimes slow) GPU/font startup and
    # made the first key/click land before the app was listening.
    ready = os.path.join(out, ".ready.png")
    if os.path.exists(ready):
        os.remove(ready)
    try:
        send(out, "shot " + ready)
    except OSError:
        pass
    deadline = time.time() + 12.0
    while time.time() < deadline:
        if os.path.exists(ready) and os.path.getsize(ready) > 1000:
            break
        if not alive(proc.pid):
            sys.exit("app exited during startup")
        time.sleep(0.05)
    else:
        sys.exit("app did not become ready")
    if os.path.exists(ready):
        os.remove(ready)
    print(json.dumps({"started": True, "screen": args.screen, "out": out}))


def cmd_key(args):
    send(args.out, "key " + args.name)
    time.sleep(0.30)
    print(json.dumps({"key": args.name, "ok": True}))


def cmd_click(args):
    send(args.out, f"click {args.x} {args.y}")
    time.sleep(0.30)
    print(json.dumps({"click": [args.x, args.y], "ok": True}))


def cmd_type(args):
    send(args.out, "type " + args.text)
    time.sleep(0.30)
    print(json.dumps({"type": args.text, "ok": True}))


def cmd_shot(args):
    out = args.out
    name = args.name
    if not name.endswith(".png"):
        name += ".png"
    path = os.path.join(out, name)
    if os.path.exists(path):
        os.remove(path)
    send(out, "shot " + path)
    deadline = time.time() + 4.0
    while time.time() < deadline:
        if os.path.exists(path) and os.path.getsize(path) > 1000:
            time.sleep(0.05)
            print(path)
            return
        time.sleep(0.05)
    sys.exit("screenshot timeout")


def cmd_stop(args):
    out = args.out
    try:
        send(out, "quit")
    except OSError:
        pass
    sess = load_session(out)
    pid = sess.get("pid", -1)
    for _ in range(30):
        if not alive(pid):
            break
        time.sleep(0.1)
    if alive(pid):
        try:
            os.kill(pid, signal.SIGTERM)
        except OSError:
            pass
    sp, fifo = session_paths(out)
    for p in (sp, fifo):
        if os.path.exists(p):
            os.remove(p)
    print(json.dumps({"stopped": True}))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=DEFAULT_OUT)
    sub = ap.add_subparsers(dest="action", required=True)

    s = sub.add_parser("start")
    s.add_argument("--screen", required=True)
    s.add_argument("--app", default="./native/build/trakr_app")
    s.add_argument("--loader-dir")
    s.add_argument("--live", action="store_true")
    s.set_defaults(func=cmd_start)

    k = sub.add_parser("key")
    k.add_argument("name")
    k.set_defaults(func=cmd_key)

    t = sub.add_parser("type")
    t.add_argument("text")
    t.set_defaults(func=cmd_type)

    c = sub.add_parser("click")
    c.add_argument("x", type=float)
    c.add_argument("y", type=float)
    c.set_defaults(func=cmd_click)

    sh = sub.add_parser("shot")
    sh.add_argument("name")
    sh.set_defaults(func=cmd_shot)

    st = sub.add_parser("stop")
    st.set_defaults(func=cmd_stop)

    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
