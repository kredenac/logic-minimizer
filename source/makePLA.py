# preprocessing c table into something more managable
import pyeda

# only white figures movement are of concern
# isWhiteTurn, isRookCaptured, BKx, BKy, WKx, WKy, WRx, WRy
lookup = {}
BOARD_SIZE = 4

# number of bits for coding the direction
UDLR_BITS = 4

# is tracking of move fields in 2 directions or 1
NUM_DIR_MOVES = 2

POS_BIT_LEN = 3
if BOARD_SIZE <=4:
    POS_BIT_LEN = 2
elif BOARD_SIZE <= 8:
    POS_BIT_LEN = 3
else:
    raise ValueError('Not accepting board sizes above 8')

# 6, for each of three pieces and their x and y axis
TOTAL_KEY_LEN = 6 * POS_BIT_LEN 
# Length of codomain that lookup table maps to king bits
TOTAL_VAL_LEN = UDLR_BITS
# rook bits
TOTAL_VAL_LEN += UDLR_BITS + NUM_DIR_MOVES * POS_BIT_LEN
#positions in initial raed table
BKx = 2
BKy = 3
WKx = 4
WKy = 5
WRx = 6
WRy = 7

def main():
    readInputFile()
    convertToBits()

    BEFORE_MINIMIZATION_LENGTH = len(bitLookup)
    print('before minimization lenght = ', BEFORE_MINIMIZATION_LENGTH)
    someKey = next(iter(bitLookup))
    print('Key length = ', len(someKey), 'Value length = ', len(bitLookup[someKey]) )

    GENERATED_PLA = f"original{BOARD_SIZE}x{BOARD_SIZE}.pla"
    print("writing table to pla file...")
    tableToPla(bitLookup, GENERATED_PLA)

    filteredLookup =  filterTable(bitLookup, r"1..1........")
    #printTable(filteredLookup)
    testFormula(filteredLookup)
    #tableToCnf(filteredLookup)
    #MINIMIZED_OUTPUT_FILE = f"minimized{BOARD_SIZE}x{BOARD_SIZE}.pla"
    #print(f"Saved to {MINIMIZED_OUTPUT_FILE}")

    # writeCsv(bitLookup)

    print("Finished.")

    
def readInputFile():
    print ("reading file...")
    INPUT_TABLE_FILE = f'tableGen/chessDict{BOARD_SIZE}x{BOARD_SIZE}.txt' 
    with open(INPUT_TABLE_FILE, 'r') as f:
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
    print('finished!')
    print("Items read = ", len(lookup))



# N E W code
# maps from current game state into optimal move
# state -> move
# key: NUM_BIT_LENGTH * 6, 6 for BKx, BKy, WKx, Wky, Wrx, Wry = 18 bits
# value: up, down left, right each 1 bit for king
#   up, down, left, right each 1 bit for rook + num bits x2 for how many fields (first x then y axis)
# kup, kdwn, kleft, kright, rup, rdwn, rlft, rrght, x3, x2, x1, y3, y2, y1
bitLookup = {}

# returns a list of bools indicating the number
def numberIntoBits(num):
    #return [i == num for i in range(BOARD_SIZE)]
    return bin(num)[2:].zfill(POS_BIT_LEN)

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

def convertToBits():
    print("Converting to bits...")
    for key, val in lookup.items():
        keyBits = numberIntoBits(key[BKx])
        keyBits += numberIntoBits(key[BKy]) 
        keyBits += numberIntoBits(key[WKx]) 
        tmp = numberIntoBits(key[WKy])
        keyBits += tmp 
        keyBits += numberIntoBits(key[WRx]) 
        keyBits += numberIntoBits(key[WRy]) 
        
        valueBits = ''
        isKing = didKingPlay(key, val)

        if isKing:
            valueBits += boolList2BinString( getKingDirection(key, val) )
            # fill with 0s on the right for rook bits
            valueBits = valueBits.ljust(TOTAL_VAL_LEN, '0')
            #print("king valueBits len", len(valueBits))
        else:
            up,down,left,right,distance = getRookDirectionAndDistance(key, val)
            valueBits += boolList2BinString([up,down,left,right])
            distanceBits = numberIntoBits(distance)
            if up or down:
                #print(f"valueBits = {valueBits}")
                #print(f"distanceBits old = {distanceBits}")
                distanceBits = distanceBits.ljust(POS_BIT_LEN * 2, '0')
                #print(f"distanceBits new = {distanceBits}")
            else:
                distanceBits = distanceBits.zfill(POS_BIT_LEN * 2)
            
            valueBits += distanceBits
            # fill with 0s on the left for king bits
            valueBits = valueBits.zfill(TOTAL_VAL_LEN)
            #print("rook valueBits len", len(valueBits))

        bitLookup[keyBits] = valueBits


def printTable(table):
    print("printing table.")
    for key, val in table.items():
        print(key, val)
    print(f"Table has {len(table)} items.")



def filterTable(table, filterString):
    import re
    return  {k:v for k,v in table.items() if re.match(filterString, v)}

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



