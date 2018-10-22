# preprocessing c table into something more managable
import pyeda

# only white figures movement are of concern
# isWhiteTurn, isRookCaptured, BKx, BKy, WKx, WKy, WRx, WRy
lookup = {}
BOARD_SIZE = 3

MOVE_BIT_LENGTH = 3
if BOARD_SIZE <=4:
    MOVE_BIT_LENGTH = 2
elif BOARD_SIZE <= 8:
    MOVE_BIT_LENGTH = 3
else:
    raise ValueError('Not accepting board sizes above 8')

#positions in initial raed table
BKx = 2
BKy = 3
WKx = 4
WKy = 5
WRx = 6
WRy = 7

print ("reading file...")
with open('chessDict.txt', 'r') as f:
    next(f) # skips file comment
    # make a dict
    for (i, line) in enumerate(f):
        isWhiteTurn, isRookCaptured, tBKx, tBKy, tWKx, tWKy, \
            tWRx, tWRy = [int(i) for i in line.split()]
        
        tup = (isWhiteTurn, isRookCaptured, tBKx, tBKy, tWKx, tWKy, tWRx, tWRy)
        if (i % 2 == 0):
            key = tup
        else:
            lookup[key] = tup


#for key, val in lookup.items():
#   print (key, val)

print('finished!')
print("Items read = ", len(lookup))

# maps from current game state into optimal move
# state -> move
# key: 3 bits for each BKx, BKy, WKx, Wky, Wrx, Wry = 18 bits
# value: 1 bit king/rook, up, down, left, right, 2 bits - how many times 
bitLookup = {}

# returns a list of bools indicating the number
def numberIntoBits(num):
    return [i == num for i in range(BOARD_SIZE)]
    #return bin(num)[2:].zfill(8)

#returns true if king played. considering the table, its only false when rook played
def didKingPlay(key, value):
    return (key[WKx] != value[WKx] or key[WKy] != value[WKy])

def getKingDirection(key, value):
    x1 = key[WKx]
    y1 = key[WKy]
    x2 = value[WKx]
    y2 = value[WKy]
    left = x1 > x2
    right = x2 > x1
    up = y2 > y1
    down = y1 > y2
    return up,down,left,right

def getRookDirectionAndDistance(key, value):
    x1 = key[WRx]
    y1 = key[WRy]
    x2 = value[WRx]
    y2 = value[WRy]
    left = x1 > x2
    right = x2 > x1
    up = y2 > y1
    down = y1 > y2

    distance = abs(x1 - x2 + y1 - y2)
    return up,down,left,right, distance

def boolList2BinString(lst):
    return ''.join(['1' if x else '0' for x in lst])

for key, val in lookup.items():
    keyBits = numberIntoBits(key[BKx])
    keyBits.extend(numberIntoBits(key[BKy]) )
    keyBits.extend(numberIntoBits(key[WKx]) )
    tmp = numberIntoBits(key[WKy])
    keyBits.extend( tmp )
    keyBits.extend(numberIntoBits(key[WRx]) )
    keyBits.extend(numberIntoBits(key[WRy]) )
    
    valueBits = []
    isKing = didKingPlay(key, val)
    valueBits.append(isKing)

    if isKing:
        valueBits.extend(getKingDirection(key, val))
        # king always moves one field
        valueBits.extend([True, False])
    else:
        up,down,left,right,distance = getRookDirectionAndDistance(key, val)
        valueBits.extend([up,down,left,right])
        # this is fine for a 3x3, but change it for a larger board
        distanceBits = [False, False]
        distanceBits[distance - 1] = True
        valueBits.extend(distanceBits)
    
    strKey = boolList2BinString(keyBits)
    #print("original key=",key)
    #print("keyBits = ", "[BKx1,  Bkx2,  bkx3,  bky1,  bky2,  bky3,  wkx1,  wkx2,  wkx3,  wky1,  wky2,  wky3,  wrx1,  wrx2,  wrx3,  wry1,  wry2,  wry3]") 
    #print("keyBits = ", keyBits, "strKey = ", strKey)
    #print("tmp = ",tmp, "key[WKy] = ", key[WKy], "key[5] = ", key[5])
    valueBitString = boolList2BinString(valueBits)
    bitLookup[strKey] = valueBitString

print('bitLookup lenght = ', len(bitLookup))

def printTable():
    for key, val in bitLookup.items():
        chunks, chunk_size = len(key), 3
        printable = [ key[i:i+chunk_size] for i in range(0, chunks, chunk_size) ]
        print(" ".join(printable) ,"|", val)
#printTable()
#exit()

def tableToPla(table, fileName):
    f = open(fileName, "w")
    aKey = next(iter(table))
    aVal = table[aKey]
    keySize = len(aKey)
    valSize = len(aVal)
    print(f".i {keySize}", file=f)
    print(f".o {valSize}", file=f)
    numEntries = len(table)
    print(f".p {numEntries}", file=f)
    print(".type fr", file=f)
    for key, val in table.items():
        print(f"{key} {val}", file=f)
    f.close()

GENERATED_PLA = "mytest.pla"
tableToPla(bitLookup, GENERATED_PLA)

from espresso_func import minimize

minimize(GENERATED_PLA, "minimized.pla")
# problem: bice previse (nepostojecih) redova, pa ce stringovi
# sa vrednostima {0,1,-} biti predugi
# mozda moze ovom espressu da se prosledi ovaj PLA file

# TODO: 
# 1) DONE create boolean representation of key and value
# 2) export it as a PLA file
# 3) run espresso minimizer
