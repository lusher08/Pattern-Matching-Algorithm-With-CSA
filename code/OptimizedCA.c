// if high_guard = -1, this is inf
// if low_guard = -1, this is dead node 

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

// Node types (a-z, *, |, ...)
typedef enum {CHAR, STAR, OR, CONCAT} NodeType;

// Counting Automata State
typedef struct State {
	struct State **followPos;
	int size, capacity, low_guard, high_guard, label;
	char symbol;
} State;

// Abstract Syntax Tree (AST) Node
typedef struct Node {
    NodeType type;
    char symbol;  
    int low_guard, high_guard, label;
    struct Node *left, *right;
    State state;
} Node;

// Global variables
const char *input;
int pos = 0;
bool halt = false;
bool star = false;

// AST Parsing Function
int parse_bounds(int *low, int *high);
void ast_pos(Node *node);
Node* parse_regex();
Node* parse_term();
Node* parse_factor();
Node* parse_base();
Node* create_node(NodeType type, char symbol, Node *left, Node *right);

// Optimizing Node Function
void set_guard(Node **prev_sym_node, Node *node, bool split);
bool compare(Node **prev_sym_node, Node *node, bool split);
void optimize_ast(Node **node);
void print_ast(Node *node, int depth);
void free_ast(Node *node);

// Optimizing Tree Function
bool tree_cmp(Node *lroot, Node *rroot);
bool merge_tree(Node **ltree, Node **rtree);
bool merge_chk(Node **ltree, Node **rtree);
void optimize_pattern(Node **node);

// Creating Automata Function
State create_state(Node *node);
void init_state(Node *node);
void add_followpos(State *from, State *to);
void print_state(Node *node);


// Create a new AST node
Node* create_node(NodeType type, char symbol, Node *left, Node *right) {
    Node *node   = (Node*)malloc(sizeof(Node));
    node->type   = type;
    node->symbol = symbol;
    node->low_guard  = 1;
    node->high_guard = 1;
    node->left   = left;
    node->right  = right;
    node->label  = pos;
    return node;
}

// Parse a full regex expression (E -> T | E)
Node* parse_regex() {
    Node *left = parse_term();
    if (input[pos] == '|') {
        pos++;
        Node *right = parse_regex();
        return create_node(OR, '|', left, right);
    }
    return left;
}

// Parse a term (T -> F T | F)
Node* parse_term() {
    Node *left = parse_factor();
    while (input[pos] && input[pos] != '|' && input[pos] != ')') {
        Node *right = parse_factor();
        left = create_node(CONCAT, '.', left, right);
    }
    return left;
}

// Parse a factor (F -> B* | B)
Node* parse_factor() {
    Node *node = parse_base();

    // Check for bounded repetition {m,n}
    if (input[pos] == '{') {
        int low = 0, high = 0;
        if (parse_bounds(&low, &high)) {
            node->low_guard = low;
            node->high_guard = high;
        }
    }

    // Handle kleene star after base or bounded node
    while (input[pos] == '*') {
        pos++;
        node = create_node(STAR, '*', node, NULL);
    }
    return node;
}

// Helper to parse bounds like {3,4}
int parse_bounds(int *low, int *high) {
    pos++; // Skip '{'

    *low = 0;
    while (isdigit(input[pos])) *low = *low * 10 + input[pos++] - '0';

	if (input[pos] == '}') { // Check if {n}
		*high = *low;
		pos++;
		return 1;
	}
    else if (input[pos] == ',') pos++; // Skip ','

    *high = 0;
	while (isdigit(input[pos])) *high = *high * 10 + input[pos++] - '0';

    pos++; // Skip '}'
    return 1;
}


// Parse a base unit (B -> (E) | CHAR)
Node* parse_base() {
    if (input[pos] == '(') {
        pos++;  // Skip '('
        Node *node = parse_regex();
        if (input[pos] == ')') pos++;  // Skip ')'
        return node;
    }
    if (isalnum(input[pos])) {
        return create_node(CHAR, input[pos++], NULL, NULL);
    }
    return NULL;  // Invalid character
}

void ast_pos(Node *node){
	if(!node) return;
	node->label = pos++;
	ast_pos(node->left);
	ast_pos(node->right);
	return;
}

// Print AST in a readable format
void print_ast(Node *node, int depth) {
	int i; 
    if (!node) return;
    for (i = 0; i < depth; i++) printf("  ");
    switch (node->type) {
        case CHAR:    printf("SYMBOL(%c) | GUARD VALUE : %d, %d | LABEL : %d\n", node->symbol, node->low_guard, node->high_guard, node->label); break;
        case STAR:    printf("STAR | LABEL : %d\n", node->label); break;
        case OR:      printf("OR | LABEL : %d\n", node->label); break;
        case CONCAT:  printf("CONCAT | LABEL : %d\n", node->label); break;
    }
    print_ast(node->left, depth + 1);
    print_ast(node->right, depth + 1);
}