def writeCsv(table):
    print("writing csv")
    import csv
    fieldnames = ['BKX1', 'BKX2', 'BKY1', 'BKY2', 'WKX1', 'WKX2', 'WKY1', 'WKY2', 'WRX1', 'WRX2', 'WRY1', 'WRY2']
    fieldnames.extend(['KU', 'KD', 'KL', 'KR', 'RU', 'RD', 'RL', 'RR', 'RX1', 'RX2', 'RY1', 'RY2'])

    # newline='' is for windows not to put newlines after each row
    with open(f"original{BOARD_SIZE}x{BOARD_SIZE}.csv", 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

        writer.writeheader()
        for key, val in table.items():
            row = {}
            # write a row, taking items from key first, then from val
            for i, item in enumerate(fieldnames):
                if i < TOTAL_KEY_LEN:
                    row[item] = item if key[i] == '1' else "~" + item
                else:
                    row[item] = item if val[i - TOTAL_KEY_LEN] == '1' else "~" + item
            writer.writerow(row)
    print("finished writing csv")



def analyzeOutput(outputFile):
    numLines = 0
    countDontCare = 0
    with open(outputFile, 'r') as f:
        for _, l in enumerate(f):
            countDontCare += l.count("-")
            numLines += 1
    numSkipLines = 5
    numLines -= numSkipLines
    #print(f"Before minimization length = {BEFORE_MINIMIZATION_LENGTH}")
    print(f"After minimization length = {numLines}, dont cares = {countDontCare}.")
    print(f'total key len = {TOTAL_KEY_LEN}')
    percentDontCare = countDontCare/ (numLines * TOTAL_KEY_LEN)
    print(f"That's {percentDontCare}% dont care bits.")

def tableToCnf(table):
    from sympy import symbols    
    dnf = None
    symbolsArg =  " ".join([str(i) for i in range(TOTAL_KEY_LEN) ])
    symbolsTuple = symbols(symbolsArg)
    symbols = list(symbolsTuple)
    print("calculating dnf")
    for k in table.keys():
        products = None
        for i, s in enumerate(k):
            prod = symbols[i] if s =='1' else ~symbols[i]
            products = prod if products is None else products & prod
        dnf = products if dnf is None else dnf | products
    print("finished dnf")
    #print(dnf)
    s = str(dnf)
    print("dnf str len = ",len(s))
    from sympy.logic.boolalg import to_cnf, simplify_logic
    simplifiedDnf = simplify_logic(dnf)
    s = str(simplifiedDnf)
    print("simplified dnf str len = ",len(s))
    exit()
    print("calculating cnf from dnf")
    cnf = to_cnf(dnf)
    s = str(cnf)
    print("cnf str len = ", len(s))

import string

def testFormula(table):
    f = None
    for k in table.keys():
        f = Formula(bstring=k)
        break
    print(f)

class Formula:
    def __init__(self, fixed = None, merged = [], bstring = None):
        # set of fixed bits. Intended usage: (a, A, b...)
        if bstring is not None:
            # expecting string with only 0s or 1s
            lower = string.ascii_lowercase
            upper = string.ascii_uppercase
            fixed = set()
            for i, chr in enumerate(bstring):
                elem = lower[i] if chr == '0' else upper[i]
                fixed.add(elem)

        self.fixed = set(fixed) if fixed is not None else set()
        # list of merged formulas
        self.merged = merged

    def __eq__(self, other):
        if not isinstance(other, Formula):
            return False
        if self.fixed != other.fixed:
            return False
        if len(self.merged) == 0 and len(other.merged) == 0:
            return True
        for i in self.merged:
            if i not in other.merged:
                return False
        return True


    def similarity(self, other):
        d = len(self.fixed.intersection(other.fixed))
        for i in self.merged:
            if i in other.merged:
                d += i.size()
        return d
                

    def size(self):
        count = len(self.fixed)
        for i in self.merged:
            count += i.size()
        return count
    
    def canMerge(self, other):
        lowerA = [i.lower() for i in self.fixed]
        lowerB = [i.lower() for i in other.fixed]
        inter = set(lowerA).intersection(set(lowerB))
        # they can merge if they have the same fixed bits
        if len(inter) == len(lowerA):
            return True
        # or if they have same merged blocks
        allSame = True
        for i in self.merged:
            if i in other.merged:
                allSame = False
                break
        return allSame

    def merge(self, other):
        # its expected that canMerge(self, other) would return true
        aMinusB = self.fixed - other.fixed
        bMinusA = other.fixed - self.fixed
        inter = self.fixed.intersection(other.fixed)
        a = Formula(aMinusB)
        b = Formula(bMinusA)
        #aAndB = Formula(None, [a, b])
        merged = [a, b]
        merged.extend(self.merged)
        x = Formula(inter, merged)
        return x
    
    def __str__(self):
        srtd = sorted(list(self.fixed), key=str.lower)
        if (len(srtd) == 0 and len(self.merged) == 0):
            return "empty"
        s = ""
        if len(srtd) > 0:
            s = "(" + " ".join(srtd) + ")"
        if len(self.merged) > 0:
            s +=  " [" 
            s += " or ".join(list(map(str, self.merged)))
            s +=  "] "
        return s

# TEST
a = Formula(bstring="111100001111")
b = Formula(bstring="111101001111")
anb = a.merge(b)
print(anb)
x = Formula(bstring="011100001111")
y = Formula(bstring="011101001111")
xny = x.merge(y)
print(xny)
print(anb.merge(xny))


exit()
if __name__ == "__main__":
    main()

