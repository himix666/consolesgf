/* 17.05.2016 version 0.11
 * japanese counting system , seki counting not implemented
 * all other japanese go rules implemented
 */
#define _XOPEN_SOURCE_EXTENDED 
#include"locale.h"
#include"ncursesw/ncurses.h"
#include"consolesgf.h"
#include<stdlib.h>
#include<string.h>
#include<math.h>
/* tui is a global variable, holding all text user interface data (board, command line, etc) */
struct consolesgfTUI tui;
/* algorithm description for updating free nodes list:
   a) find suitable free node, n0, set head, tail to n0
   b) iterate through tail to head, add adjacent free nodes to tail
   c) set new head to previous tail, set tail to free nodes list new tail
   d) if head is the same as tail - stop, otherwise go to b)
start:
    head,tail
   +----+
   | n0 | add 2 new nodes
   +----+
    head            tail
   +----+  +----+  +----+
   | n0 +<-+ n1 +<-+ n2 | update head and tail
   +----+  +----+  +----+
                    head    tail
   +----+  +----+  +----+  +----+
   | n0 +<-+ n1 +<-+ n2 +<-+ n3 | iterate from tail to head, add 1 new node
   +----+  +----+  +----+  +----+
                           head,tail 
   +----+  +----+  +----+  +----+
   | n0 +<-+ n1 +<-+ n2 +<-+ n3 | update head and tail, iterate from tail to head, head is tail - cycle is finished
   +----+  +----+  +----+  +----+
end:
 */ 
/* removeGroupForScoringStageTwo is a helper function for removeGroupForScoring:
   removes enemy groups adjacent to freelist and adds untouched nodes adjacent to those enemy groups to freelist */
void removeGroupForScoringStageTwo(struct TUIBoardNode * n, bool enemyColor, struct TUIBoardNode ** flist){
    if(n && n->head && n->isBlack == enemyColor){
	struct TUIBoardNode * it;
	it = n->head->tail;
	n->head->libs = 1; /* prepare group for removal, it must have exactly 1 lib */
	removeStones(n);
	do{
	    if(it->left && !(it->left->head || it->left->next))
		updateFreeList(it->left, flist);
	    if(it->up && !(it->up->head || it->up->next))
		updateFreeList(it->up, flist);
	    if(it->right && !(it->right->head || it->right->next))
		updateFreeList(it->right, flist);
	    if(it->down && !(it->down->head || it->down->next))
		updateFreeList(it->down, flist);
	    if(it == it->next)
		break;
	}while((it = it->next));
    }
}
/* removeGroupForScoring tries to remove selected by user captured groups.
   captured groups are divided only by free space
 */
bool removeGroupForScoring(struct TUIBoardNode * n){
    if(n->head == NULL)
	return false;
    tui.board.bTurn = !(n->isBlack); /* init done */
    struct TUIBoardNode * it, * it2, * flist, * head, * tail;
    it = n->head->tail; /* start stage 1: remove initial group and init list of free nodes */
    flist = NULL;
    n->head->libs = 1; /* prepare group for removal , it must have exactly 1 lib */
    removeStones(n);
    do{
	if(it->left && !(it->left->head || it->left->next))
	    updateFreeList(it->left, &flist);
	if(it->up && !(it->up->head || it->up->next))
	    updateFreeList(it->up, &flist);
	if(it->right && !(it->right->head || it->right->next))
	    updateFreeList(it->right, &flist);
	if(it->down && !(it->down->head || it->down->next))
	    updateFreeList(it->down, &flist);
	if(it == it->next)
	    break;
    }while((it = it->next)); /* stage 1 finished */
    /* stage 2: go through free nodes list, find adjacent captured groups, remove these groups, add not processed adjacent to removed groups free nodes, process new free nodes in the list. Then free nodes list is processed, clean it's fields. */
    tail = flist->tail;
    head = flist;
    do{
	it = tail;
	do{
	    removeGroupForScoringStageTwo(it->left, n->isBlack, &flist);
	    removeGroupForScoringStageTwo(it->up, n->isBlack, &flist);
	    removeGroupForScoringStageTwo(it->right, n->isBlack, &flist);
	    removeGroupForScoringStageTwo(it->down, n->isBlack, &flist);
	    if(it == head)
		break;
	}while((it = it->next));
	head = tail;
	tail = flist->tail;
    }while(head != tail);
    it = flist->tail;
    do{
	it2 = it;
	it = it->next;
	it2->next = NULL;
	it2->tail = NULL;
    }while(it != it2);
    tui.board.undoRefreshesGame = true; /* finishing successful removal */
    return true;
}
/* updateFreeListTail is a helper function for updateFreeList:
   updates tail of provided list with new node if possible */
void updateFreeListTail(struct TUIBoardNode * n, struct TUIBoardNode * flist){
    if(n && !(n->head || n->next)){ /* new node must not be in any list */
	n->next = flist->tail;
	flist->tail = n;
    }
}
/* updateFreeList is a helper function for removeGroupForScoring:
   updates list of free nodes by adding n and adjacent to n nodes */
