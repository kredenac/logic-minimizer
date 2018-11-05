# preprocessing c table into something more managable
import pyeda
import os
import string
import time
# only white figures movement are of concern
# isWhiteTurn, isRookCaptured, BKx, BKy, WKx, WKy, WRx, WRy
lookup = {}
BOARD_SIZE = 8

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

# size, diet, venom, legs3, legs2 legs1, europe, dangerous 
animals = []
animals.append("1101000") # lion
animals.append("1111000") # komodo
animals.append("0110001") # zmija
animals.append("0011100") # pcela

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
    goIntoScriptDir()
    mergeFormulas(animals, useFaster=True)
    exit()
    readInputFile()
    convertToBits()

    BEFORE_MINIMIZATION_LENGTH = len(bitLookup)
    print('before minimization length = ', BEFORE_MINIMIZATION_LENGTH)
    someKey = next(iter(bitLookup))
    print('Key length = ', len(someKey), 'Value length = ', len(bitLookup[someKey]) )

    #GENERATED_PLA = f"original{BOARD_SIZE}x{BOARD_SIZE}.pla"
    #print("writing table to pla file...")
    #tableToPla(bitLookup, GENERATED_PLA)

    # r"0..0........" is for king going up and right
    filteredLookup =  filterTable(bitLookup, r"1..1..........")
    print("Filtered length = ", len(filteredLookup))
    #printTable(filteredLookup)
    mergeFormulas(filteredLookup, useFaster=True)
    #MINIMIZED_OUTPUT_FILE = f"minimized{BOARD_SIZE}x{BOARD_SIZE}.pla"
    #print(f"Saved to {MINIMIZED_OUTPUT_FILE}")

    # writeCsv(bitLookup)

    print("Finished.")

def goIntoScriptDir():
    abspath = os.path.abspath(__file__)
    dname = os.path.dirname(abspath)
    os.chdir(dname)
    
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
        # list of list of formulas, that is, product of sums of products
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
        for i in other.merged:
            if i not in self.merged:
                return False
        return True


    def similarity(self, other):
        d = len(self.fixed.intersection(other.fixed))
        for i in self.merged:
            if i in other.merged:
                # this probably isn't the best metric,
                # maybe number of products within this lists element
                d += len(i[0].fixed)
        return d

    def simplify(self):
        if len(self.merged) == 0:
            return
        for disj in self.merged[:]:
            conjLen = len(disj[0].fixed)
            # eg. a or A, then remove it 
            if conjLen * 2 == len(disj):
                self.merged.remove(disj)
                continue
            # ab or AB simplify to a eq b
            canSimplify = conjLen == 2 and len(disj) == 2
            if not canSimplify:
                continue
            lst1 = list(disj[0].fixed)
            lst2 = list(disj[1].fixed)
            a1 = lst1[0].islower()
            b1 = lst1[1].islower()
            a2 = lst2[0].islower()
            b2 = lst2[1].islower()
            if a1 != a2 and b1 != b2:
                self.merged.remove(disj)
                disj = [str(lst1[0]) + " eq "  + str(lst1[1])]
                self.merged.append(disj)
            elif a1 != b1 and a2 != b2:
                self.merged.remove(disj)
                disj = [str(lst1[0]) + " xor "  + str(lst1[1])]
                self.merged.append(disj)

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
        if len(inter) == len(lowerA) and len(inter) == len(lowerB):
            return True
        # or if they have same merged blocks
        allSame = True
        for i in self.merged:
            if i not in other.merged:
                allSame = False
                break
        for i in other.merged:
            if i not in self.merged:
                allSame = False
                break
        return allSame

    def merge(self, other):
        # its expected that canMerge(self, other) would return true
        aMinusB = self.fixed - other.fixed
        bMinusA = other.fixed - self.fixed
        inter = self.fixed.intersection(other.fixed)
        merged = []
        if len(aMinusB) != 0:
            merged.append(Formula(aMinusB))
        if len(bMinusA) != 0:
            merged.append(Formula(bMinusA))
        # prevent the list of having formulas instead of list of formulas
        merged = [merged] if len(merged) != 0 else merged
        unique_data = Formula.naiveUniqueJoin(merged, self.merged)
        unique_data = Formula.naiveUniqueJoin(unique_data, other.merged)
        x = Formula(inter, unique_data)
        return x
    
    def __str__(self):
        srtd = sorted(list(self.fixed), key=str.lower)
        if (len(srtd) == 0 and len(self.merged) == 0):
            return "empty"
        s = ""
        if len(srtd) > 0:
            s = "(" + " ".join(srtd) + ")"
        if len(self.merged) > 0:
            #self.merged is a list of lists of Formula
            # [ [a or A] and [b c or b C ] and [D or d]]
            s += " and "
            s += self._listOfListsToStr()
        return s

    # used for self.merged    
    def _listOfListsToStr(self):
        s = "["
        for i, subList in enumerate(self.merged):
            s += "["
            s += " or ".join(list(map(str, subList)))
            s += "]"
            if i != len(self.merged) - 1:
                s += " and "
        s += "]"
        return s

    # for unhashables
    # expecting a,b to be list of lists of formula
    @staticmethod
    def naiveUniqueJoin(a, b):
        res = a
        #!!! if we have [ab or AB] and incoming [aB], then make [ab or aB or AB]
        # each sublist is [ab or Ab...]
        for isublist in b:
            # see if these bits already exist [bits or BiTs or ...]
            whichBits = {bit.lower() for bit in isublist[0].fixed}
            foundMatch = False # should add whole isublist to result?
            for jsublist in res:
                alreadyBits = {bit.lower() for bit in jsublist[0].fixed}
                # if this is that group of bits
                if alreadyBits == whichBits:
                    foundMatch = True
                    # append those that are not already in it
                    # When using only 'if', put 'for' in the beginning
                    jsublist.extend([f for f in isublist if f not in jsublist ])

            if not foundMatch:
                res.append(isublist)
        return res
        
