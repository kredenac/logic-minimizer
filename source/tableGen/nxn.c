
/* ***************************************************************************** */
/* ***************************************************************************** */
/* ********************* Algorithms for Chess KRK ending *********************** */
/* ***************************************************************************** */
/* ***************************************************************************** */

/* **** Written by Predrag Janicic, Filip Maric, and Marko Malikovic (2016) **** */

/* ***************************************************************************** */
/* For convenience, a KRK KRKPosition for 8x8 is represented as a sequence of 20 bits:
   ----------------------------------------------------------
   |19|18|1BOARD_DIM-1|16|15|14|13|12|11|10| 9| 8| BOARD_DIM-1| 6| 5| 4| 3| 2| 1| 0|
   ----------------------------------------------------------
   |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
   ----------------------------------------------------------
   |bW|bC|  nWRy  |  nWRx  |  nBKy  |  nBKx  |  nWKy  |  nWKx  |    
   ----------------------------------------------------------  */
/* ***************************************************************************** */

// WORKS FOR BOARDS UP TO 256x256

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

unsigned FILES;
unsigned RANKS;
unsigned FILES_BITS;
unsigned RANKS_BITS;
unsigned FILES_MASK;
unsigned RANKS_MASK;

unsigned long long MAX_POS;
const char outputFIleName[] = "chessDict.txt";

/* KRKPosition (coordinates of three pieces) */
typedef struct pos {
  bool bWhiteOnTurn;
  bool bRookCaptured;        
  unsigned char WKx, WKy, WRx, WRy, BKx, BKy;
} KRKPosition;


typedef unsigned long long BVKRKPosition; 


/* Stategy steps */
enum _eStratStep { 
    eImmediateMate,
    eReadyToMate,
    eSqueeze,
    eApproachDiag,
    eApproachNonDiag,
    eKeepRoomDiag,
    eKeepRoomNonDiag,
    eRookHome,
    eRookSafe,
    eRookSafeSmallBoards,
    eStepMax
};

typedef enum _eStratStep eStratStep;
char* SStep[eStepMax];

/* Outcomes for positons */
enum _eOutcomes { 
    DRAW     = 1000,
    UNKNOWN  = 1002,
    ILLEGAL  = 1003,
    UNDEF    = 1004
};


/* A record in the lookup table */
typedef struct lur {
    BVKRKPosition Pos, OptimalPos, OptimalDist, StrategPos, StrategDist; 
    eStratStep step;
} LookUpRecord;


/* Lookup table */
LookUpRecord* LookUpTable;

bool _B_INITIALIZATION_DONE;
bool _B_SETTING_STEPS_DONE;

// Declarations  

// Miscellanous 
void PrintTable(LookUpRecord * lur, bool shouldPrint, bool isOptimal);
void Print(KRKPosition p);
BVKRKPosition Position2Bitvector(KRKPosition p);
void Bitvector2Position(BVKRKPosition i,KRKPosition *p);

unsigned MinusPA (unsigned x, unsigned y);
unsigned AbsDiff(unsigned x, unsigned y);
unsigned ChebyshevDistance(unsigned x1,unsigned y1,unsigned x2,unsigned y2);
unsigned ManhattanDistance(unsigned x1,unsigned y1,unsigned x2,unsigned y2);
bool LexOrdering2leq(unsigned a,unsigned b,unsigned c,unsigned d);
bool LexOrdering3leq(unsigned a,unsigned b,unsigned c,unsigned d,unsigned e,unsigned f);
bool CanonicalX(KRKPosition p);
bool CanonicalY(KRKPosition p);
bool CanonicalD(KRKPosition p);

KRKPosition ReflectX(KRKPosition p);
KRKPosition ReflectY(KRKPosition p);
KRKPosition ReflectD(KRKPosition p);

KRKPosition Symmetric(unsigned i, KRKPosition p);

// Chess rules 
bool Dim(KRKPosition p);
bool RookCaptured(KRKPosition p);
bool NotKingNextKing (KRKPosition p);
bool TwoPiecesOnSameSquare (KRKPosition p);
bool WRAttacksBK (KRKPosition p);

// Legal moves definitions
bool LegalPositionWhiteToMove(KRKPosition p) ;
bool LegalPositionBlackToMove(KRKPosition p) ;
bool WhiteKingMoves(KRKPosition p1,KRKPosition p2);
bool WhiteRookMoves(KRKPosition p1,KRKPosition p2);
bool LegalWhiteMove(KRKPosition p1, KRKPosition p2) ;
bool LegalBlackMove(KRKPosition p1, KRKPosition p2) ;
bool NextLegalWhiteMove(unsigned i,KRKPosition p1,KRKPosition *p2) ;
bool NextLegalBlackMove(unsigned i,KRKPosition p1,KRKPosition *p2) ;

// Mate and Stalemate
bool Mate(KRKPosition p);
bool StalemateOneCase(KRKPosition p);
bool Stalemate(KRKPosition p);

// Definitions for Strategy
unsigned Room (KRKPosition p);
bool NewRoomSmaller (KRKPosition p1,KRKPosition p2);
bool NotBlackKingNextRook (KRKPosition p) ;
bool WhiteKingNextRook (KRKPosition p) ;
unsigned DistanceWhiteKingToCS (KRKPosition p);
unsigned DistanceWhiteKingToRook (KRKPosition p);
unsigned DistanceBlackKingToRook (KRKPosition p);
unsigned DistanceWhiteKingToBlackKing (KRKPosition p);
bool NotWhiteRookExposed (KRKPosition p);
bool WhiteRookDivides (KRKPosition p);
unsigned CSx(KRKPosition p);
unsigned CSy(KRKPosition p);
bool ApproachCriticalSquare (KRKPosition p1,KRKPosition p2);
bool WhiteKingOnEdge(KRKPosition p);
bool LPattern(KRKPosition p);
bool WhiteKingAndRookNotDiverging (KRKPosition p1,KRKPosition p2);
bool BackMove(KRKPosition p1,KRKPosition p2);

// Strategy (for White)
bool ImmediateMate(KRKPosition p1,KRKPosition p2);
bool ReadyToMate(KRKPosition p1,KRKPosition p2);
bool SqueezeCond(KRKPosition p1,KRKPosition p2);
bool ApproachDiagCond(KRKPosition p1,KRKPosition p2);
bool ApproachNonDiagCond(KRKPosition p1,KRKPosition p2);
bool KeepRoomDiagCond(KRKPosition p1,KRKPosition p2);
bool KeepRoomNonDiagCond(KRKPosition p1,KRKPosition p2);
bool RookHomeCond(KRKPosition p1,KRKPosition p2);
bool RookSafeCond(KRKPosition p1,KRKPosition p2);
bool RookSafeSmallBoardsCond(KRKPosition p1,KRKPosition p2);
KRKPosition Strategy(KRKPosition p1, eStratStep *s);
KRKPosition StrategyM(KRKPosition p1, eStratStep *s);

unsigned Measure(KRKPosition p);

/* ******************************************************************** */
/* Miscellanous                                                         */ 
/* ******************************************************************** */ 

