#include <assert.h>
#include "mcc.h"

#define COUNT_OF(array) (sizeof (array) / sizeof (array[0]))

#ifdef NDEBUG
# define ENTER(fn)      ((void) 0)
# define LEAVE(fn)      ((void) 0)
# define TRACE(fn)      ((void) 0)
#else
static int s_indent = 0;
# define ENTER(fn)  \
        if (is_verbose_level(3)) printf("%*sENTER %s\n", s_indent++, "", (fn))
# define LEAVE(fn)  \
        if (is_verbose_level(3)) printf("%*sLEAVE %s\n", --s_indent, "", (fn))
# define TRACE(fn)  \
        if (is_verbose_level(3)) printf("%*sTRACE %s\n", s_indent, "", (fn))
#endif

#ifdef NDEBUG
#define next(pars)  ((pars)->token = next_token((pars)->scan))
#else
static TOKEN next(PARSER *pars)
{
    pars->token = next_token(pars->scan);
    if (is_verbose_level(2)) {
        printf("%s(%d): %s\n", pars->scan->filename, pars->scan->line,
            scan_token_to_string(pars->scan, pars->token));
    }
    return pars->token;
}
#endif

PARSER *open_parser_text(const char *filename, const char *text)
{
    PARSER *pars = (PARSER*) alloc(sizeof (PARSER));
    pars->scan = open_scanner_text(filename, text);
    if (pars->scan == NULL) {
        close_parser(pars);
        return NULL;
    }
    pars->token = TK_EOF;
    return pars;
}

PARSER *open_parser(const char *filename)
{
    PARSER *pars = (PARSER*) alloc(sizeof (PARSER));
    pars->scan = open_scanner(filename);
    if (pars->scan == NULL) {
        close_parser(pars);
        return NULL;
    }
    pars->token = TK_EOF;
    return pars;
}

bool close_parser(PARSER *pars)
{
    if (pars == NULL)
        return false;
    if (!close_scanner(pars->scan))
        return false;
    free(pars);
    return true;
}

static void parser_error(PARSER *pars, const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    verror(pars->scan->filename, pars->scan->line, s, ap);
    va_end(ap);
    exit(1);
}

static bool expect(PARSER *pars, TOKEN tk)
{
    if (pars->token == tk) {
        next(pars);
        return true;
    }
    parser_error(pars, "missing token %s", scan_token_to_string(pars->scan, tk));
    return false;
}


static bool is_token_begin_with(PARSER *pars, TOKEN array[], int count)
{
    int i;
    for (i = 0; i < count; i++)
        if (pars->token == array[i])
            return true;
    return false;
}

static bool is_declaration_specifier(PARSER *pars)
{
    static TOKEN begin_with[] = { TK_STATIC, TK_EXTERN,
            TK_VOID, TK_INT, };
    return is_token_begin_with(pars, begin_with, COUNT_OF(begin_with));
}

static bool is_declaration(PARSER *pars)
{
    return is_declaration_specifier(pars);
}

static bool is_expression(PARSER *pars)
{
    static TOKEN begin_with[] = { TK_AND, TK_STAR, TK_MINUS, TK_NOT, TK_ID,
                    TK_INT_LIT, TK_LPAR};
    return is_token_begin_with(pars, begin_with, COUNT_OF(begin_with));
}

static bool is_statement(PARSER *pars)
{
    static TOKEN begin_with[] = { TK_BEGIN, TK_IF, TK_WHILE, TK_FOR,
            TK_WHILE, TK_FOR, TK_CONTINUE, TK_BREAK, TK_RETURN };
    if (is_expression(pars))
        return true;
    return is_token_begin_with(pars, begin_with, COUNT_OF(begin_with));
}


/*
expression
	= assignment_expression
*/
static void parse_expression(PARSER *pars)
{
    /*TODO*/
}

static void parse_declaration(PARSER *pars);
static void parse_statement(PARSER *pars);