void updateFreeList(struct TUIBoardNode * n, struct TUIBoardNode ** flist){
    struct TUIBoardNode * tail, * head, * it;
    if(*flist){
	head = (*flist)->tail;
	updateFreeListTail(n, *flist);
	tail = (*flist)->tail;
    } else {
	*flist = n;
	tail = n;
	head = n;
	(*flist)->next = n;
	(*flist)->tail = n;
    }
    do{
	it = tail;
	do{
	    updateFreeListTail(it->left, *flist);
	    updateFreeListTail(it->up, *flist);
	    updateFreeListTail(it->right, *flist);
	    updateFreeListTail(it->down, *flist);
	}while((it = it->next) != head);
	head = tail;
	tail = (*flist)->tail;
    }while(tail != head);
}
/* addMiscMessages adds miscellaneous messages under board on the screen */
void addMiscMessages(){
    wchar_t buf[30] = {L'\0'};
    /* misc messages like "remove dead groups" */
    int miscMsgStartLine = tui.board.size < 7? 15 : tui.board.size * 2 + 2;
    if(tui.board.moveCount && miscMsgStartLine < LINES){ /* show previous move if moves have been done */
	if(tui.board.moves[tui.board.moveCount - 1] == NULL)
	    mvaddnwstr(miscMsgStartLine, 0, L"  PASSED", COLS);
	else{
	    int coords = ((void *)(tui.board.moves[tui.board.moveCount - 1]) - (void *) tui.board.nodes) / sizeof(struct TUIBoardNode);
	    wcsncpy(buf, L"  PLAYED   ", 11);
	    buf[9] = csgfLiterals[2 + (coords % 19) * 2];
	    if(tui.board.size - 1 - coords / 19 > 8){
		buf[10] = L'1';
		buf[11] = csgfDigits[tui.board.size - 1 - coords / 19][1];
	    }
	    else buf[10] = csgfDigits[tui.board.size - 1 - coords / 19][1];
	    mvaddnwstr(miscMsgStartLine, 0, buf, COLS);
	}
	if(tui.board.moveCount % 2)
	    mvaddnwstr(miscMsgStartLine, 0, L"B", COLS);
	else
	    mvaddnwstr(miscMsgStartLine, 0, L"W", COLS);
	miscMsgStartLine += 2;
    }
    if(tui.board.tooManyMovesMsg && miscMsgStartLine < LINES){
	mvaddnwstr(miscMsgStartLine , 0, L"NO MORE MOVES CAN BE DONE, YOU MAY ONLY UNDO A MOVE", COLS);
	miscMsgStartLine += 2;
    }
    if(miscMsgStartLine < LINES){
	if(tui.board.done == false){
	    if(tui.board.passes == 2)
		mvaddnwstr(miscMsgStartLine , 0, L"REMOVE DEAD GROUPS", COLS);
	} else {
	    float score = tui.board.bRemoved + tui.board.bTer - tui.board.wRemoved - tui.board.wTer - tui.board.komi;
	    if(score > 0)
		swprintf(buf, 29, L"BLACK WINS BY %.1f POINTS", score);
	    else
		swprintf(buf, 29, L"WHITE WINS BY %.1f POINTS", fabsf(score));
	    mvaddnwstr(miscMsgStartLine , 0, buf, COLS);
	    miscMsgStartLine += 2;
	}
    }
}
/* helper for function countTerritory: adds new node to seed list or updates seed's affiliation */
void updateSeed(struct TUIBoardNode * n, struct TUIBoardNode * seed, bool * black, bool * white){
    if(n){
	if(n->head){
	    if(n->head->isBlack)
		*black = true;
	    else
		*white = true;
	} else if(n->next == NULL){
	    n->next = seed->tail;
	    seed->tail = n;
	    seed->size++;
	}
    }
}
/* after pressing "DONE" counts white and black territories by using next and tail field of struct.
   adds empty nodes around seed to seed list and counts how many additions happened , checks every
   node in seed list once if there empty node around that node to add */