/* Compressing and uncompressing procedures */
BVKRKPosition Position2Bitvector(KRKPosition p)  {
   BVKRKPosition i=0;
   i |= p.bWhiteOnTurn;
   i <<= 1;
   i |= p.bRookCaptured;
   i <<= RANKS_BITS;
   i |= p.WRy;
   i <<= FILES_BITS;
   i |= p.WRx;
   i <<= RANKS_BITS;
   i |= p.BKy;
   i <<= FILES_BITS;
   i |= p.BKx;
   i <<= RANKS_BITS;
   i |= p.WKy;
   i <<= FILES_BITS;
   i |= p.WKx;
   return i;
}

/* -------------------------------------------------------------------------------- */

void Bitvector2Position(BVKRKPosition i,KRKPosition *p)  {
   p->WKx = i & FILES_MASK;
   i >>= FILES_BITS;
   p->WKy = i & RANKS_MASK;   
   i >>= RANKS_BITS;
   p->BKx = i & FILES_MASK;
   i >>= FILES_BITS;
   p->BKy = i & RANKS_MASK;
   i >>= RANKS_BITS;
   p->WRx = i & FILES_MASK;
   i >>= FILES_BITS;
   p->WRy = i & RANKS_MASK;
   i >>= RANKS_BITS;
   p->bRookCaptured = i & 1;
   i >>= 1;
   p->bWhiteOnTurn = i & 1;
}


/* -------------------------------------------------------------------------------- */

bool IsRookCaptured(KRKPosition p) {
/*    return (p.WRx==p.BKx && p.WRy==p.BKy);*/
    return p.bRookCaptured;
}

/* -------------------------------------------------------------------------------- */

bool IsWhiteOnTurn(KRKPosition p) {
/*    return (p.WRx==p.BKx && p.WRy==p.BKy);*/
    return p.bWhiteOnTurn;
}


/* -------------------------------------------------------------------------------- */

void Print(KRKPosition p) {
    unsigned x,y;
    if (p.bRookCaptured) 
        printf("Rook captured, ");
    if (p.bWhiteOnTurn)
        printf("White on turn, ");
    else
        printf("Black on turn\n");

    for(y=RANKS; y>0; y--) {
        for(x=0;x<FILES;x++)  {
            if(p.WKx==x && p.WKy==y-1)
                printf("|K");
            else if (p.BKx==x && p.BKy==y-1)
                printf("|k");
            else if (p.WRx==x && p.WRy==y-1)
                printf("|R");
            else 
                printf("| ");
        }
        printf("| \n");
    }
    printf("\n");
}


/* -------------------------------------------------------------------------------- */

unsigned MinusPA (unsigned x, unsigned y) {
    return (x<=y ? 0 : x-y);
}

/* -------------------------------------------------------------------------------- */

unsigned AbsDiff(unsigned x, unsigned y) {
    return MinusPA(x,y)+MinusPA(y,x);
}

/* -------------------------------------------------------------------------------- */

unsigned ChebyshevDistance(unsigned x1,unsigned y1,unsigned x2,unsigned y2) {
    return (MinusPA(AbsDiff(y1,y2),AbsDiff(x1,x2))==0 ? AbsDiff(x1,x2) : AbsDiff(y1,y2)); 
}

/* -------------------------------------------------------------------------------- */

unsigned ManhattanDistance(unsigned x1,unsigned y1,unsigned x2,unsigned y2) {
    return AbsDiff(x1,x2)+AbsDiff(y1,y2);
}

/* -------------------------------------------------------------------------------- */

bool LexOrdering2leq(unsigned a,unsigned b,unsigned c,unsigned d) {
   /* returns (a,b) <=lex (c,d) */
   if (a<c) 
     return true;
   else if (a==c) 
     return (b<=d);
   else 
     return false;
}


/* -------------------------------------------------------------------------------- */

bool LexOrdering3leq(unsigned a,unsigned b,unsigned c,unsigned d,unsigned e,unsigned f)  {
   /* returns (a,b,c) <=lex (d,e,f) */
   if (a<d) 
     return true;
   if (a==d) {
     if (b<e) 
        return true;
     if (b==e) 
        return (c<=f);
   }
   return false;
}


/* -------------------------------------------------------------------------------- */


bool CanonicalX(KRKPosition p) {
   if (IsRookCaptured(p))
      return LexOrdering2leq(2*p.BKx+1, 2*p.WKx+1, FILES, FILES);
   else 
      return LexOrdering3leq(2*p.BKx+1, 2*p.WKx+1, 2*p.WRx+1, FILES, FILES, FILES);
}


/* -------------------------------------------------------------------------------- */

bool CanonicalY(KRKPosition p) {
   if (IsRookCaptured(p)) 
      return LexOrdering2leq(2*p.BKy+1, 2*p.WKy+1, RANKS, RANKS);
   else 
      return LexOrdering3leq(2*p.BKy+1, 2*p.WKy+1, 2*p.WRy+1, RANKS, RANKS, RANKS);
}


/* -------------------------------------------------------------------------------- */

bool CanonicalD(KRKPosition p) {
   if (IsRookCaptured(p))
      return LexOrdering2leq(p.BKx, p.WKx,p.BKy, p.WKy);
   else 
      return LexOrdering3leq(p.BKx, p.WKx, p.WRx, p.BKy, p.WKy, p.WRy);
}


/* -------------------------------------------------------------------------------- */

KRKPosition ReflectX(KRKPosition p) {
   if (CanonicalX(p))
     return p;
   p.WKx = FILES-1-p.WKx;
   p.WRx = FILES-1-p.WRx;
   p.BKx = FILES-1-p.BKx;
   return p;
}

/* -------------------------------------------------------------------------------- */

KRKPosition ReflectY(KRKPosition p) {
   if (CanonicalY(p)) 
     return p;
   p.WKy = RANKS-1-p.WKy;
   p.WRy = RANKS-1-p.WRy;
   p.BKy = RANKS-1-p.BKy;
   return p;
}

/* -------------------------------------------------------------------------------- */

KRKPosition ReflectD(KRKPosition p) {
   if (CanonicalD(p)) {
    return p;
}
   unsigned tmp;
   tmp   = p.WKx;
   p.WKx = p.WKy;
   p.WKy = tmp;
   tmp   = p.WRx;
   p.WRx = p.WRy;
   p.WRy = tmp;
   tmp   = p.BKx;
   p.BKx = p.BKy;
   p.BKy = tmp;
   return p;
}

/* -------------------------------------------------------------------------------- */

KRKPosition Symmetric(unsigned i, KRKPosition p) {
    KRKPosition pp=p;
    switch(i) {
      case 0: break;
      case 1: pp.WKx=FILES-1-p.WKx;      pp.WRx=FILES-1-p.WRx;  pp.BKx=FILES-1-p.BKx; break;
      case 2: pp.WKy=RANKS-1-p.WKy;      pp.WRy=RANKS-1-p.WRy;  pp.BKy=RANKS-1-p.BKy; break;
      case 3: pp.WKx=FILES-1-p.WKx;      pp.WRx=FILES-1-p.WRx;  pp.BKx=FILES-1-p.BKx;
              pp.WKy=RANKS-1-p.WKy;      pp.WRy=RANKS-1-p.WRy;  pp.BKy=RANKS-1-p.BKy; break;

      case 4: pp.WKx=p.WKy;              pp.WRx=p.WRy;          pp.BKx=p.BKy;
              pp.WKy=p.WKx;              pp.WRy=p.WRx;          pp.BKy=p.BKx;         break;
      case 5: pp.WKx=RANKS-1-p.WKy;      pp.WRx=RANKS-1-p.WRy;  pp.BKx=RANKS-1-p.BKy;
              pp.WKy=p.WKx;              pp.WRy=p.WRx;          pp.BKy=p.BKx;         break;
      case 6: pp.WKx=p.WKy;              pp.WRx=p.WRy;          pp.BKx=p.BKy;
              pp.WKy=FILES-1-p.WKx;      pp.WRy=FILES-1-p.WRx;  pp.BKy=FILES-1-p.BKx; break;
      case 7: pp.WKx=RANKS-1-p.WKy;      pp.WRx=RANKS-1-p.WRy;  pp.BKx=RANKS-1-p.BKy;
              pp.WKy=FILES-1-p.WKx;      pp.WRy=FILES-1-p.WRx;  pp.BKy=FILES-1-p.BKx; break;
      default: break;
    }
    return pp;
}