/*
compound_statement
	= '{' [declaration_list] {statement} '}'
declaration_list
    = declaration {declaration}
*/
static void parse_compound_statement(PARSER *pars)
{
    ENTER("parse_compound_statement");
    next(pars); /* skip '{' */
    while (is_declaration(pars))
        parse_declaration(pars);
    while (is_statement(pars))
        parse_statement(pars);
    expect(pars, TK_END);
    LEAVE("parse_compound_statement");
}

/*
statement
	= expression_statement
	| compound_statement
	| selection_statement
	| iteration_statement
	| jump_statement

expression_statement
	= [expression] ';'

compound_statement
	= '{' [declaration_list] {statement} '}'

selection_statement
	= IF '(' expression ')' statement
	| IF '(' expression ')' statement ELSE statement

iteration_statement
	= WHILE '(' expression ')' statement
	| FOR '(' [expression] ';' [expression] ';' [expression] ')'
		statement

jump_statement
	= CONTINUE ';'
	| BREAK ';'
	| RETURN [expression] ';'

*/
static void parse_statement(PARSER *pars)
{
    ENTER("parse_statement");
    switch (pars->token) {
    case TK_BEGIN:
        parse_compound_statement(pars);
        break;
    case TK_IF:
        next(pars);
        expect(pars, TK_LPAR);
        parse_expression(pars);
        expect(pars, TK_RPAR);
        parse_statement(pars);
        if (pars->token == TK_ELSE) {
            next(pars);
            parse_statement(pars);
        }
        break;
    case TK_WHILE:
        next(pars);
        expect(pars, TK_LPAR);
        parse_expression(pars);
        expect(pars, TK_RPAR);
        parse_statement(pars);
        break;
    case TK_FOR:
        next(pars);
        expect(pars, TK_LPAR);
        if (is_expression(pars))
            parse_expression(pars);
        expect(pars, TK_SEMI);
        if (is_expression(pars))
            parse_expression(pars);
        expect(pars, TK_SEMI);
        if (is_expression(pars))
            parse_expression(pars);
        expect(pars, TK_RPAR);
        parse_statement(pars);
        break;
    case TK_CONTINUE:
        next(pars);
        expect(pars, TK_SEMI);
        break;
    case TK_BREAK:
        next(pars);
        expect(pars, TK_SEMI);
        break;
    case TK_RETURN:
        next(pars);
        if (is_expression(pars))
            parse_expression(pars);
        expect(pars, TK_SEMI);
        break;
    default:
        if (pars->token != TK_SEMI)
            parse_expression(pars);
        expect(pars, TK_SEMI);
        break;
    }
    LEAVE("parse_statement");
}

/*
declarator
	= {'*'} (IDENTIFIER | '(' declarator ')') ['(' ')']
*/
static void parse_declarator(PARSER *pars, TYPE **pptyp, char **id)
{
    TYPE *typ = NULL;

    ENTER("parse_declarator");

    assert(pptyp);
    assert(*pptyp);
    assert(id);

    while (pars->token == TK_STAR) {
        *pptyp = new_type(T_POINTER, (*pptyp)->sclass, *pptyp);
        next(pars);
    }
    if (pars->token == TK_ID) {
        *id = pars->scan->id;
        next(pars);
    } else if (pars->token == TK_LPAR) {
        typ = new_type(T_UNKNOWN, SC_DEFAULT, NULL);
        next(pars);
        parse_declarator(pars, &typ, id);
        expect(pars, TK_RPAR);
    } else {
        parser_error(pars, "syntax error");
    }
    if (pars->token == TK_LPAR) {
        next(pars);
        expect(pars, TK_RPAR);
        *pptyp = new_type(T_FUNC, (*pptyp)->sclass, *pptyp);
        if (typ) {
            TYPE *p = typ;
            while (p && p->type && p->type->kind != T_UNKNOWN)
                p = p->type;
            if (p->type) {
                if (p->type->kind == T_UNKNOWN) {
                    p->type = *pptyp;
                    *pptyp = typ;
                } else {
                    assert(0);
                }
            } else {
                assert(0);
            }
        }
    } else if (typ) {
        /*TODO*/
        parser_error(pars, "syntax error (mcc)");
    }
    LEAVE("parse_declarator");
}

