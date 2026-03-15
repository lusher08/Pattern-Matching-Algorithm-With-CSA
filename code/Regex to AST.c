#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// Token types
typedef enum { CHAR, STAR, OR, CONCAT, GROUP } NodeType;

// Abstract Syntax Tree (AST) Node
typedef struct Node {
    NodeType type;
    char value;  // Used for CHAR nodes
    struct Node *left, *right;
} Node;

// Global variables
const char *input;
int pos = 0;

// Function prototypes
Node* parse_regex();
Node* parse_term();
Node* parse_factor();
Node* parse_base();
Node* create_node(NodeType type, char value, Node *left, Node *right);
void print_ast(Node *node, int depth);
void free_ast(Node *node);

// Create a new AST node
Node* create_node(NodeType type, char value, Node *left, Node *right) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->type = type;
    node->value = value;
    node->left = left;
    node->right = right;
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
    while (input[pos] == '*') {
        pos++;
        node = create_node(STAR, '*', node, NULL);
    }
    return node;
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

// Print AST in a readable format
void print_ast(Node *node, int depth) {
	int i; 
    if (!node) return;
    for (i = 0; i < depth; i++) printf("  ");
    switch (node->type) {
        case CHAR:    printf("CHAR(%c)\n", node->value); break;
        case STAR:    printf("STAR\n"); break;
        case OR:      printf("OR\n"); break;
        case CONCAT:  printf("CONCAT\n"); break;
        case GROUP:   printf("GROUP\n"); break;
    }
    print_ast(node->left, depth + 1);
    print_ast(node->right, depth + 1);
}

// Free memory allocated for AST
void free_ast(Node *node) {
    if (!node) return;
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}

// Main function
int main() {
    char regex[100];
    printf("Enter regular expression: ");
    scanf("%s", regex);
    input = regex;
    pos = 0;

    Node *ast = parse_regex();
    printf("\nParsed AST:\n");
    print_ast(ast, 0);

    free_ast(ast);
    return 0;
}

