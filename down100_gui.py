# open_then_shutdown_repeat.py
import subprocess
import time
import pyautogui

EXE = "./Transform"      # use full path if needed
ARGS = ["gl"]            # your arg
REPEATS = 30             # how many open → shutdown cycles
STARTUP_DELAY = 1.0      # give the window time to appear
BETWEEN_RUNS = 0.2       # small pause between cycles
CLOSE_KEY = ("alt", "f4")  # how to close the window

for i in range(1, REPEATS + 1):
    print(f"Run {i}/{REPEATS}")
    proc = subprocess.Popen([EXE] + ARGS)

    # wait for window to appear and accept input
    time.sleep(STARTUP_DELAY)

    # try app friendly quit if it supports a key like 'q'
    # pyautogui.press('q')
    # time.sleep(0.2)

    # ensure shutdown: Alt+F4
    pyautogui.hotkey(*CLOSE_KEY)
    time.sleep(0.3)

    # if the process is still alive, terminate, then kill as fallback
    try:
        proc.terminate()
        proc.wait(timeout=2)
    except Exception:
        try:
            proc.kill()
        except Exception:
            pass

    time.sleep(BETWEEN_RUNS)

print("All done.")