void countTerritory(){
    cleanGhostNodes();
    struct TUIBoardNode * tail, * head, * seed, * it;
    bool black, white;
    for(int i = 0; i < tui.board.size; i++)
	for(int j = 0; j < tui.board.size; j++){
	    if(tui.board.nodes[i][j].head || tui.board.nodes[i][j].next)
		continue;
	    seed = &(tui.board.nodes[i][j]);
	    black = white = false;
	    tail = seed;
	    head = seed;
	    seed->next = seed;
	    seed->tail = seed;
	    seed->size = 1;
	    do{
		it = tail;
		do{
		    updateSeed(it->left, seed, &black, &white);
		    updateSeed(it->up, seed, &black, &white);
		    updateSeed(it->right, seed, &black, &white);
		    updateSeed(it->down, seed, &black, &white);
		}while((it = it->next) != head);
		head = tail;
		tail = seed->tail;
	    }while(tail != head);
	    if(white == black)
		continue;
	    if(white)
		tui.board.wTer += seed->size;
	    else
		tui.board.bTer += seed->size;
	}
    cleanGhostNodes();
}
/* undoMove refreshes board to finished state if player selected group as dead or undoes one move */
void undoMove(){
    if(tui.board.moveCount == 0)
	return;
    if(tui.board.undoRefreshesGame){
	tui.board.moveCount++;
	tui.board.undoRefreshesGame = false;
    }
    int mc = tui.board.moveCount - 1;
    tui.board.bTer = tui.board.wTer = tui.board.bRemoved = tui.board.wRemoved = tui.board.moveCount = tui.board.passes = 0;
    tui.board.bTurn = true;
    tui.board.done = false; 
    tui.board.koSpace = NULL;
    for(int i = 0; i < tui.board.size; i++)
	for(int j = 0; j < tui.board.size; j++){
	    tui.board.nodes[i][j].head = NULL;
	    tui.board.nodes[i][j].next = NULL;
	    tui.board.nodes[i][j].tail = NULL;
	}
    for(int i = 0; i < mc; i++)
	makeMove(tui.board.moves[i]);
}
/* cleanGhostNodes makes free nodes really clean and ready for done-phase */
void cleanGhostNodes(){
    for(int i = 0; i < tui.board.size; i++)
	for(int j = 0; j < tui.board.size; j++)
	    if(tui.board.nodes[i][j].head == NULL){
		tui.board.nodes[i][j].next = NULL;
		tui.board.nodes[i][j].tail = NULL;
	    }
}
/* updateLibs updates node's n's head libs and enemy's affected by n node groups */
void updateLibs(struct TUIBoardNode * n){
    int libs = 0;
    struct TUIBoardNode * n1 = NULL, * n2 = NULL;
    /* updating n's group's libs 
       if empty node's neighbour is n's group then it is a liberty */
    for(int i = 0; i < tui.board.size; i++)
	for(int j = 0; j < tui.board.size; j++){
	    n1 = &(tui.board.nodes[i][j]);
	    if(n1->head == NULL)
		if((n1->left && n1->left->head == n->head) ||
			(n1->up && n1->up->head == n->head) ||
			(n1->right && n1->right->head == n->head) ||
			(n1->down && n1->down->head == n->head))
		    libs++;
	}
    n->head->libs = libs;
    /* updating enemy's groups' libs affected by n-node */
    if(n->left && n->left->head && n->left->isBlack != tui.board.bTurn){
	n1 = n->left->head;
	n1->libs = n1->libs - 1;
    }
    if(n->up && n->up->head && n->up->isBlack != tui.board.bTurn && n1 != n->up->head){
	if(n1 == NULL){
	    n1 = n->up->head;
	    n1->libs = n1->libs - 1;
	} else {
	    n2 = n->up->head;
	    n2->libs = n2->libs - 1;
	}
    }
    if(n->right && n->right->head && n->right->isBlack != tui.board.bTurn)
	if(n->right->head != n1 && n->right->head != n2){
	    if(n1 == NULL){
		n1 = n->right->head;
		n1->libs = n1->libs - 1;
	    } else if(n2 == NULL){
		n2 = n->right->head;
		n2->libs = n2->libs - 1;
	    } else {
		n->right->head->libs = n->right->head->libs - 1;
		return;
	    }
	}
    if(n->down && n->down->head && n->down->isBlack != tui.board.bTurn)
	if(n->down->head != n1 && n->down->head != n2)
	    n->down->head->libs = n->down->head->libs - 1;
}
/* merges two groups into one group:
   updates smaller group's nodes' head
   adds smaller group to the tail of bigger group */
void mergeStones(struct TUIBoardNode ** n1, struct TUIBoardNode * n2){
    /* n1 is merger's group's head's address , n2 is to be merged group's head */
    struct TUIBoardNode * tail , * head, *m;
    int size;
    if((*n1)->size < n2->size){
	head = *n1;
	m = (*n1)->tail;
	tail = m;
	*n1 = n2;
    } else {
	head = n2;
	m = n2->tail;
	tail = m;
    }
    size = head->size;
    do{
	m->head = (*n1);
    }while((m = m->next));
    head->next = (*n1)->tail;
    (*n1)->tail = tail;
    (*n1)->size = (*n1)->size + size;
}
/* adds stone to the board:
   finds merger group, adds other groups to it, updates n's info */