/* ******************************************************************** */
/* Chess rules for legal KRKPositions                                      */      
/* ******************************************************************** */

bool Dim(KRKPosition p) {
    return (p.WKx<=FILES-1 && p.WKy<=RANKS-1 && 
            p.BKx<=FILES-1 && p.BKy<=RANKS-1 &&
            p.WRx<=FILES-1 && p.WRy<=RANKS-1);
}

/* -------------------------------------------------------------------------------- */

bool TwoPiecesOnSameSquare (KRKPosition p) {
    return ((p.WKx==p.WRx && p.WKy==p.WRy) || (p.BKx==p.WRx && p.BKy==p.WRy)) && !p.bRookCaptured ;
}

/* -------------------------------------------------------------------------------- */

bool NotKingNextKing (KRKPosition p) {
    return (p.WKx>p.BKx+1 || p.BKx>p.WKx+1 || p.WKy>p.BKy+1 || p.BKy>p.WKy+1);
}

/* -------------------------------------------------------------------------------- */

bool WRAttacksBK (KRKPosition p) {
    if (IsRookCaptured(p)) return false;
    return (p.WRx==p.BKx && (p.WKx!=p.WRx || p.WKx==p.WRx && (p.WKy<=p.BKy && p.WKy<=p.WRy || p.BKy<=p.WKy && p.WRy<=p.WKy)) ||
            p.WRy==p.BKy && (p.WKy!=p.WRy || p.WKy==p.WRy && (p.WKx<=p.BKx && p.WKx<=p.WRx || p.BKx<=p.WKx && p.WRx<=p.WKx)));
}

/* -------------------------------------------------------------------------------- */

bool LegalPositionWhiteToMove(KRKPosition p)  {
    return Dim(p) && NotKingNextKing(p) &&  !TwoPiecesOnSameSquare(p) && !WRAttacksBK(p) && IsWhiteOnTurn(p);
}

/* -------------------------------------------------------------------------------- */

bool LegalPositionBlackToMove(KRKPosition p)  {
    return Dim(p) && NotKingNextKing(p) &&  !TwoPiecesOnSameSquare(p) && !IsWhiteOnTurn(p);
}

/* ******************************************************************** */
/* Chess rules for legal moves                                          */      
/* ******************************************************************** */

bool WhiteKingMoves(KRKPosition p1,KRKPosition p2) {
    return (p1.WKx!=p2.WKx || p1.WKy!=p2.WKy);
}

/* -------------------------------------------------------------------------------- */

bool WhiteRookMoves(KRKPosition p1,KRKPosition p2) {
    return (p1.WRx!=p2.WRx || p1.WRy!=p2.WRy);
}

/* -------------------------------------------------------------------------------- */

bool LegalWhiteMove(KRKPosition p1, KRKPosition p2)  {
    if (!p1.bWhiteOnTurn) 
        return false;

    if (p1.BKx!=p2.BKx || p1.BKy!=p2.BKy) return false;
    
    if (!IsRookCaptured(p1) && p1.WKx==p2.WKx && p1.WKy==p2.WKy) { // Rook moves
        if (p1.WRx==p2.WRx && p1.WRy==p2.WRy) return false;     
        if (p1.WRx!=p2.WRx && p1.WRy!=p2.WRy) return false;
        if (p1.WRx>p2.WRx) {
            if (p1.WKy==p1.WRy && p1.WRx>p1.WKx && p1.WKx>p2.WRx) return false;
            if (p1.BKy==p1.WRy && p1.WRx>p1.BKx && p1.BKx>p2.WRx) return false; 
        }
        if (p1.WRx<p2.WRx) {
            if (p1.WKy==p1.WRy && p1.WRx<p1.WKx && p1.WKx<p2.WRx) return false; 
            if (p1.BKy==p1.WRy && p1.WRx<p1.BKx && p1.BKx<p2.WRx) return false;
        }
        if (p1.WRy>p2.WRy) {
            if (p1.WKx==p1.WRx && p1.WRy>p1.WKy && p1.WKy>p2.WRy) return false; 
            if (p1.BKx==p1.WRx && p1.WRy>p1.BKy && p1.BKy>p2.WRy) return false; 
        }
        if (p1.WRy<p2.WRy) {
            if (p1.WKx==p1.WRx && p1.WRy<p1.WKy && p1.WKy<p2.WRy) return false; 
            if (p1.BKx==p1.WRx && p1.WRy<p1.BKy && p1.BKy<p2.WRy) return false; 
        }
    }
    else { // King moves
        if (p1.WKx==p2.WKx && p1.WKy==p2.WKy) return false;     
        if (p1.WRx!=p2.WRx || p1.WRy!=p2.WRy) return false;
        if (p1.WKx>p2.WKx+1 || p2.WKx>p1.WKx+1 ||
            p1.WKy>p2.WKy+1 || p2.WKy>p1.WKy+1) return false;
    }
    return LegalPositionBlackToMove(p2);
}

/* -------------------------------------------------------------------------------- */

bool LegalBlackMove(KRKPosition p1, KRKPosition p2)  {
    if (p1.bWhiteOnTurn) 
        return false;
    if (p1.WKx!=p2.WKx || p1.WKy!=p2.WKy) 
        return false;
    if (!IsRookCaptured(p2) && (p1.WRx!=p2.WRx || p1.WRy!=p2.WRy)) 
        return false;
    if (p1.BKx==p2.BKx && p1.BKy==p2.BKy) 
        return false;       
    if (p1.BKx>p2.BKx+1 || p2.BKx>p1.BKx+1 || p1.BKy>p2.BKy+1 || p2.BKy>p1.BKy+1)   
        return false;
    return LegalPositionWhiteToMove(p2);
}

/* -------------------------------------------------------------------------------- */

