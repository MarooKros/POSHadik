import sys
import socket
import threading
import tkinter as tk
from tkinter import messagebox
import time as time_module

CELL = 30
BOARD_W, BOARD_H = 20, 20

class SnakeGUI:
    def __init__(self, master, server_ip, port):
        self.master = master
        self.master.title("POSHadik GUI Client")
        self.server_ip = server_ip
        self.port = port
        
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.sock.connect((server_ip, int(port)))
        except Exception as e:
            messagebox.showerror("Connection error", str(e))
            master.destroy()
            return
        
        self.running = True
        self.snakes = []
        self.fruits = []
        self.obstacles = []
        self.game_over = False
        self.scores = [0] * 10
        self.start_time = time_module.time()
        self.sock2 = None
        self.initial_snakes = 1
        self.in_menu = True
        self.winner_shown = False
        
        self.current_mode = 0
        self.current_time_limit = 0
        self.current_obstacles = 0
        self.server_elapsed = 0
        self.paused = False

        self.main_frame = tk.Frame(master)
        self.main_frame.pack(fill=tk.BOTH, expand=True)
        self.info_frame = tk.Frame(self.main_frame)
        self.info_frame.pack(fill=tk.X, padx=5, pady=5)
        self.info_label = tk.Label(self.info_frame, text="Score: 0  Time: 0s", font=("Arial", 12))
        self.info_label.pack(side=tk.LEFT)
        self.canvas = tk.Canvas(self.main_frame, width=BOARD_W*CELL, height=BOARD_H*CELL, bg="black")
        self.canvas.pack()
        self.master.bind('<Key>', self.on_key)
        
        def on_close():
            self.running = False
            self.send_msg("LEAVE")
            if self.sock2:
                try:
                    self.sock2.sendall("LEAVE".encode())
                    self.sock2.close()
                except:
                    pass
                self.sock2 = None
            self.master.destroy()
        
        self.master.protocol("WM_DELETE_WINDOW", on_close)
        
        self.pending_state = None

        threading.Thread(target=self.listen, daemon=True).start()
        
        self.show_menu()
    
    def show_menu(self):
        self.in_menu = True
        self.winner_shown = False
        dialog = tk.Toplevel(self.master)
        dialog.title("Menu")
        dialog.geometry("500x700")
        dialog.configure(bg='black')
        dialog.transient(self.master)
        dialog.grab_set()
        
        def on_close():
            self.running = False
            self.send_msg("LEAVE")
            self.master.destroy()
        
        dialog.protocol("WM_DELETE_WINDOW", on_close)
        
        title = tk.Label(dialog, text="POSHadik", font=("Arial", 24, "bold"), bg='black', fg='lime')
        title.pack(pady=20)
        
        width_var = tk.StringVar(value="20")
        height_var = tk.StringVar(value="20")
        mode_var = tk.IntVar(value=0)
        time_var = tk.StringVar(value="0")
        obstacles_var = tk.IntVar(value=0)

        form = tk.Frame(dialog, bg='black')
        form.pack(pady=10)
        tk.Label(form, text="Width", bg='black', fg='white').grid(row=0, column=0, padx=6, pady=4, sticky='e')
        tk.Entry(form, textvariable=width_var, width=8).grid(row=0, column=1, padx=6, pady=4)
        tk.Label(form, text="Height", bg='black', fg='white').grid(row=0, column=2, padx=6, pady=4, sticky='e')
        tk.Entry(form, textvariable=height_var, width=8).grid(row=0, column=3, padx=6, pady=4)

        tk.Label(form, text="Mode", bg='black', fg='white').grid(row=1, column=0, padx=6, pady=4, sticky='e')
        tk.Radiobutton(form, text="Standard", variable=mode_var, value=0, bg='black', fg='white', selectcolor='black').grid(row=1, column=1)
        tk.Radiobutton(form, text="Timed", variable=mode_var, value=1, bg='black', fg='white', selectcolor='black').grid(row=1, column=2)
        tk.Label(form, text="Time limit (s)", bg='black', fg='white').grid(row=2, column=0, padx=6, pady=4, sticky='e')
        tk.Entry(form, textvariable=time_var, width=8).grid(row=2, column=1, padx=6, pady=4)

        tk.Label(form, text="Obstacles", bg='black', fg='white').grid(row=3, column=0, padx=6, pady=4, sticky='e')
        tk.Radiobutton(form, text="None", variable=obstacles_var, value=0, bg='black', fg='white', selectcolor='black').grid(row=3, column=1)
        tk.Radiobutton(form, text="Random", variable=obstacles_var, value=1, bg='black', fg='white', selectcolor='black').grid(row=3, column=2)
        tk.Radiobutton(form, text="From file", variable=obstacles_var, value=2, bg='black', fg='white', selectcolor='black').grid(row=3, column=3)

        def new_game():
            self.game_over = False
            self.in_menu = False
            self.winner_shown = False
            try:
                w = int(width_var.get())
                h = int(height_var.get())
                m = int(mode_var.get())
                tl = int(time_var.get()) if mode_var.get() == 1 else 0
                ob = int(obstacles_var.get())
            except ValueError:
                messagebox.showerror("Chyba", "Prosím zadajte čísla pre rozmery a čas.")
                return
            self.current_mode = m
            self.current_time_limit = tl
            self.current_obstacles = ob
            self.send_msg(f"NEW_GAME {w} {h} {m} {tl} {ob}")
            self.initial_snakes = 1
            import time as t
            t.sleep(0.3)
            self.send_msg("JOIN")
            self.send_msg("PAUSE")
            self.paused = True
            self.start_time = time_module.time()
            dialog.destroy()
        
        def join_game():
            self.game_over = False
            self.in_menu = False
            self.winner_shown = False
            self.current_mode = 0
            self.current_time_limit = 0
            self.current_obstacles = 0
            self.send_msg("JOIN")
            self.initial_snakes = 1
            self.start_time = time_module.time()
            dialog.destroy()
        
        def exit_game():
            self.running = False
            self.send_msg("LEAVE")
            self.master.destroy()
        
        btn_frame = tk.Frame(dialog, bg='black')
        btn_frame.pack(padx=10, pady=10)
        
        btn_width = 30
        btn_height = 4
        
        tk.Button(btn_frame, text="New Game", width=btn_width, height=btn_height, font=("Arial", 14), bg='#1a1a1a', fg='lime', command=new_game).pack(pady=12)
        tk.Button(btn_frame, text="Join Game", width=btn_width, height=btn_height, font=("Arial", 14), bg='#1a1a1a', fg='lime', command=join_game).pack(pady=12)
        tk.Button(btn_frame, text="Exit", width=btn_width, height=3, font=("Arial", 14), bg='#1a1a1a', fg='red', command=exit_game).pack(pady=12)

    def send_msg(self, msg):
        try:
            self.sock.sendall(msg.encode())
        except:
            pass

    def listen(self):
        expect_state = False
        while self.running:
            try:
                data = self.sock.recv(131072)
                if not data:
                    break
                decoded = data.decode()
                for line in decoded.split('\n'):
                    txt = line.strip()
                    if not txt:
                        continue
                    if expect_state:
                        self.parse_state(txt)
                        expect_state = False
                        continue
                    if txt.startswith('GAME_STATE'):
                        content = txt[len('GAME_STATE'):]
                        if content:
                            self.parse_state(content)
                        else:
                            expect_state = True
            except Exception as e:
                break
        self.sock.close()
    
    def listen_player2(self):
        if not self.sock2:
            return
        while self.running:
            try:
                data = self.sock2.recv(131072)
                if not data:
                    break
                decoded = data.decode()
                for line in decoded.split('\n'):
                    if line.strip():
                        txt = line.strip()
                        if txt.startswith('OK') or txt.startswith('ERROR') or txt.startswith('GAME_STATE'):
                            continue
            except:
                break
        if self.sock2:
            self.sock2.close()
    
    def show_winner_dialog(self):
        scores_live = [max(0, len(snake) - 1) for snake in self.snakes]
        final_scores = [s for s in self.scores if s > 0] if any(self.scores) else scores_live
        
        dialog = tk.Toplevel(self.master)
        dialog.title("Hra skoncila")
        dialog.geometry("500x450")
        dialog.configure(bg='black')
        dialog.transient(self.master)
        dialog.grab_set()
        
        def on_close():
            self.running = False
            self.send_msg("LEAVE")
            if self.sock2:
                self.sock2.sendall("LEAVE".encode())
                self.sock2.close()
                self.sock2 = None
            self.master.destroy()
        
        dialog.protocol("WM_DELETE_WINDOW", on_close)
        
        tk.Label(dialog, text="Hra skoncila!", font=("Arial", 28, "bold"), fg="red", bg='black').pack(pady=20)
        
        scores_frame = tk.Frame(dialog, bg='black')
        scores_frame.pack(pady=10)
        for i, score in enumerate(final_scores):
            tk.Label(scores_frame, text=f"Hrac {i+1}: {score}", font=("Arial", 18), fg="white", bg='black').pack(pady=5)
        
        def new_game():
            self.game_over = False
            self.in_menu = False
            self.winner_shown = False
            self.send_msg(f"NEW_GAME {BOARD_W} {BOARD_H} {self.current_mode} {self.current_time_limit} {self.current_obstacles}")
            import time as t
            t.sleep(0.3)
            self.send_msg("JOIN")
            if self.sock2:
                try:
                    self.sock2.sendall("JOIN".encode())
                except:
                    pass
            self.start_time = time_module.time()
            dialog.destroy()
        
        def back_to_menu():
            self.game_over = True
            self.in_menu = True
            self.winner_shown = False
            self.send_msg("LEAVE")
            if self.sock2:
                try:
                    self.sock2.sendall("LEAVE".encode())
                    self.sock2.close()
                except:
                    pass
                self.sock2 = None
            dialog.destroy()
            self.master.after(100, self.show_menu)
        
        def exit_game():
            self.running = False
            self.send_msg("LEAVE")
            if self.sock2:
                try:
                    self.sock2.sendall("LEAVE".encode())
                    self.sock2.close()
                except:
                    pass
                self.sock2 = None
            self.master.destroy()
        
        tk.Button(dialog, text="Nova hra", width=20, height=2, font=("Arial", 14), bg='#1a1a1a', fg='lime', command=new_game).pack(pady=10)
        tk.Button(dialog, text="Navrat do menu", width=20, height=2, font=("Arial", 14), bg='#1a1a1a', fg='yellow', command=back_to_menu).pack(pady=10)
        tk.Button(dialog, text="Ukoncit", width=20, height=2, font=("Arial", 14), bg='#1a1a1a', fg='red', command=exit_game).pack(pady=10)

    def parse_state(self, state):
        if self.game_over or self.in_menu:
            return

        try:
            print("Parsing state:\n", state)
            parts = state.split(';')
            if not parts[0]:
                return
            wh = parts[0].split(',')
            if len(wh) < 2:
                return
            global BOARD_W, BOARD_H
            BOARD_W, BOARD_H = int(wh[0]), int(wh[1])
            self.canvas.config(width=BOARD_W*CELL, height=BOARD_H*CELL)
            # Resize window to fit new canvas size
            window_width = max(400, BOARD_W*CELL + 20)
            window_height = max(400, BOARD_H*CELL + 100)
            self.master.geometry(f"{window_width}x{window_height}")
            self.snakes = []
            self.fruits = []
            self.obstacles = []
            self.server_elapsed = 0
            for part in parts[1:]:
                if not part:
                    continue
                if part.startswith('sc'):
                    vals = part[2:].split(',')
                    parsed = []
                    for v in vals[:10]:
                        if v == '':
                            parsed.append(0)
                        else:
                            try:
                                parsed.append(int(v))
                            except:
                                parsed.append(0)
                    while len(parsed) < 10:
                        parsed.append(0)
                    self.scores = parsed
                    continue
                if part.startswith('t'):
                    try:
                        self.server_elapsed = int(part[1:])
                    except:
                        self.server_elapsed = 0
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
                    if self.game_over and not self.winner_shown:
                        self.winner_shown = True
                        self.show_winner_dialog()
                        return
                elif part.startswith('p'):
                    self.paused = (part == 'p1')
            self.draw()
        except Exception as e:
            pass

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
        
        elapsed = self.server_elapsed if self.server_elapsed else int(time_module.time() - self.start_time)
        if self.current_time_limit > 0:
            time_text = f"Time: {elapsed}/{self.current_time_limit}s"
        else:
            time_text = f"Time: {elapsed}s"
        # Compute live scores from snake lengths to avoid index mixups
        scores_live = [max(0, len(snake) - 1) for snake in self.snakes]
        score_str = "  |  ".join([f"P{i+1}: {s}" for i, s in enumerate(scores_live)]) if scores_live else "Score: 0"
        players_text = f"Players: {len(self.snakes)}"
        help_text = "Press 'P' to Resume" if self.paused else "Press 'P' to Pause"
        self.info_label.config(text=f"{players_text}  |  {score_str}  |  {time_text} | {help_text}")

    def on_key(self, event):
        key = event.keysym.lower()
        if key == 'q':
            self.running = False
            self.send_msg("LEAVE")
            if self.sock2:
                self.sock2.sendall("LEAVE".encode())
            self.master.destroy()
        # Player 1 controls (WASD)
        elif key in ('w',):
            self.send_msg("MOVE_UP")
        elif key in ('s',):
            self.send_msg("MOVE_DOWN")
        elif key in ('a',):
            self.send_msg("MOVE_LEFT")
        elif key in ('d',):
            self.send_msg("MOVE_RIGHT")
        # Player 2 controls (Arrow keys)
        elif key in ('up',):
            if self.sock2:
                self.sock2.sendall("MOVE_UP".encode())
        elif key in ('down',):
            if self.sock2:
                self.sock2.sendall("MOVE_DOWN".encode())
        elif key in ('left',):
            if self.sock2:
                self.sock2.sendall("MOVE_LEFT".encode())
        elif key in ('right',):
            if self.sock2:
                self.sock2.sendall("MOVE_RIGHT".encode())
        elif key == 'p':
            if self.paused:
                self.send_msg("RESUME")
                self.paused = False
            else:
                self.send_msg("PAUSE")
                self.paused = True

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python gui_client.py <server_ip> <port>")
        sys.exit(1)
    root = tk.Tk()
    app = SnakeGUI(root, sys.argv[1], sys.argv[2])
    root.mainloop()
