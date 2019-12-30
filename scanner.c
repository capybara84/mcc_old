#include <string.h>
#include <ctype.h>
#include "mcc.h"

/* string buffer / number parse buffer */
#define MAX_BUFFER  256

typedef struct ident {
    struct ident *next;
    char *id;
} IDENT;

static IDENT *s_ident_top = NULL;

char* new_ident(const char *s)
{
    IDENT *p = (IDENT*) alloc(sizeof (IDENT));
    p->id = strdup(s);
    p->next = s_ident_top;
    s_ident_top = p;
    return p->id;
}

void print_ident(void)
{
    IDENT *id = s_ident_top;
    for (; id != NULL; id = id->next)
        printf("%p:%s\n", id, id->id);
}

char *intern(const char *s)
{
    IDENT *id = s_ident_top;

/*
    print_ident();
*/

    for (; id != NULL; id = id->next)
        if (strcmp(id->id, s) == 0)
            return id->id;
    return new_ident(s);
}

SCANNER *open_scanner_text(const char *filename, const char *text)
{
    SCANNER *s = (SCANNER*) alloc(sizeof (SCANNER));
    s->source = text;
    s->size = strlen(text);
    s->pos = 0;
    s->ch = ' ';
    s->line = 1;
    s->filename = filename;
    s->num = 0;
    s->id = NULL;
    return s;
}

SCANNER *open_scanner(const char *filename)
{
    FILE *fp;
    size_t size;
    char *s;

    fp = fopen(filename, "r");
    if (fp == NULL)
        return NULL;

    if (fseek(fp, 0, SEEK_END) != 0)
        return NULL;

    size = ftell(fp);

    if (fseek(fp, 0, SEEK_SET) != 0)
        return NULL;

    s = (char*) alloc(size * sizeof (char));
    if (fread(s, size, 1, fp) != 1) {
        fclose(fp);
        free(s);
        return NULL;
    }
    fclose(fp);
    return open_scanner_text(filename, s);
}

bool close_scanner(SCANNER *s)
{
    if (s == NULL)
        return false;
    free(s);
    return true;
}

static int is_white(int ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'
        || ch == '\f' || ch == '\v';
}

static int is_alpha(int ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

static int is_alnum(int ch)
{
    return is_alpha(ch) || isdigit(ch);
}

static int next_char(SCANNER *scan)
{
    if (scan->ch == '\n')
        scan->line++;
    scan->ch = scan->source[scan->pos++];

printf("%s(%d):next_char: '%c'\n", scan->filename, scan->line, scan->ch);

    return scan->ch;
}

static TOKEN scan_id(SCANNER *scan)
{
    char buffer[MAX_IDENT+1];
    int i = 0;
    while (is_alnum(scan->ch)) {
        if (i < MAX_IDENT)
            buffer[i++] = scan->ch;
        next_char(scan);
    }
    buffer[i] = '\0';
    /*TODO keywords */
    scan->id = intern(buffer);
    return TK_ID;
}

static TOKEN scan_num(SCANNER *scan)
{
    char buffer[MAX_BUFFER+1];
    int i = 0;
    while (isdigit(scan->ch)) {
        if (i < MAX_BUFFER)
            buffer[i++] = scan->ch;
        next_char(scan);
    }
    buffer[i] = '\0';
    scan->num = atoi(buffer);
    /* TODO unsigned, long, float, double */
    return TK_INT_LIT;
}

TOKEN next_token(SCANNER *scan)
{
    for (;;) {
        while (is_white(scan->ch))
            next_char(scan);
        if (scan->ch == '\0')
            return TK_EOF;
        if (is_alpha(scan->ch))
            return scan_id(scan);
        if (isdigit(scan->ch))
            return scan_num(scan);
        error(scan->filename, scan->line,
            (isprint(scan->ch) ? "illegal character '%c'"
                              : "illegal character (code=%02d)"), scan->ch);
        next_char(scan);
    }
}