bool NextLegalWhiteMove(unsigned i,KRKPosition p1,KRKPosition *p2)  {
    if (!p1.bWhiteOnTurn)
       return false;
    *p2 = p1;
    switch(i) {
        case 0: p2->WKx = p2->WKx+1; break;
        case 1: p2->WKx = p2->WKx+1; p2->WKy = p2->WKy+1; break;
        case 2:                      p2->WKy = p2->WKy+1; break;
        case 3: p2->WKx = p2->WKx-1; p2->WKy = p2->WKy+1; break;
        case 4: p2->WKx = p2->WKx-1; break;
        case 5: p2->WKx = p2->WKx-1; p2->WKy = p2->WKy-1; break;
        case 6:                      p2->WKy = p2->WKy-1; break;
        case 7: p2->WKx = p2->WKx+1; p2->WKy = p2->WKy-1; break;

        default: 
               if(i>=8 && i<=8+FILES-1) {
                  if (!IsRookCaptured(p1)) 
                    p2->WRx = i-8; 
                  else 
                   return false;

               }
               else if (i>=8+FILES && i<=8+FILES+RANKS-1) {
                  if (!IsRookCaptured(p1)) 
                    p2->WRy = i-8-FILES; 
                 else 
                   return false;
               }
               else 
                 return false;
    }
    p2->bWhiteOnTurn  = !p1.bWhiteOnTurn;    
    p2->bRookCaptured = p1.bRookCaptured;
    return LegalWhiteMove(p1, *p2);
}

/* -------------------------------------------------------------------------------- */

bool NextLegalBlackMove(unsigned i,KRKPosition p1,KRKPosition *p2)  {
    if (p1.bWhiteOnTurn)
       return false;
    *p2 = p1;
    switch(i) {
        case 0: p2->BKx = p2->BKx+1; break;
        case 1: p2->BKx = p2->BKx+1; p2->BKy = p2->BKy+1; break;
        case 2:                      p2->BKy = p2->BKy+1; break;
        case 3: p2->BKx = p2->BKx-1; p2->BKy = p2->BKy+1; break;
        case 4: p2->BKx = p2->BKx-1; break;
        case 5: p2->BKx = p2->BKx-1; p2->BKy = p2->BKy-1; break;
        case 6:                      p2->BKy = p2->BKy-1; break;
        case 7: p2->BKx = p2->BKx+1; p2->BKy = p2->BKy-1; break;
        default: return false;
    }
    p2->bWhiteOnTurn  = !p1.bWhiteOnTurn ;    
    p2->bRookCaptured = p1.bRookCaptured;
    if (p2->BKx==p1.WRx && p2->BKy==p1.WRy)  
       p2->bRookCaptured = true;
 
    return LegalBlackMove(p1, *p2);
}


/* -------------------------------------------------------------------------------- */

bool BlackCannotMove(KRKPosition p) {
    KRKPosition p1;
    for(unsigned i=0; i<8; i++)
       if (NextLegalBlackMove(i,p,&p1)) 
         return false;
    return true;
}

/* ******************************************************************** */ 
/* Mate, Stalemate, Draw                                                */
/* ******************************************************************** */

bool Mate(KRKPosition p) {
//    return !IsRookCaptured(p) && BlackCannotMove(p) && WRAttacksBK(p);
    return LegalPositionBlackToMove(p) && BlackCannotMove(p) && WRAttacksBK(p);
}

/* -------------------------------------------------------------------------------- */


bool StalemateOneCase(KRKPosition p) {
    return (p.BKx==0 && p.BKy==0 && p.WRx==1 && p.WRy==1 && p.WKx==2 && p.WKy<3 ||
            p.BKx==0 && p.BKy==0 && p.WRx>0 && p.WRy==1 && p.WKx==2 && p.WKy==0);
}

/* -------------------------------------------------------------------------------- */

bool Stalemate(KRKPosition p) {
    unsigned i;
    if(IsRookCaptured(p)) 
        return false;

    for(i=0;i<8;i++)
        if(StalemateOneCase(Symmetric(i,p)))
            return true;
    return false;
}

/* ******************************************************************** */
/* Definitions for Strategy                                             */
/* ******************************************************************** */

unsigned Room (KRKPosition p) {
    if (AbsDiff(p.WRx,p.BKx)==0)
        return FILES+RANKS-1;
    else {
        if (AbsDiff(p.WRy,p.BKy)==0)
            return FILES+RANKS-1;
        else {
            if (MinusPA(p.BKx,p.WRx)==0) {
                if (MinusPA(p.BKy,p.WRy)==0)
                    return p.WRx+p.WRy;
                else 
                    return p.WRx+(RANKS-1-p.WRy);
            }
            else {
                if (MinusPA(p.BKy,p.WRy)==0)
                    return (FILES-1-p.WRx)+p.WRy;
                else 
                    return (FILES-1-p.WRx)+(RANKS-1-p.WRy);
            }
        }
    }
}


/* -------------------------------------------------------------------------------- */

bool NewRoomSmaller (KRKPosition p1,KRKPosition p2) {
    return (Room(p1)>Room(p2));
}

/* -------------------------------------------------------------------------------- */

bool NotBlackKingNextRook (KRKPosition p)  {
    return (p.WRx>p.BKx+ 1 || p.BKx>p.WRx+1 || p.WRy>p.BKy+1 || p.BKy>p.WRy+1);
}

/* -------------------------------------------------------------------------------- */

bool WhiteKingNextRook (KRKPosition p)  {
    return (p.WRx<=p.WKx+1 && p.WKx<=p.WRx+1 && p.WRy<=p.WKy+1 && p.WKy<=p.WRy+1);
}

/* -------------------------------------------------------------------------------- */

unsigned DistanceWhiteKingToCS (KRKPosition p) {
    return ChebyshevDistance(p.WKx,p.WKy,CSx(p),CSy(p));
}

/* -------------------------------------------------------------------------------- */

unsigned DistanceWhiteKingToRook (KRKPosition p) {
    return ChebyshevDistance(p.WKx,p.WKy,p.WRx,p.WRy);
}

/* -------------------------------------------------------------------------------- */

unsigned DistanceBlackKingToRook (KRKPosition p) {
    return ChebyshevDistance(p.BKx,p.BKy,p.WRx,p.WRy);
}

/* -------------------------------------------------------------------------------- */

unsigned DistanceWhiteKingToBlackKing (KRKPosition p) {
    return ChebyshevDistance(p.WKx,p.WKy,p.BKx,p.BKy);
}

/* -------------------------------------------------------------------------------- */

bool NotWhiteRookExposed (KRKPosition p) {
    return (DistanceWhiteKingToRook(p)<=DistanceBlackKingToRook(p));
}

/* -------------------------------------------------------------------------------- */

bool WhiteRookDivides (KRKPosition p) {
    return (p.WKx>p.WRx && p.WRx>p.BKx || p.WKx<p.WRx && p.WRx<p.BKx || 
            p.WKy>p.WRy && p.WRy>p.BKy || p.WKy<p.WRy && p.WRy<p.BKy);
}

/* -------------------------------------------------------------------------------- */

unsigned CSx(KRKPosition p) {
    if (AbsDiff(p.BKx,p.WRx)== 0) 
        return p.WRx;
    else {
        if (MinusPA(p.BKx,p.WRx) == 0) 
            return p.WRx-1;
        else 
            return p.WRx+1;
    }
}

/* -------------------------------------------------------------------------------- */

unsigned CSy(KRKPosition p) {
    if (AbsDiff(p.BKy,p.WRy) == 0) 
        return p.WRy;
    else {
        if (MinusPA(p.BKy,p.WRy) == 0) 
            return p.WRy-1;
        else 
            return p.WRy+1;
    }
}

/* -------------------------------------------------------------------------------- */

bool ApproachCriticalSquare (KRKPosition p1,KRKPosition p2) {
    unsigned d1=ManhattanDistance(p1.WKx,p1.WKy,CSx(p1),CSy(p1));
    unsigned d2=ManhattanDistance(p2.WKx,p2.WKy,CSx(p2),CSy(p2));
    return (d1>d2);
}