// Set guard for each node
void set_guard(Node **prev_sym_node, Node *node, bool split) {
	if (!node) return;
	switch (node->type) {
		case CHAR:    split = compare(prev_sym_node, node, split); break;
        case STAR:    split = true; break;
        case OR:      
        case CONCAT:  if (split || node->type == OR) *prev_sym_node = NULL;	
					  split = false; break;
	}
	set_guard(prev_sym_node, node->left, split);
	if (node->type == OR) *prev_sym_node = NULL;
	set_guard(prev_sym_node, node->right, split);
}

// Compare and remove unnecessary node
bool compare(Node **prev_sym_node, Node *node, bool split) {
	if (!*prev_sym_node) {
		*prev_sym_node = node;
	} 
	else if (node->symbol == (*prev_sym_node)->symbol) {
		node->low_guard += (*prev_sym_node)->low_guard;
		
		// if one of node has star
		if ((*prev_sym_node)->high_guard == -1 || node->high_guard == -1) {
			node->high_guard = -1;
	}
		else node->high_guard += (*prev_sym_node)->high_guard;
		(*prev_sym_node)->low_guard = -1; // kill the previous symbol node
	}
	if (split) {
		node->high_guard = -1; // inf value
		node->low_guard -= 1; 
		split = false;
	}
	*prev_sym_node = node;
	return split;
}

// Remove unnecessary node from the tree
void optimize_ast(Node **node) {
	if (!node || !*node) return;
	optimize_ast(&(*node)->left);
    optimize_ast(&(*node)->right);
    
    Node *left = (*node)->left;
    Node *right = (*node)->right;
	if ((*node)->low_guard == -1) *node = NULL;
	else if ((*node)->type != CHAR && !left && !right) *node = NULL;
	else if ((*node)->type == STAR && left && left->type == CHAR) *node = left; // a* -> a{0,inf)
	else if ((*node)->type == STAR && right && right->type == CHAR) *node = right; 
	else if ((*node)->type == CONCAT) {
		if (!left && right) *node = right;
		else if (left && !right) *node = left;
	}
}

// Compare sub-tree
bool tree_cmp(Node *lroot, Node *rroot){
	bool left, right;
	if (lroot->symbol != rroot->symbol) return false;
	
	if (lroot->left && rroot->left) left = tree_cmp(lroot->left, rroot->left);
	else left = true;
	if (lroot->right && rroot->right) right = tree_cmp(lroot->right, rroot->right);
	else right = true;
	
	if (left && right) return true;
	else return false;
}

int sep = 0;
// merge left/right sub-tree
bool merge_chk(Node **ltree, Node **rtree){
	if (!*ltree || !*rtree || !ltree || !rtree) return true;
	
	bool left, right;
	left = merge_chk(&(*ltree)->left, &(*rtree)->left);
	right = merge_chk(&(*ltree)->right, &(*rtree)->right);
	int lgap = (*ltree)->low_guard - (*rtree)->high_guard; // R<<L
	int rgap = (*rtree)->low_guard - (*ltree)->high_guard; // L<<<R
	// if there's more than 1 gap, it shouldn't be combined. like a{3,4}b|aaaaabb
	if (lgap==1) sep += 1;
	if (rgap==1) sep += 1;
	if (((*ltree)->low_guard < (*rtree)->low_guard && (*ltree)->high_guard > (*rtree)->high_guard) 
		||((*ltree)->low_guard < (*rtree)->low_guard && (*ltree)->high_guard > (*rtree)->high_guard)) return true;
	else if(((abs(rgap)>1 && (*ltree)->high_guard!=-1) && (abs(lgap)>1 && (*rtree)->high_guard!=-1))||sep>1) return false;
	else if (left == false || right == false) return false;
	else return true;
}

// merge left/right sub-tree
bool merge_tree(Node **ltree, Node **rtree){
	if (!*ltree || !*rtree || !ltree || !rtree) return true;
	
	bool left, right;
	left = merge_tree(&(*ltree)->left, &(*rtree)->left);
	right = merge_tree(&(*ltree)->right, &(*rtree)->right);

	(*ltree)->low_guard = ((*ltree)->low_guard < (*rtree)->low_guard) ? (*ltree)->low_guard : (*rtree)->low_guard;
	if ((*ltree)->high_guard == -1 || (*rtree)->high_guard == -1) (*ltree)->high_guard = -1; // in case inf
	else (*ltree)->high_guard = ((*ltree)->high_guard > (*rtree)->high_guard) ? (*ltree)->high_guard : (*rtree)->high_guard;
	
	return true;
}

// Compress unnecessary sub-tree (repeating tree pattern)
void optimize_pattern(Node **node) {
	if (!node || !*node) return;
	
	optimize_ast(&(*node));
	optimize_pattern(&(*node)->left);
    optimize_pattern(&(*node)->right);
	
	Node *left = (*node)->left;
    Node *right = (*node)->right; 
    bool similar_tree;
	if (left && right){ // if there's both sub tree,
		similar_tree = tree_cmp(left, right);
		if(similar_tree && (*node)->type == OR) {
			sep = 0;
			similar_tree = merge_chk(&left, &right);
			if(similar_tree) similar_tree = merge_tree(&left, &right);
			if(similar_tree) *node = left;
		}
		return;
	}
	return;
}


