#include <assert.h>
#include "mcc.h"

NODE *new_node(NODE_KIND kind, TYPE *typ)
{
    NODE *np = (NODE*) alloc(sizeof (NODE));
    np->kind = kind;
    np->type = typ;
    return np;
}

NODE *new_node1(NODE_KIND kind, TYPE *typ, NODE *n1)
{
    NODE *np = new_node(kind, typ);
    np->u.link.n1 = n1;
    return np;
}

NODE *new_node2(NODE_KIND kind, TYPE *typ, NODE *n1, NODE *n2)
{
    NODE *np = new_node(kind, typ);
    np->u.link.n1 = n1;
    np->u.link.n2 = n2;
    return np;
}

NODE *new_node3(NODE_KIND kind, TYPE *typ, NODE *n1, NODE *n2, NODE *n3)
{
    NODE *np = new_node(kind, typ);
    np->u.link.n1 = n1;
    np->u.link.n2 = n2;
    np->u.link.n3 = n3;
    return np;
}

NODE *new_node4(NODE_KIND kind, TYPE *typ, NODE *n1, NODE *n2, NODE *n3, NODE *n4)
{
    NODE *np = new_node(kind, typ);
    np->u.link.n1 = n1;
    np->u.link.n2 = n2;
    np->u.link.n3 = n3;
    np->u.link.n4 = n4;
    return np;
}

NODE *link_node(NODE_KIND kind, NODE *node, NODE *top)
{
    NODE *np;
    NODE *p;

    np = new_node(kind, NULL);
    np->u.link.n1 = node;
    np->u.link.n2 = NULL;
    if (top == NULL)
        return np;
    for (p = top; p->u.link.n2 != NULL; p = p->u.link.n2)
        ;
    p->u.link.n2 = np;
    return top;
}

NODE *new_node_sym(NODE_KIND kind, SYMBOL *sym)
{
    NODE *np;
    np = new_node(kind, sym->type);
    np->u.sym = sym;
    return np;
}

NODE *new_node_int(NODE_KIND kind, int num)
{
    NODE *np;
    np = new_node(kind, &g_type_int);
    np->u.num = num;
    return np;
}

bool node_can_take_addr(const NODE *np)
{
    return (np != NULL && np->kind == NK_ID);
}

const char *node_kind_to_str(NODE_KIND kind)
{
    switch (kind) {
    case NK_ASSIGN: return "=";
    case NK_LOR:    return "||";
    case NK_LAND:   return "&&";
    case NK_EQ:     return "==";
    case NK_NEQ:    return "!=";
    case NK_LT:     return "<";
    case NK_GT:     return ">";
    case NK_LE:     return "<=";
    case NK_GE:     return ">=";
    case NK_ADD:    return "+";
    case NK_SUB:    return "-";
    case NK_MUL:    return "*";
    case NK_DIV:    return "/";
    case NK_ADDR:   return "&";
    case NK_INDIR:  return "*";
    case NK_MINUS:  return "-";
    case NK_NOT:    return "!";
    default:        assert(0);
    }
    return "";
}

void print_node(NODE *np)
{
    if (np == NULL) {
/*
        printf("<NULL>");
*/
        return;
    }
    switch (np->kind) {
    case NK_COMPOUND:
        print_node(np->u.link.n1);
        print_node(np->u.link.n2);
        break;
    case NK_IF:
        printf("if (");
        print_node(np->u.link.n1);
        printf(")\n");
        print_node(np->u.link.n2);
        printf("\n");
        if (np->u.link.n3) {
            printf("else\n");
            print_node(np->u.link.n3);
        }
        break;
    case NK_WHILE:
        printf("while (");
        print_node(np->u.link.n1);
        printf(")\n");
        print_node(np->u.link.n2);
        break;
    case NK_FOR:
        printf("for (");
        print_node(np->u.link.n1);
        printf("; ");
        print_node(np->u.link.n2);
        printf("; ");
        print_node(np->u.link.n3);
        printf(")\n");
        print_node(np->u.link.n4);
        break;
    case NK_CONTINUE:
        printf("continue;\n");
        break;
    case NK_BREAK:
        printf("break;\n");
        break;
    case NK_RETURN:
        printf("return ");
        if (np->u.link.n1)
            print_node(np->u.link.n1);
        printf(";\n");
        if (is_verbose_level(1)) {
            printf(" : ");
            print_type(np->type);
            printf("\n");
        }
        break;
    case NK_EXPR:
        print_node(np->u.link.n1);
        printf(";\n");
        if (is_verbose_level(1)) {
            printf(" : ");
            print_type(np->type);
            printf("\n");
        }
        break;
    case NK_ASSIGN:
    case NK_LOR:
    case NK_LAND:
    case NK_EQ:
    case NK_NEQ:
    case NK_LT:
    case NK_GT:
    case NK_LE:
    case NK_GE:
    case NK_ADD:
    case NK_SUB:
    case NK_MUL:
    case NK_DIV:
        printf("(");
        print_node(np->u.link.n1);
        printf(" %s ", node_kind_to_str(np->kind));
        print_node(np->u.link.n2);
        printf(")");
        if (is_verbose_level(1)) {
            printf(" : ");
            print_type(np->type);
            printf("\n");
        }
        break;
    case NK_ADDR:
    case NK_INDIR:
    case NK_MINUS:
    case NK_NOT:
        printf("(%s", node_kind_to_str(np->kind));
        print_node(np->u.link.n1);
        printf(")");
        if (is_verbose_level(1)) {
            printf(" : ");
            print_type(np->type);
            printf("\n");
        }
        break;
    case NK_ID:
        assert(np->u.sym);
        printf("%s", np->u.sym->id);
        if (is_verbose_level(1)) {
            printf(" : ");
            print_type(np->type);
            printf("\n");
        }
        break;
    case NK_INT_LIT:
        printf("%d", np->u.num);
        if (is_verbose_level(1)) {
            printf(" : ");
            print_type(np->type);
            printf("\n");
        }
        break;
    case NK_CALL:
        print_node(np->u.link.n1);
        printf("(");
        print_node(np->u.link.n2);
        printf(")");
        if (is_verbose_level(1)) {
            printf(" : ");
            print_type(np->type);
            printf("\n");
        }
        break;
    case NK_ARG:
        print_node(np->u.link.n1);
        if (np->u.link.n2) {
            printf(", ");
            print_node(np->u.link.n2);
        }
        break;
    }
}

