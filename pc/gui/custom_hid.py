import hid
import tkinter as tk
from tkinter import font, ttk

VID_DEFAULT = 0x03EA
PID_DEFAULT = 0x2710

MODE_RESET = 16

MODE_MAP = {
    "Set CCW Angle": 0,
    "Set CW Angle": 1,
    "Add CCW Angle": 2,
    "Add CW Angle": 3,
}

REPORT_LEN = 64  # HID report length (bytes)

QUICK_STEPS = [-90, -45, -30, -15, -5, 0, 5, 15, 30, 45, 90]

DEFAULT_ANGLE_DEG = 0

def check_status() -> None:
    try:
        h = hid.device()
        h.open(VID_DEFAULT, PID_DEFAULT)
        h.close()
        status_var.set("Status: Connected")

    except IOError:
        status_var.set("Status: Disconnected")

    master.after(1000, check_status)


def encode_angle_bytes(angle: int) -> tuple[int, int]:
    if angle > 255:
        return 1, angle - 255
    return 0, angle


def build_report(mode: int, angle: int) -> list[int]:
    if mode == MODE_RESET:
        angle1, angle2 = 0, 0
    else:
        angle1, angle2 = encode_angle_bytes(angle)

    report = [0, int(mode), int(angle1), int(angle2)]
    report += [0] * (REPORT_LEN - len(report))
    return report


def send_hid(mode: int, angle: int) -> bool:
    print(f"mode chosen: {mode}, angle chosen: {angle}")

    try:
        h = hid.device()
        h.open(VID_DEFAULT, PID_DEFAULT)
        h.set_nonblocking(1)

        report = build_report(mode, angle)
        h.write(report)
        
        status_var.set("Status: Connected")
        return True

    except IOError as ex:
        print(ex)
        print("You probably don't have the hard-coded device.")
        print("Update the VID/PID (or open method) and try again.")
        status_var.set(f"Status: Disconnected ({ex})")
        return False
    
    finally:
        try:
            h.close()
        except Exception:
            pass


def reset_cmd() -> None:
    if send_hid(MODE_RESET, 0):
        set_current_angle_display(DEFAULT_ANGLE_DEG)


def send_cmd() -> None:
    # Validate mode selection
    mode_label = mode_var.get()
    if mode_label not in MODE_MAP:
        print("Please choose a mode.")
        return

    # Validate angle input
    try:
        angle = int(entry_angle.get())
        if angle < 0 or angle > 270:
            print("Please enter integer [0, 270]")
            return
    except ValueError:
        print("Invalid input: angle must be an integer.")
        return

    mode = MODE_MAP[mode_label]
    ok = send_hid(mode, angle)

    if not ok:
        return
    
    # Update current angle label
    if mode_label == "Add CCW Angle":
        set_current_angle_display(current_angle_deg + abs(angle))
    elif mode_label == "Add CW Angle":
        set_current_angle_display(current_angle_deg - abs(angle))
    elif mode_label == "Set CW Angle":
        set_current_angle_display(-angle)
    else:
        set_current_angle_display(angle)


def quick_step_cmd(step: int) -> None:
    if step == 0:
        if send_hid(MODE_RESET, 0):
            set_current_angle_display(DEFAULT_ANGLE_DEG)
        return

    if step < 0:
        ok = send_hid(MODE_MAP["Add CW Angle"], abs(step))
        if ok:
            set_current_angle_display(current_angle_deg - abs(step))
    else:
        ok = send_hid(MODE_MAP["Add CCW Angle"], step)
        if ok:
            set_current_angle_display(current_angle_deg + step)


def set_current_angle_display(angle: int) -> None:
    global current_angle_deg
    current_angle_deg = angle

    if current_angle_deg > 135:
        current_angle_deg = 135
    elif current_angle_deg < -135:
        current_angle_deg = -135

    current_angle_var.set(f"{current_angle_deg}°")

# UI layout
master = tk.Tk()
master.title("Servo HID Controller")

default_font = font.nametofont("TkDefaultFont")
default_font.configure(size=12)
entry_font = font.nametofont("TkTextFont")
entry_font.configure(size=12)

# Status label
status_var = tk.StringVar(value="Status: Disconnected")
tk.Label(
    master,
    textvariable=status_var,
    anchor="w"
).grid(row=0, column=0, columnspan=2, sticky="w", padx=10, pady=(8, 4))

send_hid(MODE_RESET, 0)

tk.Label(master, text="Mode").grid(row=1, column=0, sticky="e", padx=10, pady=6)
tk.Label(master, text="Angle").grid(row=2, column=0, sticky="e", padx=10, pady=6)

# Mode dropdown
mode_var = tk.StringVar(value="Choose Mode")
mode_combo = ttk.Combobox(
    master,
    textvariable=mode_var,
    values=list(MODE_MAP.keys()),
    state="readonly",  # prevents typing random text
    width=18,
)
mode_combo.grid(row=1, column=1, sticky="w", padx=10, pady=6)

# Angle entry
entry_angle = tk.Entry(master, width=20)
entry_angle.grid(row=2, column=1, sticky="w", padx=10, pady=6)

# Send command button
button_frame = tk.Frame(master)
button_frame.grid(row=3, column=0, columnspan=2)
tk.Button(button_frame, text="Send Command", command=send_cmd, padx=20).pack()

# Angle value display
current_angle_deg = DEFAULT_ANGLE_DEG
current_angle_var = tk.StringVar(value="0°")

tk.Label(button_frame, textvariable=current_angle_var).pack(pady=(6, 0))

# Quick change angle buttons
quick_frame = tk.Frame(master)
quick_frame.grid(row=5, column=0, columnspan=2, pady=(10, 0))
for step in QUICK_STEPS:
    if step > 0:
        label = f"+{step}°"
    else:
        label = f"{str(step)}°"

    tk.Button(
        quick_frame,
        text=label,
        command=lambda s=step: quick_step_cmd(s),
        padx=10,
    ).pack(side="left", padx=3)

check_status()

tk.mainloop()