// 두자리수 처리중. 더블 포인터로 바꾸고 char 타입을 string 으로 바꾸면됨 
/*
// Helper function to create string with parentheses if needed
char* with_parens(const char* s) {
    size_t len = strlen(s);
    char* out = (char*)malloc(len + 3);
    sprintf(out, "(%s)", s);
    return out;
}

// Recursively convert tree to regex string
char* regex_from_tree(Node* node) {
    if (!node) return NULL;

    char *left_str, *right_str, *result;

    switch (node->type) {
        case CHAR:
        	pos = 0;
            if (node->high_guard != 1){
            	result = (char*)malloc(1);
            	if (node->high_guard == node->low_guard{

				)
            	result = (char*)malloc(7);
            	result[0] = node->symbol;
            	result[1] = '{';
            	result[2] = '0' + node->low_guard;
            	result[3] = ',';
            	result[4] = '0' + node->high_guard; // if inf, (?,/)
            	result[5] = '}';
            	result[6] = '\0';
			}
			else{
				result = (char*)malloc(2);
            	result[0] = node->symbol;
            	result[1] = '\0';
			}
            return result;
            

        case STAR: {
            left_str = regex_from_tree(node->left);
            int need_parens = (node->left->type == OR || node->left->type == CONCAT);
            if (need_parens) {
                char* tmp = with_parens(left_str);
                free(left_str);
                left_str = tmp;
            }
            result = (char*)malloc(strlen(left_str) + 2);
            sprintf(result, "%s*", left_str);
            free(left_str);
            return result;
        }

        case OR:
            left_str = regex_from_tree(node->left);
            right_str = regex_from_tree(node->right);
            result = (char*)malloc(strlen(left_str) + strlen(right_str) + 2);
            sprintf(result, "%s|%s", left_str, right_str);
            // wrap OR in parentheses to preserve grouping when nested in STAR/CONCAT
            char* or_with_parens = with_parens(result);
            free(left_str);
            free(right_str);
            free(result);
            return or_with_parens;

        case CONCAT:
            left_str = regex_from_tree(node->left);
            right_str = regex_from_tree(node->right);
            result = (char*)malloc(strlen(left_str) + strlen(right_str) + 1);
            sprintf(result, "%s%s", left_str, right_str);
            free(left_str);
            free(right_str);
            return result;
    }

    return NULL; // should not reach here
}
*/

// Free memory allocated for AST
void free_ast(Node *node) {
    if (!node) return;
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}

// Create state for desired node
State create_state(Node *node) {
	State state;
	state.followPos = malloc(sizeof(State*));
	state.size      = 0;
	state.capacity  = 1;
	state.symbol    = node->symbol;
	state.low_guard = node->low_guard;
	state.high_guard = node->high_guard;
	state.label     = node->label;
	return state;
}

// Set state for each node(along self-follow state)
void init_state(Node *node){
	if (!node) return;
	init_state(node->left);
	init_state(node->right);
	if (node->type == CHAR) {
		node->state = create_state(node);
		if (node->high_guard > 1 || node->high_guard == -1) add_followpos(&node->state, &node->state);
	}
	return;
}

// Add FollowPos to the State
void add_followpos(State *from, State *to) {
	printf("%d %d\n",from->label,to->label);
    if (from->size >= from->capacity) {
        from->capacity *= 2;
        from->followPos = realloc(from->followPos, from->capacity * sizeof(State*));
        if (!from->followPos) {
            perror("Failed to realloc");
            exit(EXIT_FAILURE);
        }
    }
    from->followPos[from->size++] = to;
}

// Print Counting Automata State
void print_state(Node *node){
	if (!node) return;
	print_state(node->left);
	print_state(node->right);
	if (node->type == CHAR) {
		printf("STATE | SYMBOL : %c | GUARD VALUE : %d, %d | LABEL : %d\n", 
		node->state.symbol, node->state.low_guard, node->state.high_guard, node->state.label);
	}
	return;
}

// Main function
int main() {
	// initialize
    char regex[100];
    printf("Enter regular expression: ");
    scanf("%s", regex);
    input = regex;
    pos = 0;
    
	// parsing regex
    Node *ast = parse_regex();
    pos = 1;
    ast_pos(ast);
    printf("\nParsed AST:\n");
    print_ast(ast, 0);
    
    // optimizing ast node
    Node *prev_sym_node = NULL;
    set_guard(&prev_sym_node, ast, false);
    optimize_ast(&ast);
    printf("\nNode-Optimized AST:\n");
    print_ast(ast, 0);
    
    // optimizing ast tree pattern
    // (aa*a{3,4}b{5}|ab)는 b 수 불일치에 의해 축약 불가 /  (aa*a{3,4}b{5}|ab*)는 (a(4,inf)b(0,inf))로 축약 가능 
    optimize_pattern(&ast);
    printf("\nTree-Optimized AST:\n");
    print_ast(ast, 0);

    free_ast(ast);
    return 0;
}