/* -------------------------------------------------------------------------------- */

bool WhiteKingOnEdge(KRKPosition p) {
    return (p.WKx==0 || p.WKx==FILES-1 || p.WKy==0 || p.WKy==RANKS-1);
}

/* -------------------------------------------------------------------------------- */

bool LPattern(KRKPosition p) {
    return (p.WKy==p.BKy && (p.WKx==p.BKx+2 || p.BKx==p.WKx+2) && 
            p.WRx==p.WKx && (p.WRy==p.WKy+1 || p.WKy==p.WRy+1)||
            p.WKx==p.BKx && (p.WKy==p.BKy+2 || p.BKy==p.WKy+2) && 
            p.WRy==p.WKy && (p.WRx==p.WKx+1 || p.WKx==p.WRx+1));
}

/* -------------------------------------------------------------------------------- */

bool WhiteKingAndRookNotDiverging (KRKPosition p1,KRKPosition p2) {
    return (DistanceWhiteKingToRook(p1) >= DistanceWhiteKingToRook(p2));
}

/* -------------------------------------------------------------------------------- */


bool BackMove(KRKPosition p1,KRKPosition p2) {
    return     
    ((p1.BKx == 0       && p1.WRx == 1       && p2.WKx < p1.WKx) ||
     (p1.BKx == FILES-1 && p1.WRx == FILES-2 && p2.WKx > p1.WKx) ||
     (p1.BKy == 0       && p1.WRy == 1       && p2.WKy < p1.WKy) ||
     (p1.BKy == RANKS-1 && p1.WRy == RANKS-2 && p2.WKy > p1.WKy));
}


/* -------------------------------------------------------------------------------- */


bool KingsOnSameEdge(KRKPosition p) {
    return (p.WKx==0 && p.BKx==0) || (p.WKy==0 && p.BKy==0) || 
           (p.WKx==FILES-1 && p.BKx==FILES-1) || (p.WKy==RANKS-1 && p.BKy==RANKS-1);    
}



/* ******************************************************************** */
/* Strategy (for White)                                                 */
/* ******************************************************************** */


bool ImmediateMateCond(KRKPosition p1,KRKPosition p2) {
    unsigned i;
    KRKPosition p3;
    if(Stalemate(p2))
        return false;
    return Mate(p2);
}

/* -------------------------------------------------------------------------------- */


bool ReadyToMateCond(KRKPosition p1,KRKPosition p2) {
    KRKPosition pp2;
    if(Stalemate(p2)) 
        return false; 

    pp2=ReflectD(ReflectY(ReflectX(p2)));
    return 
       ((pp2.BKx == 0 && pp2.BKy == 0 && pp2.WKx == 1 && pp2.WKy == 2 && pp2.WRx > 2 && pp2.WRy > 0) ||
        (pp2.BKx == 0 && pp2.BKy == 1 && pp2.WKx == 2 && pp2.WKy == 1 && pp2.WRx >= 1 && pp2.WRy == 2) ||
        (pp2.BKx == 0 && pp2.WKx == 2 && pp2.WRx >= 2 && AbsDiff(pp2.WKy,pp2.BKy)==1 && AbsDiff(pp2.WRy,pp2.BKy)==1 && pp2.WKy!=pp2.WRy));
}


/* -------------------------------------------------------------------------------- */

bool SqueezeCond(KRKPosition p1,KRKPosition p2) {
    if(Stalemate(p2)) 
        return false;
    if(!WhiteRookMoves(p1,p2))
        return false;

    // non-maximal squeeze
    // return (NewRoomSmaller(p1,p2) && NotWhiteRookExposed(p2) && WhiteRookDivides(p2));

    // maximal squeeze
    if (!(NewRoomSmaller(p1,p2) && NotWhiteRookExposed(p2) && WhiteRookDivides(p2)))
        return false;

    KRKPosition p3;
    for(unsigned j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p3) && !Stalemate(p3) && WhiteRookMoves(p1,p3))  
            if (NewRoomSmaller(p2,p3) && NotWhiteRookExposed(p3) && WhiteRookDivides(p3))
                return false;
    return true;
}

/* -------------------------------------------------------------------------------- */

bool ApproachDiagCond(KRKPosition p1,KRKPosition p2) {
    if(Stalemate(p2))
        return false;
    if(!WhiteKingMoves(p1,p2))
        return false;
    if(p2.WKx==p1.WKx || p2.WKy==p1.WKy)
        return false;
    return (ApproachCriticalSquare(p1,p2) &&
            NotWhiteRookExposed(p2) &&
            (WhiteRookDivides(p2) || LPattern(p2)) &&
            (Room(p2) > 3 || (!KingsOnSameEdge(p2) && (DistanceWhiteKingToRook(p1)!=1 || !BackMove(p1,p2)))));
}


/* -------------------------------------------------------------------------------- */

bool ApproachNonDiagCond(KRKPosition p1,KRKPosition p2) {
    if(Stalemate(p2))
        return false;
    if(!WhiteKingMoves(p1,p2))
        return false;
    if(p2.WKx!=p1.WKx && p2.WKy!=p1.WKy)
        return false;
    return (ApproachCriticalSquare(p1,p2) &&
            NotWhiteRookExposed(p2) &&
            (WhiteRookDivides(p2) || LPattern(p2)) &&
            (Room(p2) > 3 || (!KingsOnSameEdge(p2) && (DistanceWhiteKingToRook(p1)!=1 || !BackMove(p1,p2)))));
}

/* -------------------------------------------------------------------------------- */

bool KeepRoomDiagCond(KRKPosition p1,KRKPosition p2) {
    if(Stalemate(p2))
        return false;
    if(!WhiteKingMoves(p1,p2))
        return false;
    if(p2.WKx==p1.WKx || p2.WKy==p1.WKy)
        return false;
    return (NotWhiteRookExposed(p2) &&
            WhiteRookDivides(p2) &&
            WhiteKingAndRookNotDiverging(p1,p2) &&
           (Room(p2) > 3 || (!KingsOnSameEdge(p2) && (DistanceWhiteKingToRook(p1)!=1 || !BackMove(p1,p2)))));
}


/* -------------------------------------------------------------------------------- */

bool KeepRoomNonDiagCond(KRKPosition p1,KRKPosition p2) {
    if(Stalemate(p2))
        return false;
    if(!WhiteKingMoves(p1,p2))
        return false;
    if(p2.WKx!=p1.WKx && p2.WKy!=p1.WKy)
        return false;
    return (NotWhiteRookExposed(p2) &&
            WhiteRookDivides(p2) &&
            WhiteKingAndRookNotDiverging(p1,p2) &&
            (Room(p2) > 3 || (!KingsOnSameEdge(p2) && (DistanceWhiteKingToRook(p1)!=1 || !BackMove(p1,p2)))));
}

/* -------------------------------------------------------------------------------- */


