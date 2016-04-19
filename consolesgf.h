#ifndef CONSOLESGF_H
#define CONSOLESGF_H
#include<linux/limits.h>
#define MAXIMUM_COMMAND_LENGTH 4
#define MAX_BOARD_SIZE 19
/* board node is an intersection on the board.
   head's head suppose to point to something, otherwise intersection is empty.
   */
struct TUIBoardNode
{
    int size, libs; /* head's properties */
    bool isBlack; /* every node's */
    struct TUIBoardNode * next, *head /* every node's , proper head on alive node */; 
    struct TUIBoardNode * tail; /* head's */
    struct TUIBoardNode * left, * up, * right, * down; /* every node's */
};
struct TUIBoard
{
    int size, bRemoved, wRemoved, moveCount, passes, wTer, bTer;
    float komi;
    bool bTurn, done, undoRefreshesGame, tooManyMovesMsg;
    struct TUIBoardNode nodes[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    struct TUIBoardNode * koSpace;
    struct TUIBoardNode * moves[1024];
};
struct TUICmdLine
{
    bool active;
    int bufLen;
    wchar_t buf[MAXIMUM_COMMAND_LENGTH + 1 + PATH_MAX + 1]; /* :open filename */
};
struct consolesgfTUI
{
    struct TUICmdLine cmdLine;
    struct TUIBoard board;
    wchar_t boardBuf[MAX_BOARD_SIZE * 2 + 1][42];
    wchar_t rightPanelBuf[14][7];
};
void parseCmdLineBuffer();
void redrawTUI();
void setBoardBuf();
void setRightPanelBuf();
void setCmdLineBuf();
bool initBoard(unsigned int size);
bool makeMove(struct TUIBoardNode * n);
void setKo(struct TUIBoardNode * n);
void removeStones(struct TUIBoardNode * n);
bool isSuicide(struct TUIBoardNode * n);
void addStone(struct TUIBoardNode * n);
void mergeStones(struct TUIBoardNode ** n1, struct TUIBoardNode * n2);
void updateLibs(struct TUIBoardNode * n);
void cleanGhostNodes();
bool removeGroupForScoring(struct TUIBoardNode * n);
void undoMove();
void countTerritory();
void updateSeed(struct TUIBoardNode * n, struct TUIBoardNode * seed, bool * black, bool * white);

wchar_t csgfLiterals[40] = {32,32,65,32,66,32,67,32,68,32,69,32,70,32,71,32,72,32,74,32,75,32,76,32,77,32,78,32,79,32,80,32,81,32,82,32,83,32,84,0};
wchar_t csgfDigits[19][2] = {{32,49},{32,50},{32,51},{32,52},{32,53},{32,54},{32,55},{32,56},{32,57},{49,48},{49,49},{49,50},{49,51},{49,52},{49,53},{49,54},{49,55},{49,56},{49,57}};
#endif
