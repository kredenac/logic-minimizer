
print ("reading file...")

# change to dict
lookup = []
with open('chessDict.txt', 'r') as f:
    next(f) # skips file comment
    for line in f:
        isWhiteTurn, isRookCaptured, BKx, BKy, WKx, WKy, \
            WRx, WRy = [int(i) for i in line.split()]
        print (f'{isWhiteTurn}, {isRookCaptured}, {BKx}, {BKy}, {WKx}, {WKy}, {WRx}, {WRy}')

print('finished!')