bool RookHomeCond(KRKPosition p1,KRKPosition p2) {
    if (Stalemate(p2))
        return false;
    if(!WhiteRookMoves(p1,p2))
        return false;
    if (!NotBlackKingNextRook(p2) && !WhiteKingNextRook(p2))
        return false;
        return
        (p1.BKx<p1.WKx && p1.WRx!=p1.WKx-1 && p2.WRx==p1.WKx-1 ||
         p1.BKx>p1.WKx && p1.WRx!=p1.WKx+1 && p2.WRx==p1.WKx+1 ||
         p1.BKy<p1.WKy && p1.WRy!=p1.WKy-1 && p2.WRy==p1.WKy-1 ||
         p1.BKy>p1.WKy && p1.WRy!=p1.WKy+1 && p2.WRy==p1.WKy+1 ||
         p1.WRx==p1.WKx && p1.WKx==p1.BKx && (p2.WRx==p1.WKx+1 || p2.WRx==p1.WKx-1) ||
         p1.WRy==p1.WKy && p1.WKy==p1.BKy && (p2.WRy==p1.WKy+1 || p2.WRy==p1.WKy-1));
}

/* -------------------------------------------------------------------------------- */

bool RookSafeCond(KRKPosition p1,KRKPosition p2) {
    if (Stalemate(p2))
        return false;
    if(!WhiteRookMoves(p1,p2))
        return false;
    if (!(NotBlackKingNextRook(p2) || WhiteKingNextRook(p2)))
       return false;
    return (((p1.WRx!=0       && p2.WRx==0) ||
             (p1.WRx!=FILES-1 && p2.WRx==FILES-1) ||
             (p1.WRy!=0       && p2.WRy==0) ||
             (p1.WRy!=RANKS-1 && p2.WRy==RANKS-1))
              &&
             DistanceBlackKingToRook(p2)>2);
}


/* -------------------------------------------------------------------------------- */

bool RookSafeSmallBoardsCond(KRKPosition p1,KRKPosition p2) {
    if (Stalemate(p2))
        return false;
    if(!WhiteRookMoves(p1,p2))
        return false;
    return  ((p1.WRx!=0 && p2.WRx==0) || 
             (p1.WRx!=FILES-1 && p2.WRx==FILES-1) || 
             (p1.WRy!=0 && p2.WRy==0) || 
             (p1.WRy!=RANKS-1 && p2.WRy==RANKS-1)) &&
  		      DistanceBlackKingToRook(p2) == 2 && (p2.WRx == p2.WKx || p2.WRy == p2.WKy);

}

/* -------------------------------------------------------------------------------- */

KRKPosition Strategy(KRKPosition p1, eStratStep *s) {
    unsigned j;
    *s = eStepMax;
    KRKPosition p2; 
    for(j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p2))  
            if(ImmediateMateCond(p1,p2)) {
                *s=eImmediateMate;    
                return p2;
            }
    for(j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p2))  
            if(ReadyToMateCond(p1,p2)) {
                *s=eReadyToMate;    
                return p2;
            } 
    for(j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p2))  
            if(SqueezeCond(p1,p2)) {
                *s=eSqueeze;    
                return p2;
            }
    for(j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p2))  
            if(ApproachDiagCond(p1,p2)) {
                *s=eApproachDiag;   
                return p2;
            }
    for(j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p2))  
            if(ApproachNonDiagCond(p1,p2)) {
                *s=eApproachNonDiag;    
                return p2;
            }
    for(j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p2))  
            if(KeepRoomDiagCond(p1,p2)) {
                *s=eKeepRoomDiag;   
                return p2;
            }
    for(j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p2))  
            if(KeepRoomNonDiagCond(p1,p2)) {
                *s=eKeepRoomNonDiag;    
                return p2;
            }
    for(j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p2))  
            if(RookHomeCond(p1,p2)) {
                *s=eRookHome;   
                return p2;
            }
    for(j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p2))  
            if(RookSafeCond(p1,p2)) {
                *s=eRookSafe;
                return p2;
            }
    for(j=0;j<8+FILES+RANKS;j++)  
        if (NextLegalWhiteMove(j,p1,&p2))  
            if(RookSafeSmallBoardsCond(p1,p2)) {
                *s=eRookSafeSmallBoards;
                return p2;
            }
    return p1;
}


/* -------------------------------------------------------------------------------- */

// ********************************************************************
// Search procedures -- checking correctness, finding optimal moves
// ********************************************************************


void Init() {
    KRKPosition p1;
    unsigned long long i, PosNo=0;
    
    for(i=0;i<MAX_POS;i++) {
        Bitvector2Position(i,&p1);
        if (!LegalPositionWhiteToMove(p1))  {
    	  LookUpTable[i].OptimalDist=ILLEGAL;
    	  LookUpTable[i].StrategDist=ILLEGAL;
    	}
  	    else  {
	        if (IsRookCaptured(p1)) { 
	            LookUpTable[i].OptimalDist=DRAW;
	            LookUpTable[i].StrategDist=DRAW;
	        }
	        else {
	            LookUpTable[i].OptimalDist=UNKNOWN;
	            LookUpTable[i].StrategDist=UNKNOWN;
	            PosNo++;
	        }
    	}
    }
    printf("Total number of legal KRK Positions: %lli.\n",PosNo);

    SStep[eImmediateMate]=       "ImmediateMate       ";
    SStep[eReadyToMate]=         "ReadyToMate         ";
    SStep[eSqueeze]=             "Squeeze             ";
    SStep[eApproachDiag]=        "ApproachDiag        ";
    SStep[eApproachNonDiag]=     "ApproachNonDiag     ";
    SStep[eKeepRoomDiag]=        "KeepRoomDiag        ";
    SStep[eKeepRoomNonDiag]=     "KeepRoomNonDiag     ";
    SStep[eRookHome]=            "RookHome            ";
    SStep[eRookSafe]=            "RookSafe            ";
    SStep[eRookSafeSmallBoards]= "RookSafeSmallBoards ";

    _B_INITIALIZATION_DONE=true;
}

/* -------------------------------------------------------------------------------- */

void SetStrategyMoves() {
    unsigned long long i;
    unsigned notDefined=0;
    KRKPosition p;
    eStratStep s;
    
    if (!_B_INITIALIZATION_DONE)
        Init();

    _B_SETTING_STEPS_DONE=true;

    for(i=0;i<MAX_POS;i++)  {
        if (LookUpTable[i].StrategDist!=ILLEGAL && LookUpTable[i].StrategDist!=DRAW)  {
            Bitvector2Position(i,&p);
            LookUpTable[i].StrategPos = Position2Bitvector(Strategy(p,&s));
            LookUpTable[i].step = s;

	    if(s==eStepMax) {
               printf("Warning: the following KRKPosition has no defined strategy step:\n");
               Print(p);
               notDefined++;
               _B_SETTING_STEPS_DONE=false;
	    }
        }
    }

    if(!_B_SETTING_STEPS_DONE)
       printf("Warning: for %i KRKPositions the strategy is not defined!\n",notDefined);
}



/* -------------------------------------------------------------------------------- */