void addStone(struct TUIBoardNode * n){
    struct TUIBoardNode * merger = NULL;
    if(n->left && n->left->head && n->left->isBlack == tui.board.bTurn)
	merger = n->left->head;
    if(n->up && n->up->head && n->up->isBlack == tui.board.bTurn){
	if(merger && merger != n->up->head)
	    mergeStones(&merger, n->up->head);
	else
	    merger = n->up->head;
    }
    if(n->right && n->right->head && n->right->isBlack == tui.board.bTurn){
	if(merger && merger != n->right->head)
	    mergeStones(&merger, n->right->head);
	else
	    merger = n->right->head;
    }
    if(n->down && n->down->head && n->down->isBlack == tui.board.bTurn){
	if(merger && merger != n->down->head)
	    mergeStones(&merger, n->down->head);
	else
	    merger = n->down->head;
    }
    if(merger){
	n->next = merger->tail;
	merger->tail = n;
	n->head = merger;
	merger->size += 1;
	n->isBlack = tui.board.bTurn;
	updateLibs(n);
    } else {
	n->tail = n;
	n->head = n;
	n->size = 1;
	n->isBlack = tui.board.bTurn;
	updateLibs(n);
    }
}
/* checks if playing n is suicidal:
   playing n resulting group must have liberties */
bool isSuicide(struct TUIBoardNode * n){
    if(n->left){
	if(n->left->head == NULL)
	    return false;
	if(n->left->isBlack == tui.board.bTurn && n->left->head->libs != 1)
	    return false;
    }
    if(n->right){
	if(n->right->head == NULL)
	    return false;
	if(n->right->isBlack == tui.board.bTurn && n->right->head->libs != 1)
	    return false;
    }
    if(n->down){
	if(n->down->head == NULL)
	    return false;
	if(n->down->isBlack == tui.board.bTurn && n->down->head->libs != 1)
	    return false;
    }
    if(n->up){
	if(n->up->head == NULL)
	    return false;
	if(n->up->isBlack == tui.board.bTurn && n->up->head->libs != 1)
	    return false;
    }
    return true;
}
/* updates to be removed group's neighbour friendly groups' libs and removes group */
void removeStones(struct TUIBoardNode * n){ 
    if(n && n->head && n->isBlack != tui.board.bTurn && n->head->libs == 1){
	struct TUIBoardNode * head = n->head;
	if(tui.board.bTurn)
	    tui.board.bRemoved += n->head->size;
	else
	    tui.board.wRemoved += n->head->size;
	struct TUIBoardNode * n1 = n->head->tail, * n2, * n3;
	do{
	    n2 = NULL, n3 = NULL;
	    n1->head = NULL;
	    if(n1->left && n1->left->head && n1->left->isBlack == tui.board.bTurn){
		n2 = n1->left->head;
		n1->left->head->libs = n1->left->head->libs + 1;	
	    }
	    if(n1->up && n1->up->head && n1->up->isBlack == tui.board.bTurn)
		if(n1->up != n2){
		    if(n2)
			n3 = n1->up->head;
		    else
			n2 = n1->up->head;
		    n1->up->head->libs = n1->up->head->libs + 1;
	    }
	    if(n1->right && n1->right->head && n1->right->isBlack == tui.board.bTurn)
		if(n1->right != n2 && n1->right != n3){
		    if(n2 == NULL){
			n2 = n1->right->head;
			n1->right->head->libs = n1->right->head->libs + 1;
		    } else if(n3 == NULL){
			n3 = n1->right->head;
			n1->right->head->libs = n1->right->head->libs + 1;
		    } else {
			n1->right->head->libs = n1->right->head->libs + 1;
			continue;
		    }
	    }
	    if(n1->down && n1->down->head && n1->down->isBlack == tui.board.bTurn)
		if(n1->down != n2 && n1->down != n3)
			n1->down->head->libs = n1->down->head->libs + 1;
	}while((n1 = n1->next));
	head->next = head;
    }
}
/* sets ko to n-node's affected neighbour free space so next played stone cannot be placed in it */
void setKo(struct TUIBoardNode * n){
    if(n->left && n->left->head == NULL)
	tui.board.koSpace = n->left;
    if(n->up && n->up->head == NULL)
	tui.board.koSpace = n->up;
    if(n->right && n->right->head == NULL)
	tui.board.koSpace = n->right;
    if(n->down && n->down->head == NULL)
	tui.board.koSpace = n->down;
}
/* makes move if possible:
   checks if node is free and not affected by ko-rule, removes captured stones, checks for suicide,
   adds stone to the board, updates ko-situation and if move is successful sets next turn's color */