/*
declaration_specifier
	= storage_class_specifier | type_specifier
storage_class_specifier
    = STATIC | EXTERN
type_specifier
	= VOID | INT
*/
static void parse_declaration_specifier(PARSER *pars, TYPE *typ)
{
    ENTER("parse_declaration_specifier");
    switch (pars->token) {
    case TK_STATIC:
        if (typ->sclass != SC_DEFAULT)
            parser_error(pars, "invalid 'static'");
        typ->sclass = SC_STATIC;
        next(pars);
        break;
    case TK_EXTERN:
        if (typ->sclass != SC_DEFAULT)
            parser_error(pars, "invalid 'extern'");
        typ->sclass = SC_EXTERN;
        next(pars);
        break;
    case TK_VOID:
        if (typ->kind != T_UNKNOWN)
            parser_error(pars, "cannot combine 'void'");
        typ->kind = T_VOID;
        next(pars);
        break;
    case TK_INT:
        if (typ->kind != T_UNKNOWN)
            parser_error(pars, "cannot combine 'int'");
        typ->kind = T_INT;
        next(pars);
        break;
    default:
        parser_error(pars, "syntax error");
    }
    LEAVE("parse_declaration_specifier");
}


/*
declaration_specifiers
    = declaration_specifier {declaration_specifier}
*/
static void parse_declaration_specifiers(PARSER *pars, TYPE *typ)
{
    ENTER("parse_declaration_specifiers");
    parse_declaration_specifier(pars, typ);
    while (is_declaration_specifier(pars)) {
        parse_declaration_specifier(pars, typ);
    }
    LEAVE("parse_declaration_specifiers");
}

/*
declaration
	= declaration_specifiers [declarator_list] ';'
*/
static void parse_declaration(PARSER *pars)
{
    TYPE *typ;

    ENTER("parse_declaration");

    typ = new_type(T_UNKNOWN, SC_DEFAULT, NULL);
    parse_declaration_specifiers(pars, typ);

    for (;;) {
        char *id;
        TYPE *ntyp = dup_type(typ);
        parse_declarator(pars, &ntyp, &id);

printf("id:%s type:", id);
print_type(ntyp);
printf("\n");

        if (pars->token != TK_COMMA)
            break;
        next(pars);
    }
    expect(pars, TK_SEMI);
    LEAVE("parse_declaration");
}

/*
external_declaration
    = declaration_specifiers declarator ';'
    | declaration_specifiers declarator compound_statement
*/
static NODE *parse_external_delaration(PARSER *pars)
{
    SYMBOL *sym;
    NODE *np;
    TYPE *typ;
    char *id;

    ENTER("parse_external_delaration");

    typ = new_type(T_UNKNOWN, SC_DEFAULT, NULL);
    parse_declaration_specifiers(pars, typ);

    parse_declarator(pars, &typ, &id);

    if (pars->token == TK_SEMI) {
        bool isfunc = typ->kind == T_FUNC;
        sym = new_symbol(isfunc ? SK_FUNC : SK_VAR, id, typ);
        np = new_node_sym(isfunc ? NK_FUNC_DECL : NK_VAR_DECL, sym, typ);
        next(pars);
    } else if (pars->token == TK_BEGIN) {
        sym = new_symbol(SK_FUNC, id, typ);
        np = new_node_sym(NK_FUNC_DECL, sym, typ);
        parse_compound_statement(pars);
    } else {
        np = NULL;
        parser_error(pars, "syntax error");
    }
    LEAVE("parse_external_delaration");
    return np;
}

/*
translation_unit
    = {external_declaration}
*/
NODE *parse(PARSER *pars)
{
    NODE *np = NULL;

    next(pars);
    while (pars->token != TK_EOF) {
        NODE *n = parse_external_delaration(pars);
        np = node_link(NK_DECL_LINK, np, n);
    }
    return np;
}