void ComputeOptimalDistancesToWin() {
    unsigned i,j,k;
    KRKPosition p1,p2,p3;

    if (!_B_INITIALIZATION_DONE)
        Init();

    unsigned Iteration=0,Updated=0;
    do {
        Updated=0;
        Iteration++;
        printf("Iteration: %i.\n",Iteration);

        for(i=0;i<MAX_POS;i++) {

            if (LookUpTable[i].OptimalDist!=ILLEGAL && LookUpTable[i].OptimalDist!=DRAW)  {
                Bitvector2Position(i,&p1);

                for(j=0;j<8+FILES+RANKS;j++) { // find best move for White 
                    if (NextLegalWhiteMove(j,p1,&p2)) { 

                        unsigned d3_max=UNDEF;
                        bool bNoLegalMoves=true;
                        for(k=0;k<8;k++)  {  // find best move for Black 
                            if (NextLegalBlackMove(k,p2,&p3))  {  
                                bNoLegalMoves=false;
                                unsigned d3_cur = LookUpTable[Position2Bitvector(p3)].OptimalDist;
                                if (d3_cur==DRAW || d3_cur==UNKNOWN)
                                    d3_max=d3_cur;
                                else if (d3_max==UNDEF || d3_cur>d3_max) 
                                    d3_max=d3_cur;
                            }
                        }

                        if (bNoLegalMoves) {
                            if(WRAttacksBK(p2) && LookUpTable[i].OptimalDist!=1) { // mate
                                LookUpTable[i].OptimalDist=1;
                                LookUpTable[i].OptimalPos=Position2Bitvector(p2);;
                                Updated++;
                            }
                        }
                        else if(d3_max!=ILLEGAL && d3_max!=UNKNOWN && d3_max!=DRAW && LookUpTable[i].OptimalDist>d3_max+2) {
                            LookUpTable[i].OptimalDist = d3_max+2;
                            LookUpTable[i].OptimalPos=Position2Bitvector(p2);;
                            Updated++;
                        }
                    }
                }
            }
        }

        unsigned Dist[1010];
        for(i=0;i<1000;i++) 
            Dist[i]=0;
        for(i=0;i<MAX_POS;i++) 
            Dist[LookUpTable[i].OptimalDist]++;
        for(i=0;i<100;i++) 
            if (i%2)
                printf("Guaranteed mate in: %i plies for: %i KRKPositions.\n",i,Dist[i]);

        unsigned u=0;
        for(i=0;i<MAX_POS;i++) 
            if (LookUpTable[i].OptimalDist==UNKNOWN)
                u++;
        printf("Unknown: %i.\n", u);
        printf("Updated: %i.\n", Updated);
    }
    while(Updated);
//    getc(stdin);
}

/* -------------------------------------------------------------------------------- */

void ComputeStrategyDistancesToWin() {
    KRKPosition p,np,p1,p2,p3;
    eStratStep s;
    unsigned i,k,u,d3_max,Iteration=0,Updated=0;

    if (!_B_SETTING_STEPS_DONE) {
        printf("Strategy not defined for all legal KRKPositions!\n");
        return;
    }

    do {
        Updated=0;
        Iteration++;
        printf("Iteration: %i\n", Iteration);

        for(i=0;i<MAX_POS;i++) {
            if (LookUpTable[i].StrategDist!=ILLEGAL && LookUpTable[i].StrategDist!=DRAW)  {
                Bitvector2Position(i,&p1);
                Bitvector2Position(LookUpTable[i].StrategPos,&p2);
                d3_max=UNDEF;
                bool bNoLegalMoves=true;
                for(k=0;k<8;k++)  {  // find best move for Black 
                    if (NextLegalBlackMove(k,p2,&p3))  {  
                        bNoLegalMoves=false;
                        unsigned d3_cur = LookUpTable[Position2Bitvector(p3)].StrategDist;
                        if (d3_cur==DRAW || d3_cur==UNKNOWN)
                            d3_max=d3_cur;
                        else if (d3_max==UNDEF || d3_cur>d3_max) 
                            d3_max=d3_cur;
                    }
                }

                if (bNoLegalMoves) {
                    if(WRAttacksBK(p2) && LookUpTable[i].StrategDist!=1) { // mate
                        LookUpTable[i].StrategDist=1;
                        Updated++;
                    }
                }
                else if(d3_max!=ILLEGAL && d3_max!=UNKNOWN && d3_max!=DRAW && LookUpTable[i].StrategDist>d3_max+2) {
                    LookUpTable[i].StrategDist = d3_max+2;
                    Updated++;
                }
            }
        }

        unsigned Dist[1010];
        for(i=0;i<1000;i++) 
            Dist[i]=0;
        for(i=0;i<MAX_POS;i++) 
            Dist[LookUpTable[i].StrategDist]++;
        for(i=0;i<200;i++) 
            if (i%2)
                printf("Guaranteed mate in: %i plies for: %i KRKPositions.\n",i,Dist[i]);

        u=0;
        for(i=0;i<MAX_POS;i++) 
            if (LookUpTable[i].StrategDist==UNKNOWN)
                u++;
        printf("Unknown: %i.\n", u);
        printf("Updated: %i.\n", Updated);
    }
    while(Updated);

    if(u==0)
        printf("The strategy is correct!\n");
    else
        printf("The strategy is incorrect!\n");

    for(i=0;i<MAX_POS;i++) 
        if (LookUpTable[i].StrategDist==UNKNOWN)  {
            printf("KRKPosition UNKNOWN %i\n", i);;
            Bitvector2Position(i,&p);
            Print(p);
            np=Strategy(p,&s);  
            printf("Strategy move (%i): \n",LookUpTable[i].step);
            Print(np);
            printf("Press Enter... \n");
            getc(stdin);
        }
}

/* -------------------------------------------------------------------------------- */

void MeasureDecreases() {
    KRKPosition p,pm,po; 
    eStratStep s;
    unsigned i,m1,mo,mdec=0,opt=0,im;

    if (!_B_INITIALIZATION_DONE)
        Init();
    
    for(i=0;i<MAX_POS;i++) 
        if (LookUpTable[i].OptimalDist!=ILLEGAL && LookUpTable[i].OptimalDist!=DRAW)  {
            Bitvector2Position(i,&p);
            m1=Room(p);
            pm=Strategy(p,&s);
            im=Position2Bitvector(pm);
            Bitvector2Position(LookUpTable[i].OptimalPos,&po);
            mo=Room(po);
            if (m1>mo) mdec++;
            if(im==LookUpTable[i].OptimalPos) opt++;
            else {
                printf("Starting KRKPosition : %i",i);
                Print(p);
                printf("Strategy\n");
                Print(pm);
                printf("Optimal\n");
                Print(po);
            }
        }
        printf("The measure decreases in %i cases\n",mdec);
        printf("Optimal move chosen in %i cases\n",opt);
}