bool makeMove(struct TUIBoardNode * n){
    if(tui.board.moveCount == 1024){
	tui.board.tooManyMovesMsg = true;
	return false;
    }
    if(n == NULL){ /* only 2 passes allowed, one for black, one for white */
	if(tui.board.passes < 2){
	    tui.board.passes++;
	    if(tui.board.passes == 2)
		cleanGhostNodes(); 
	}
	else
	    return false;
    } else {
	int stones = tui.board.bTurn? tui.board.bRemoved: tui.board.wRemoved;
	if(n->head)
	    return false; /* x , y is already occupied */
	if(n == tui.board.koSpace)
	    return false;
	removeStones(n->left);
	removeStones(n->up);
	removeStones(n->right);
	removeStones(n->down);
	if(isSuicide(n))
	    return false;
	addStone(n);
	if((tui.board.bTurn? tui.board.bRemoved: tui.board.wRemoved) - stones == 1 && n->head->size == 1 && n->head->libs == 1)
	    setKo(n);
	else
	    if(tui.board.koSpace)
		tui.board.koSpace = NULL; /* get rid of ko */
	if(tui.board.passes)
	    tui.board.passes = 0;
    }
    tui.board.bTurn = !(tui.board.bTurn);
    tui.board.moves[tui.board.moveCount++] = n;
    return true;
}
/* initializes board:
   sets every node's left,up,down,right nodes so we can move with ->direction notation */
bool initBoard(unsigned int size){
    if((size < 3) || (size > 19) || ((size % 2) == 0))
	return false;
    tui.board.size = size;
    tui.board.koSpace = NULL;
    tui.board.bTurn = true;
    tui.board.passes = 0;
    tui.board.done = false;
    tui.board.wRemoved = 0;
    tui.board.bRemoved = 0;
    tui.board.komi = 7.5;
    tui.board.moveCount = 0;
    memset(tui.board.nodes, 0, MAX_BOARD_SIZE * MAX_BOARD_SIZE * sizeof(struct TUIBoardNode));
    for(int i = 1; i < tui.board.size - 1; i++){
	tui.board.nodes[0][i].left = &(tui.board.nodes[0][i - 1]); /* top-nodes, up is NULL */
	tui.board.nodes[0][i].down = &(tui.board.nodes[1][i]);
	tui.board.nodes[0][i].right = &(tui.board.nodes[0][i + 1]);
	tui.board.nodes[i][0].up = &(tui.board.nodes[i - 1][0]); /* left-nodes, left is NULL */
	tui.board.nodes[i][0].right = &(tui.board.nodes[i][1]);
	tui.board.nodes[i][0].down =&(tui.board.nodes[i + 1][0]);
	tui.board.nodes[tui.board.size - 1][i].left = &(tui.board.nodes[tui.board.size - 1][i - 1]); /* down-nodes, down is NULL */
	tui.board.nodes[tui.board.size - 1][i].up = &(tui.board.nodes[tui.board.size - 2][i]);
	tui.board.nodes[tui.board.size - 1][i].right = &(tui.board.nodes[tui.board.size - 1][i + 1]);
	tui.board.nodes[i][tui.board.size - 1].up = &(tui.board.nodes[i - 1][tui.board.size - 1]); /* right-nodes, right is NULL */
	tui.board.nodes[i][tui.board.size - 1].down = &(tui.board.nodes[i + 1][tui.board.size - 1]);
	tui.board.nodes[i][tui.board.size - 1].left = &(tui.board.nodes[i][tui.board.size - 2]);
    }
    tui.board.nodes[0][0].right = &(tui.board.nodes[0][1]);
    tui.board.nodes[0][0].down = &(tui.board.nodes[1][0]);
    tui.board.nodes[0][tui.board.size - 1].left = &(tui.board.nodes[0][tui.board.size - 2]);
    tui.board.nodes[0][tui.board.size - 1].down = &(tui.board.nodes[1][tui.board.size - 1]);
    tui.board.nodes[tui.board.size - 1][tui.board.size - 1].left = &(tui.board.nodes[tui.board.size - 1][tui.board.size - 2]);
    tui.board.nodes[tui.board.size - 1][tui.board.size - 1].up = &(tui.board.nodes[tui.board.size - 2][tui.board.size - 1]);
    tui.board.nodes[tui.board.size - 1][0].up = &(tui.board.nodes[tui.board.size - 2][0]);
    tui.board.nodes[tui.board.size - 1][0].right = &(tui.board.nodes[tui.board.size - 1][1]);
    for(int i = 1; i < tui.board.size - 1; i++)
	for(int j = 1; j < tui.board.size - 1; j++){
	    tui.board.nodes[j][i].left = &(tui.board.nodes[j][i - 1]);
	    tui.board.nodes[j][i].up = &(tui.board.nodes[j - 1][i]);
	    tui.board.nodes[j][i].right = &(tui.board.nodes[j][i + 1]);
	    tui.board.nodes[j][i].down = &(tui.board.nodes[j + 1][i]);
	}
    return true;
}
/* parses for:
   :n size , where legal size is uneven decimal between 3 and 19
   :q for quit
   illegal input is discarded without error msg */
