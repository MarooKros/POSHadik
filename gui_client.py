import sys
import socket
import threading
import tkinter as tk
from tkinter import messagebox

CELL = 30
BOARD_W, BOARD_H = 20, 20

class SnakeGUI:
    def __init__(self, master, server_ip, port):
        self.master = master
        self.master.title("POSHadik GUI Client")
        self.canvas = tk.Canvas(master, width=BOARD_W*CELL, height=BOARD_H*CELL, bg="black")
        self.canvas.pack()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.sock.connect((server_ip, int(port)))
            print("Connected to server")
        except Exception as e:
            print(f"Connection error: {e}")
            messagebox.showerror("Connection error", str(e))
            master.destroy()
            return
        self.running = True
        self.snakes = []
        self.fruits = []
        self.obstacles = []
        self.game_over = False
        self.master.bind('<Key>', self.on_key)
        threading.Thread(target=self.listen, daemon=True).start()
        import time
        time.sleep(0.5)
        print("Sending NEW_GAME")
        self.send_msg("NEW_GAME 20 20 0 0 0")
        time.sleep(0.5)
        print("Sending JOIN")
        self.send_msg("JOIN")

    def send_msg(self, msg):
        try:
            self.sock.sendall(msg.encode())
        except:
            pass

    def listen(self):
        while self.running:
            try:
                data = self.sock.recv(131072)
                if not data:
                    print("Server closed connection")
                    break
                decoded = data.decode()
                print(f"Received: {repr(decoded[:120])}")
                for line in decoded.split('\n'):
                    if line.strip():
                        txt = line.strip()
                        if txt.startswith('OK') or txt.startswith('ERROR'):
                            print(f"Skipping: {txt}")
                            continue
                        if txt.startswith('GAME_STATE'):
                            txt = txt[len('GAME_STATE'):]
                            if not txt:
                                continue
                        self.parse_state(txt)
            except Exception as e:
                print(f"Listen error: {e}")
                break
        self.sock.close()

    def parse_state(self, state):
        try:
            print(f"Parsing: {state[:80]}")
            parts = state.split(';')
            if not parts[0]:
                return
            wh = parts[0].split(',')
            if len(wh) < 2:
                print(f"Invalid format: {state}")
                return
            global BOARD_W, BOARD_H
            BOARD_W, BOARD_H = int(wh[0]), int(wh[1])
            print(f"Board: {BOARD_W}x{BOARD_H}")
            self.snakes = []
            self.fruits = []
            self.obstacles = []
            for part in parts[1:]:
                if not part:
                    continue
                if part.startswith('sc'):
                    continue
                if part.startswith('s'):
                    vals = list(map(int, part[1:].split(',')))
                    length, direction = vals[0], vals[1]
                    body = [(vals[i], vals[i+1]) for i in range(2, len(vals), 2)]
                    self.snakes.append(body)
                elif part.startswith('f'):
                    x, y = map(int, part[1:].split(','))
                    self.fruits.append((x, y))
                elif part.startswith('o'):
                    x, y = map(int, part[1:].split(','))
                    self.obstacles.append((x, y))
                elif part.startswith('go'):
                    self.game_over = (part == 'go1')
            self.draw()
        except Exception as e:
            print(f"Parse error: {e}")

    def draw(self):
        self.canvas.delete('all')
        for x, y in self.obstacles:
            self.canvas.create_rectangle(x*CELL, y*CELL, (x+1)*CELL, (y+1)*CELL, fill='gray')
        for x, y in self.fruits:
            self.canvas.create_oval(x*CELL+5, y*CELL+5, (x+1)*CELL-5, (y+1)*CELL-5, fill='red')
        colors = ['lime', 'yellow', 'cyan', 'magenta', 'white']
        for idx, snake in enumerate(self.snakes):
            for i, (x, y) in enumerate(snake):
                color = colors[idx % len(colors)]
                self.canvas.create_rectangle(x*CELL, y*CELL, (x+1)*CELL, (y+1)*CELL, fill=color)
        if self.game_over:
            self.canvas.create_text(BOARD_W*CELL//2, BOARD_H*CELL//2, text="GAME OVER", fill="red", font=("Arial", 32))

    def on_key(self, event):
        key = event.keysym.lower()
        if key == 'q':
            self.running = False
            self.send_msg("LEAVE")
            self.master.destroy()
        elif key in ('up', 'w'):
            self.send_msg("MOVE_UP")
        elif key in ('down', 's'):
            self.send_msg("MOVE_DOWN")
        elif key in ('left', 'a'):
            self.send_msg("MOVE_LEFT")
        elif key in ('right', 'd'):
            self.send_msg("MOVE_RIGHT")
        elif key == 'p':
            self.send_msg("PAUSE")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python gui_client.py <server_ip> <port>")
        sys.exit(1)
    root = tk.Tk()
    app = SnakeGUI(root, sys.argv[1], sys.argv[2])
    root.mainloop()
