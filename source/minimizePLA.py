from espresso_func import minimize
import time

def countLines():
    numLines = 0
    with open(GENERATED_PLA, 'r') as f:
        for _, _ in enumerate(f):
            numLines += 1
    numSkipLines = 5
    numLines -= numSkipLines
    return numLines

BOARD_SIZE = 8
GENERATED_PLA = f"original{BOARD_SIZE}x{BOARD_SIZE}.pla"
print(f"Minimizing {GENERATED_PLA} file...")
MINIMIZED_OUTPUT_FILE = f"minimized{BOARD_SIZE}x{BOARD_SIZE}.pla"

print('conuting lines...')
numLines = countLines()
print('Minimizing...')
start = time.time()
minimize(GENERATED_PLA, MINIMIZED_OUTPUT_FILE)
end = time.time()

timeLength = end - start
RUN_TIMES_FILE = "run_time_length.txt"
with open(RUN_TIMES_FILE, "a") as f:
    f.write(f"{numLines} lines, {timeLength}\n")
print("Time elpassed = ", timeLength)
print(f"Finished!, file saved as {MINIMIZED_OUTPUT_FILE}.")