void TerminationLemma() {
    KRKPosition p1,p2,p3,p4,p5,p6,p7; 
    eStratStep s1,s2,s3;
    unsigned i,m1,mdec=0,m7,k,kk,kkk,pos=0;

    if (!_B_INITIALIZATION_DONE)
        Init();

    time_t seconds_start,seconds_curr;
    seconds_start = time (NULL);
    
    for(i=0;i<MAX_POS;i++) 
        if (LookUpTable[i].OptimalDist!=ILLEGAL && LookUpTable[i].OptimalDist!=DRAW)  {

            pos++;
//            printf("Initial KRKPositions: %i\n",pos);
            Bitvector2Position(i,&p1);
            m1=Measure(p1);

            p2=Strategy(p1,&s1);
            for(k=0;k<8;k++)  {  // find best move for Black 
                if (NextLegalBlackMove(k,p2,&p3))  {  

                    p4=Strategy(p3,&s2);
                    for(kk=0;kk<8;kk++)  {  // find best move for Black 
                        if (NextLegalBlackMove(kk,p4,&p5))  {  

                            p6=Strategy(p5,&s3);
                            for(kkk=0;kkk<8;kkk++)  {  // find best move for Black 
                                if (NextLegalBlackMove(kkk,p6,&p7))  {  

                                   m7=Measure(p7);

                                   if(s1!=eReadyToMate && s1!=eRookHome && s1!=eRookSafe &&
                                      s2!=eReadyToMate && s2!=eRookHome && s2!=eRookSafe &&
                                      s3!=eReadyToMate && s3!=eRookHome && s3!=eRookSafe &&  
                                      m7>=m1 && m1>24) { 
                                          mdec++;
                                          printf("-------------------------------");
                                          printf("Measure increases! : %i\n",pos);
                                          Print(p1);
                                          Print(p2);
                                          Print(p3);
                                          Print(p4);
                                          Print(p5);
                                          Print(p6);
                                          Print(p7);
                                   }
                                }
                            }
                        }   
                    }
                }
            }
        }
     printf("The measure decreases in %i cases\n",mdec);
     printf("Initial KRKPositions: %i\n",pos);
     seconds_curr = time (NULL);
     printf("Time spent: %u seconds\n", (unsigned int)(seconds_curr-seconds_start));
     printf("Press Enter... \n");
     getc(stdin);
}


/* -------------------------------------------------------------------------------- */

void CountStrategySteps() {
    unsigned i,Steps[eStepMax],Total=0;

    if (!_B_SETTING_STEPS_DONE) {
        printf("Partial result: the strategy not defined for all legal KRKPositions!\n");
    }
    
    for(i=0;i<eStepMax;i++) 
        Steps[i]=0;

    for(i=0;i<MAX_POS;i++)  {
        if (LookUpTable[i].OptimalDist!=ILLEGAL && LookUpTable[i].OptimalDist!=DRAW && LookUpTable[i].step!=eStepMax) { 
            Steps[LookUpTable[i].step]++;
            Total++;
        }
    }

    for(i=0;i<eStepMax;i++)  
        printf("Step : %s : %i\n", SStep[i], Steps[i]);
    printf("----------------------------------------------\n");
    printf("Total:                     %i\n",Total);
}

/* -------------------------------------------------------------------------------- */

unsigned Measure(KRKPosition p) {
    return 6*Room(p)+ManhattanDistance(p.WKx,p.WKy,CSx(p),CSy(p));
}

/* -------------------------------------------------------------------------------- */

// prints out the lookup table
void PrintTable(LookUpRecord * lur, bool shouldPrint, bool isOptimal)
{
    int legitMoves = 0;
    FILE * f = fopen(outputFIleName, "w");
    if (f == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
    fprintf(f, "Ignore this first line. Format: isWhiteTurn, isRookCaptured, BKx, BKy, WKx, WKy, WRx, WRy. One line for 'key', next line for 'value'.\n");
    KRKPosition pos;
    for(int i=0;i<MAX_POS; i++)
    {
        LookUpRecord r = lur[i];
        if (isOptimal && (r.OptimalDist == ILLEGAL || r.OptimalDist == DRAW))
        {
            //printf("Skipping %d\n", i);
            continue;
        }
        if (!isOptimal && (r.StrategDist == ILLEGAL || r.StrategDist == DRAW))
        {
            //printf("Skipping %d\n", i);
            continue;
        }
        
        legitMoves++;
        // if (!shouldPrint)
        //     continue;

        //Print(pos);
        // printf("BitVec=%llu, %llu, %llu, %llu, %llu\n", 
        //     r.Pos,
        //     r.OptimalPos,
        //     r.OptimalDist,
        //     r.StrategPos,
        //     r.StrategDist
        //     );

        Bitvector2Position(i, &pos);

        // printf("Key: turn = %s, rook captured = %s, BK = %d, %d, WK = %d, %d, WR = %d, %d\n",
        //     pos.bWhiteOnTurn ? "white" : "black", 
        //     pos.bRookCaptured ? "true" : "false",
        //     pos.BKx, pos.BKy,
        //     pos.WKx, pos.WKy,
        //     pos.WRx, pos.WRy
        //     );
        fprintf(f, "%d %d %d %d %d %d %d %d\n",
            pos.bWhiteOnTurn, 
            pos.bRookCaptured,
            pos.BKx, pos.BKy,
            pos.WKx, pos.WKy,
            pos.WRx, pos.WRy
            );
        
        Bitvector2Position(isOptimal ? r.OptimalPos : r.StrategPos, &pos);
        fprintf(f, "%d %d %d %d %d %d %d %d\n",
            pos.bWhiteOnTurn, 
            pos.bRookCaptured,
            pos.BKx, pos.BKy,
            pos.WKx, pos.WKy,
            pos.WRx, pos.WRy
            );
        //printf("\n");
    }
    fclose(f);
    printf("Number of legit moves = %d\n", legitMoves);
}

// ********************************************************************
// Main
// ********************************************************************

unsigned main(unsigned argc, char **argv) {
    time_t seconds_start,seconds_curr;
    seconds_start = time (NULL);

    if (argc!=3) {
       printf("\nWrong number of arguments. The program is invoked with arguments Files and Ranks\n");
       return -1; 
    }

    FILES = (unsigned int)atoi(argv[1]); 
    RANKS = (unsigned int)atoi(argv[2]);
   
    if(FILES<=4)
      FILES_BITS = 2;
    else if (FILES<=8)
      FILES_BITS = 3;
    else if (FILES<=16)
      FILES_BITS = 4;
    else if (FILES<=32)
      FILES_BITS = 5;

    if(RANKS<=4)
      RANKS_BITS = 2;
    else if (RANKS<=8)
      RANKS_BITS = 3;
    else if (RANKS<=16)
      RANKS_BITS = 4;
    else if (RANKS<=32)
      RANKS_BITS = 5;

    FILES_MASK = (1<<FILES_BITS)-1;
    RANKS_MASK = (1<<RANKS_BITS)-1;

    MAX_POS = (unsigned long long)4*(1<<FILES_BITS)*(1<<RANKS_BITS)*(1<<FILES_BITS)*(1<<RANKS_BITS)*(1<<FILES_BITS)*(1<<RANKS_BITS);
    printf("\n Files = %i, Ranks = %i, Max pos = %lli\n", FILES, RANKS, MAX_POS);

    size_t sz = MAX_POS*sizeof(LookUpRecord);
    LookUpTable = (LookUpRecord*)malloc(sz);

    if (LookUpTable == NULL)  {
       printf("\n Unsufficient memory!\n");
       return -1;
    }

    _B_INITIALIZATION_DONE=false;
    _B_SETTING_STEPS_DONE=false;

    //ComputeOptimalDistancesToWin();
    //MeasureDecreases();
    SetStrategyMoves();

    //CountMeasureDecreasing();

    //TerminationLemma();

    ComputeStrategyDistancesToWin();
    // CountStrategySteps();  

    seconds_curr = time (NULL);
    
    PrintTable(LookUpTable, false, false);

    free(LookUpTable);

    printf("Time spent: %u seconds\n", (unsigned int)(seconds_curr-seconds_start));

    printf("Press Enter... \n");
    getc(stdin);
    return 0;
}