void parseCmdLineBuffer(){
    int cntChar = 0;
    unsigned int size = 0;
    if(tui.cmdLine.bufLen == 0){
	curs_set(0);
	tui.cmdLine.active = false;
	redrawTUI();
	return;
    }
    while(tui.cmdLine.buf[cntChar] != L'\0'){
	switch(tui.cmdLine.buf[cntChar++]){
	    case L'n':
		if(tui.cmdLine.buf[cntChar++] != L' ')
		    goto clearinput;
		if(swscanf(tui.cmdLine.buf + cntChar, L"%ud", &size) == 0)
		    goto clearinput;
		if(initBoard(size) == false) /* TODO: make up error msg */
		    goto clearinput;
		goto clearinput;
	    case L'q':
		exit(EXIT_SUCCESS);
clearinput:
	    default:
		tui.cmdLine.bufLen = 0;
		curs_set(0);
		tui.cmdLine.active = false;
		tui.cmdLine.buf[0] = L'\0';
		redrawTUI();
		return;
	}
    }
}
/* draws text user interface:
   puts coordinates,
   board template (unicode box drawing),
   stones (B or W),
   command line if active */
void redrawTUI(){
    if(erase() == ERR){
	endwin();
	fprintf(stderr, "Cannot clear screen, aborting\n");
	exit(EXIT_FAILURE);
    }
    setBoardBuf();
    setRightPanelBuf();
    for(int i = 0, j = COLS < tui.board.size * 2 + 3 ? COLS : tui.board.size * 2 + 3; i < LINES && i < tui.board.size * 2 + 1; i++)
	/* mvprintw(i, 0, "%.*ls", j, tui.boardBuf[i]); */ /* printf counts \x253C as 3 characters, it's a bug */
	mvaddnwstr(i, 0, tui.boardBuf[i], j);
    if(tui.board.passes == 2) /* selected by user removed groups */
	for(int i = 0; i < tui.board.size; i++)
	    for(int j = 0; j < tui.board.size; j++)
		if(tui.board.nodes[i][j].head == NULL && (tui.board.nodes[i][j].next || tui.board.nodes[i][j].tail))
		    mvaddch(i * 2 + 1, j * 2 + 2, (tui.board.nodes[i][j].isBlack? 'B' : 'W') | COLOR_PAIR(1));
    if(COLS > tui.board.size * 2 + 9 && LINES > 13) /* print right panel if there is space for it */
	for(int i = 0; i < 14; i++)
	    mvaddwstr(i, tui.board.size * 2 + 4, tui.rightPanelBuf[i]);
    addMiscMessages();
    if(tui.cmdLine.active){
	if(tui.cmdLine.bufLen > COLS - 2)/* show end of buffer if it does not fit on screen */
	    mvprintw(LINES - 1, 0, ":%ls", tui.cmdLine.buf + tui.cmdLine.bufLen - COLS + 2);
	else
	    mvprintw(LINES - 1, 0, ":%ls", tui.cmdLine.buf);
    }
    refresh();
}
/* updates board's screen buffer */
void setBoardBuf(){
    memset(&(tui.boardBuf), 0, sizeof(tui.boardBuf)); /* reset board's output , fill buf , print according to LINES and COLS */
    wcsncpy(tui.boardBuf[0], csgfLiterals, tui.board.size * 2 + 1);
    wcsncpy(tui.boardBuf[tui.board.size * 2], csgfLiterals, tui.board.size * 2 + 1);
    for(int i = 0; i < tui.board.size; i++){ /* coords ready */
	tui.boardBuf[i * 2 + 1][0] = csgfDigits[tui.board.size - 1 - i][0];
	tui.boardBuf[i * 2 + 1][tui.board.size * 2 + 1] = csgfDigits[tui.board.size - 1 - i][0];
	tui.boardBuf[i * 2 + 1][1] = csgfDigits[tui.board.size - 1 - i][1]; 
	tui.boardBuf[i * 2 + 1][tui.board.size * 2 + 2] = csgfDigits[tui.board.size - 1 - i][1]; 
	tui.boardBuf[i * 2][0] = L' '; /* spaces before |-lines */
    }
    for(int i = 0; i < tui.board.size - 1; i++)
	for(int j = 0; j < tui.board.size; j++){
	    tui.boardBuf[j * 2 + 1][i * 2 + 3] = L'\x2500';
	    tui.boardBuf[i * 2 + 2][j * 2 + 1] = L' ' , tui.boardBuf[i * 2 + 2][j * 2 + 2] = L'\x2502';
	 }
    for(int i = 1; i < tui.board.size - 1; i++){ /* sides drawing */
	if(tui.board.nodes[0][i].head != NULL)
	    tui.boardBuf[1][i * 2 + 2] = tui.board.nodes[0][i].isBlack?L'B':L'W'; 
	else
	    tui.boardBuf[1][i * 2 + 2] = L'\x252C';
	if(tui.board.nodes[tui.board.size - 1][i].head != NULL)
	    tui.boardBuf[tui.board.size * 2 - 1][i * 2 + 2] = tui.board.nodes[tui.board.size - 1][i].isBlack?L'B':L'W';
	else
	    tui.boardBuf[tui.board.size * 2 - 1][i * 2 + 2] = L'\x2534';
	if(tui.board.nodes[i][0].head != NULL)
	    tui.boardBuf[i * 2 + 1][2] = tui.board.nodes[i][0].isBlack?L'B':L'W';
	else
	    tui.boardBuf[i * 2 + 1][2] = L'\x251C';
	if(tui.board.nodes[i][tui.board.size - 1].head != NULL)
	    tui.boardBuf[i * 2 + 1][tui.board.size * 2] = tui.board.nodes[i][tui.board.size - 1].isBlack?L'B':L'W';
	else
	    tui.boardBuf[i * 2 + 1][tui.board.size * 2] = L'\x2524';
    }
    if(tui.board.nodes[0][0].head != NULL) /* corner drawing */
	tui.boardBuf[1][2] = tui.board.nodes[0][0].isBlack?L'B':L'W';
    else
	tui.boardBuf[1][2] = L'\x250C';
    if(tui.board.nodes[0][tui.board.size - 1].head != NULL) 
	tui.boardBuf[1][2 * tui.board.size] = tui.board.nodes[0][tui.board.size - 1].isBlack?L'B':L'W';
    else
	tui.boardBuf[1][2 * tui.board.size] = L'\x2510';
    if(tui.board.nodes[tui.board.size - 1][0].head != NULL) 
	tui.boardBuf[tui.board.size * 2 - 1][2] = tui.board.nodes[tui.board.size - 1][0].isBlack?L'B':L'W';
    else
	tui.boardBuf[tui.board.size * 2 - 1][2] = L'\x2514';
    if(tui.board.nodes[tui.board.size - 1][tui.board.size - 1].head != NULL) 
	tui.boardBuf[tui.board.size * 2 - 1][tui.board.size * 2] = tui.board.nodes[tui.board.size - 1][tui.board.size - 1].isBlack?L'B':L'W';
    else
	tui.boardBuf[tui.board.size * 2 - 1][tui.board.size * 2] = L'\x2518';
    for(int i = 1; i < tui.board.size - 1; i++)/* inside part of board drawing */
	for(int j = 1; j < tui.board.size - 1; j++)
	    if(tui.board.nodes[j][i].head != NULL)
		tui.boardBuf[j * 2 + 1][i * 2 + 2] = tui.board.nodes[j][i].isBlack?L'B':L'W';
	    else
		tui.boardBuf[j * 2 + 1][i * 2 + 2] = L'\x253C';
}
/* updates right panel screen buffer */
void setRightPanelBuf(){
    memset(&(tui.rightPanelBuf), 0, sizeof(tui.rightPanelBuf));
    if(tui.board.passes != 2)
	mbstowcs(&(tui.rightPanelBuf[1][1]), "PASS", 4);
    else
	mbstowcs(&(tui.rightPanelBuf[1][1]), "DONE", 4);
    mbstowcs(&(tui.rightPanelBuf[4][1]), "UNDO", 4);
    mbstowcs(tui.rightPanelBuf[7], "B CAPT", 6);
    mbstowcs(tui.rightPanelBuf[11], "W CAPT", 6);
    for(int i = 0; i < 4; i += 3){
	tui.rightPanelBuf[i][0] = L'\x250C';
	tui.rightPanelBuf[i + 2][0] = L'\x2514';
	tui.rightPanelBuf[i][5] = L'\x2510';
	tui.rightPanelBuf[i + 2][5] = L'\x2518';
	tui.rightPanelBuf[i + 1][0] = L'\x2502';
	tui.rightPanelBuf[i + 1][5] = L'\x2502';
	for(int j = 1; j < 5; j++){
	    tui.rightPanelBuf[i][j] = L'\x2500';
	    tui.rightPanelBuf[i + 2][j] = L'\x2500';
	}
    }
    swprintf(tui.rightPanelBuf[9], 6, L"  %d\0", tui.board.bRemoved);
    swprintf(tui.rightPanelBuf[13], 6, L"  %d\0", tui.board.wRemoved);
}
/* updates cmdLine screen buffer */
void setCmdLineBuf(){
}
/* handles mouse and keyboard and resize events:
   ':' activates command line
   Esc deactivates command line (slowly)
   Enter enters command if command line is active
   mouse clicks makes moves on board 
     on resize event redraws screen */