def TestFormula():
    print("TEST start")
    a = Formula(bstring="111100001111")
    b = Formula(bstring="111101001111")
    anb = a.merge(b)
    print(anb)
    x = Formula(bstring="011100001111")
    y = Formula(bstring="011101001111")
    xny = x.merge(y)
    print(xny)
    anb_n_xny= anb.merge(xny)
    print(anb_n_xny)
    twoa = Formula(bstring="111100001111")
    twob = Formula(bstring="111101101111")
    treci = Formula(bstring="111101001111")
    ttab = twoa.merge(twob)
    print(ttab)
    print(treci)
    print(ttab.canMerge(treci), ttab.merge(treci))
    twoc = Formula(bstring="101100001111")
    twod = Formula(bstring="101101101111")
    ttcd = twoc.merge(twod)
    print("###")
    print(ttab)
    print(ttcd)
    print(ttcd.merge(ttab ) )
    print("###")
    print("TEST end")

def mergeFormulas(table, useFaster):
    tableList = []
    if isinstance(table, dict):
        # make a list of Formulas from table
        for k in table.keys():
            f = Formula(bstring=k)
            tableList.append(f)
    else:
        tableList = [Formula(bstring=row) for row in table]

    method = onePairingIteration if useFaster else onlyPairBestOnes
    oldLength = len(tableList)
    # while list is getting smaller
    iteration = 0
    start = time.time()
    while True:
        iteration += 1
        print("iter",iteration)
        oldSize = len(tableList)
        tableList = method(tableList)
        #tableList = onlyPairBestOnes(tableList)
        newSize = len(tableList)
        if iteration == 3:
            pass
            #break # tmp
        if newSize == oldSize:
            break

    end = time.time()
    for i in tableList:
        #continue
        i.simplify()
        print(i)
    print("List len before = ", oldLength, "List len after merging = ", len(tableList))
    print("Time elpassed = ", end - start)


def onePairingIteration(tableList):
    # one iteration of pairing up    
    # pair up two closest 'rows'
    length = len(tableList)
    for i in range(len(tableList) - 1):
        # searching for max similarity
        bestCost = 0
        index = None
        for j in range(i + 1, len(tableList)):
            canMerge = tableList[i].canMerge(tableList[j])
            if canMerge is False:
                continue 
            similarity = tableList[i].similarity(tableList[j])
            if similarity > bestCost:
                index = j
                bestCost = similarity
        if index is not None:
            paired = tableList[i].merge(tableList[index])
            # store it instead of i, and delete j
            tableList[i] = paired
            if index != len(tableList) - 1:
                tableList[index] = tableList.pop()
            else:
                tableList.pop()
    print("List len before = ", length, "List len after merging = ", len(tableList))
    return tableList

def onlyPairBestOnes(tableList):
    totalBestCost = 0
    indexi = None
    indexj = None
    length = len(tableList)
    for i in range(length - 1):
        for j in range(i + 1, length):
            canMerge = tableList[i].canMerge(tableList[j])
            if canMerge is False:
                continue
            similarity = tableList[i].similarity(tableList[j])
            if similarity > totalBestCost:
                indexi = i
                indexj = j
                totalBestCost = similarity
    if indexi is not None:
        paired = tableList[indexi].merge(tableList[indexj])
        # store it instead of i, and delete j
        tableList[indexi] = paired
        if indexj != len(tableList) - 1:
            tableList[indexj] = tableList.pop()
        else:
            tableList.pop()
    return tableList
    

if __name__ == "__main__":
    main()