void handleEvents(){
    tui.cmdLine.active = false;
    bool escOrAltPressed = false;
    wint_t key = 0;
    MEVENT event;
    mousemask(ALL_MOUSE_EVENTS, NULL);
    while (/*key != 27 ESC*/true){
	int cntKeyEvent = get_wch(&key);
	switch(cntKeyEvent){
	    case OK:{
		switch(key){
		    case 27:/* ESC */
			escOrAltPressed = true;
			break;
		    case 10:/* Enter */
			if(escOrAltPressed)
			    escOrAltPressed = false;
			if(tui.cmdLine.active)
			    parseCmdLineBuffer();
			    /* mvprintw(0, 0, "Enter was pressed"); */
			break;
		    default: /* if cmd is active, put char into buf if buf not full */
			if(escOrAltPressed)
			    escOrAltPressed = false;
			if(tui.cmdLine.active == false){
			    if(key == L':'){
				curs_set(1);
				tui.cmdLine.active = true;
				redrawTUI();
				break;
			    } else break;
			}
			if(tui.cmdLine.bufLen == MAXIMUM_COMMAND_LENGTH + PATH_MAX + 1)
			    break; /* Expecting Enter or Esc or char removing, nothing else */
			else {
			    tui.cmdLine.buf[tui.cmdLine.bufLen] = key;
			    tui.cmdLine.buf[tui.cmdLine.bufLen + 1] = L'\0';
			    tui.cmdLine.bufLen++;
			    redrawTUI();
			    break;
			}
		}
	    }
	    break;
	    case ERR:
	    	if(escOrAltPressed)/* Esc was pressed */
		    escOrAltPressed = false; /* reset Esc / Alt checker */
		else
		    break; /* halfdelay enabled, ERR's means nothing without ESC */
		if(tui.cmdLine.active == false)
		    break;/* discard useless Esc */
		else {
		    tui.cmdLine.bufLen = 0;
		    tui.cmdLine.buf[0] = L'\0';
		    curs_set(0);
		    tui.cmdLine.active = false;
		    redrawTUI();
		    break;
		}
	    case KEY_CODE_YES:
		switch(key){
		    case KEY_RESIZE:
			redrawTUI();
			break;
		    case KEY_MOUSE:
			if(getmouse(&event) == OK)
			    if((event.bstate & BUTTON1_DOUBLE_CLICKED) ||
				    (event.bstate & BUTTON1_CLICKED) ||
				    (event.bstate & BUTTON1_TRIPLE_CLICKED)){
				if(0 < event.y && event.y < tui.board.size * 2 && event.y&1 && 1 < event.x && event.x < tui.board.size * 2 +1 && (event.x % 2 == 0)){
				    if(tui.board.passes != 2){
					if(makeMove(&(tui.board.nodes[(event.y + 1) / 2 - 1][event.x / 2 - 1]))) {
					    redrawTUI(); /* caught and processed proper move on the board */
					    break;
					}
				    } else if(tui.board.done == false){
					if(removeGroupForScoring(&(tui.board.nodes[(event.y + 1) / 2 - 1][event.x / 2 - 1]))) {
					    redrawTUI(); /* removed dead group from the board , update view */
					    break;
					}
				    }
				}
				if(event.y < 3 && event.x > tui.board.size * 2 + 3 && event.x < tui.board.size * 2 + 10) {
				    if(tui.board.passes != 2)
					makeMove(NULL); /* caught pass */
				    else if(tui.board.done == false){
					countTerritory();
					tui.board.done = true;
				    }
				    redrawTUI();
				    break;
				}
				if(event.y < 6 && event.y > 2 && event.x > tui.board.size * 2 + 3 && event.x < tui.board.size * 2 + 10) {
				    undoMove();
				    redrawTUI();
				    break;
				}
			    }
			break;
		    case KEY_BACKSPACE:
			if((tui.cmdLine.active == false) || (tui.cmdLine.bufLen == 0))
			    break;
			else {
			    tui.cmdLine.buf[tui.cmdLine.bufLen - 1] = L'\0';
			    tui.cmdLine.bufLen--;
			    redrawTUI();
			    break;
			}
		    default:
			break;
	    }
		break;
	    default:
		break;
	}
	/* mvprintw(cntRow, 0, "char:%lc code:%d", key, key);*/
    }
}

int main(){
    if(!setlocale(LC_ALL,"en_US.UTF-8")){
	endwin();
	fprintf(stderr, "Cannot set proper locale, aborting\n");
	return EXIT_FAILURE;
    }
    initscr(); /* start curses mode */
    if(has_colors() == FALSE){	
	endwin();
	printf("Your terminal does not support color\n");
	exit(1);
    }
    start_color();
    if(cbreak() == ERR){
	endwin();
	fprintf(stderr, "Cannot set cbreak mode, aborting\n");
	return EXIT_FAILURE;
    }
    if(halfdelay(10) == ERR){
	endwin();
	fprintf(stderr, "Cannot set input delay to 0,5 seconds, aborting\n");
	return EXIT_FAILURE;
    }
    if(ERR == noecho()){
	endwin();
	fprintf(stderr, "Cannot set off echo\n");
	return EXIT_FAILURE;
    }
    if(ERR == keypad(stdscr, TRUE)){
	endwin();
	fprintf(stderr, "Cannot activate keypad\n");
	return EXIT_FAILURE;
    }
    if(use_default_colors() == ERR){
	endwin();
	fprintf(stderr, "Cannot use default colors\n");
	return EXIT_FAILURE;
    }
    init_pair(1, COLOR_RED, -1); /* color pair for red text */
    initBoard(19); /* safe init */
    curs_set(0);
    redrawTUI();
    handleEvents();

    endwin();

    return EXIT_SUCCESS;
}
