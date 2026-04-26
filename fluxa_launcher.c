#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define FLUXA_VERSION "0.4.0"
#define MAX_PARAMS 32

typedef struct Value Value;
typedef struct Environment Environment;
typedef struct Statement Statement;
typedef struct Expression Expression;
typedef struct Module Module;
typedef struct Interpreter Interpreter;
typedef struct StructDef StructDef;
typedef struct Instance Instance;
typedef struct Token Token;
typedef struct FunctionDef FunctionDef;
typedef struct RuntimeModule RuntimeModule;
typedef struct BoundMethod BoundMethod;

typedef enum {
    VALUE_NULL,
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_BOOL,
    VALUE_STRING,
    VALUE_FUNCTION,
    VALUE_BUILTIN,
    VALUE_MODULE,
    VALUE_STRUCT,
    VALUE_INSTANCE,
    VALUE_BOUND_METHOD,
    VALUE_LIST
} ValueKind;

typedef enum {
    STMT_IMPORT,
    STMT_FROM_IMPORT,
    STMT_VAR,
    STMT_ASSIGN,
    STMT_EXPR,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_FUNC,
    STMT_STRUCT,
    STMT_RETURN
} StatementKind;

typedef enum {
    EXPR_LITERAL,
    EXPR_IDENTIFIER,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_MEMBER,
    EXPR_AWAIT
} ExpressionKind;

typedef struct {
    char *name;
    char *type_name;
} Parameter;

typedef struct {
    Value *items;
    int count;
    int capacity;
} ValueList;

typedef Value (*BuiltinFn)(Interpreter *, Environment *, Value *, int, const char *, int);

typedef struct {
    BuiltinFn fn;
    char *name;
} Builtin;

struct Value {
    ValueKind kind;
    union {
        int int_value;
        double float_value;
        int bool_value;
        char *string_value;
        FunctionDef *function_value;
        Builtin *builtin_value;
        RuntimeModule *module_value;
        StructDef *struct_value;
        Instance *instance_value;
        BoundMethod *bound_method_value;
        ValueList *list_value;
    } as;
};

typedef struct {
    char *name;
    char *type_name;
    Expression *default_value;
} FieldDef;

typedef struct {
    Statement **items;
    int count;
    int capacity;
} StatementList;

typedef struct {
    Expression **items;
    int count;
    int capacity;
} ExpressionList;

struct FunctionDef {
    char *name;
    Parameter params[MAX_PARAMS];
    int param_count;
    char *return_type;
    StatementList body;
    int is_async;
    Environment *closure;
};

struct BoundMethod {
    FunctionDef *def;
    Instance *instance;
};

struct RuntimeModule {
    char *name;
    Environment *env;
};

typedef struct {
    char *module_name;
} ImportStmt;

typedef struct {
    char *module_name;
    char **members;
    int member_count;
} FromImportStmt;

typedef struct {
    char *name;
    char *type_name;
    Expression *expr;
} VarStmt;

typedef struct {
    char *name;
    Expression *expr;
} AssignStmt;

typedef struct {
    Expression *expr;
} ExprStmt;

typedef struct {
    Expression *condition;
    StatementList then_block;
    StatementList else_block;
    int has_else;
} IfStmt;

typedef struct {
    Expression *condition;
    StatementList body;
} WhileStmt;

typedef struct {
    char *var_name;
    Expression *iterable;
    StatementList body;
} ForStmt;

typedef struct {
    char *name;
    FieldDef *fields;
    int field_count;
    int field_capacity;
    FunctionDef **methods;
    int method_count;
    int method_capacity;
    Environment *closure;
} StructStmt;

typedef struct {
    Expression *expr;
} ReturnStmt;

typedef struct {
    char *name;
} IdentifierExpr;

typedef struct {
    char *op;
    Expression *left;
    Expression *right;
} BinaryExpr;

typedef struct {
    char *op;
    Expression *operand;
} UnaryExpr;

typedef struct {
    Expression *callee;
    ExpressionList args;
} CallExpr;

typedef struct {
    Expression *target;
    char *member_name;
} MemberExpr;

typedef struct {
    Expression *inner;
} AwaitExpr;

struct Expression {
    ExpressionKind kind;
    char *file_path;
    int line;
    Value literal_value;
    union {
        IdentifierExpr identifier;
        BinaryExpr binary;
        UnaryExpr unary;
        CallExpr call;
        MemberExpr member;
        AwaitExpr await_expr;
    } as;
};

typedef struct {
    char *name;
    Value value;
    char *type_name;
} Variable;

struct Environment {
    Variable *vars;
    int count;
    int capacity;
    Environment *parent;
};

struct StructDef {
    char *name;
    FieldDef *fields;
    int field_count;
    FunctionDef **methods;
    int method_count;
    Environment *closure;
};

struct Module {
    char *name;
    char *file_path;
    StatementList statements;
};

struct Instance {
    StructDef *def;
    Variable *fields;
    int field_count;
    int field_capacity;
};

struct Statement {
    StatementKind kind;
    char *file_path;
    int line;
    union {
        ImportStmt import_stmt;
        FromImportStmt from_import_stmt;
        VarStmt var_stmt;
        AssignStmt assign_stmt;
        ExprStmt expr_stmt;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        FunctionDef func_stmt;
        StructStmt struct_stmt;
        ReturnStmt return_stmt;
    } as;
};

typedef struct {
    Token *items;
    int count;
    int capacity;
} TokenList;

struct Token {
    char *type;
    char *lexeme;
};

typedef struct {
    char *file_path;
    char **lines;
    int line_count;
} Parser;

struct Interpreter {
    Module **modules;
    int module_count;
    int module_capacity;
    int returning;
    Value return_value;
    char *exe_dir;
};

static void fatal_error(const char *file_path, int line, const char *message, const char *hint) {
    fprintf(stderr, "%s:%d: error FLX1000: %s\n", file_path, line, message);
    if (hint != NULL && strlen(hint) > 0) {
        fprintf(stderr, "hint: %s\n", hint);
    }
    exit(1);
}

static void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Fluxa internal error: out of memory\n");
        exit(1);
    }
    memset(ptr, 0, size);
    return ptr;
}

static char *clone_str(const char *src) {
    size_t len = strlen(src);
    char *dst = (char *)xmalloc(len + 1);
    memcpy(dst, src, len + 1);
    return dst;
}

static char *str_trim(char *text) {
    while (*text && isspace((unsigned char)*text)) {
        text++;
    }
    if (*text == '\0') {
        return text;
    }
    char *end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return text;
}

static int starts_with(const char *text, const char *prefix) {
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static int ends_with(const char *text, const char *suffix) {
    size_t a = strlen(text);
    size_t b = strlen(suffix);
    if (b > a) {
        return 0;
    }
    return strcmp(text + a - b, suffix) == 0;
}

static void statement_list_push(StatementList *list, Statement *stmt) {
    if (list->count == list->capacity) {
        list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        list->items = (Statement **)realloc(list->items, sizeof(Statement *) * list->capacity);
    }
    list->items[list->count++] = stmt;
}

static void expression_list_push(ExpressionList *list, Expression *expr) {
    if (list->count == list->capacity) {
        list->capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        list->items = (Expression **)realloc(list->items, sizeof(Expression *) * list->capacity);
    }
    list->items[list->count++] = expr;
}

static void value_list_push(ValueList *list, Value value) {
    if (list->count == list->capacity) {
        list->capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        list->items = (Value *)realloc(list->items, sizeof(Value) * list->capacity);
    }
    list->items[list->count++] = value;
}

static Value make_null(void) {
    Value value;
    memset(&value, 0, sizeof(Value));
    value.kind = VALUE_NULL;
    return value;
}

static Value make_int(int v) {
    Value value = make_null();
    value.kind = VALUE_INT;
    value.as.int_value = v;
    return value;
}

static Value make_float(double v) {
    Value value = make_null();
    value.kind = VALUE_FLOAT;
    value.as.float_value = v;
    return value;
}

static Value make_bool(int v) {
    Value value = make_null();
    value.kind = VALUE_BOOL;
    value.as.bool_value = v ? 1 : 0;
    return value;
}

static Value make_string(const char *v) {
    Value value = make_null();
    value.kind = VALUE_STRING;
    value.as.string_value = clone_str(v);
    return value;
}

static Value make_function(FunctionDef *def) {
    Value value = make_null();
    value.kind = VALUE_FUNCTION;
    value.as.function_value = def;
    return value;
}

static Value make_builtin(const char *name, BuiltinFn fn) {
    Value value = make_null();
    value.kind = VALUE_BUILTIN;
    value.as.builtin_value = (Builtin *)xmalloc(sizeof(Builtin));
    value.as.builtin_value->name = clone_str(name);
    value.as.builtin_value->fn = fn;
    return value;
}

static Value make_module_value(RuntimeModule *module) {
    Value value = make_null();
    value.kind = VALUE_MODULE;
    value.as.module_value = module;
    return value;
}

static Value make_struct_value(StructDef *def) {
    Value value = make_null();
    value.kind = VALUE_STRUCT;
    value.as.struct_value = def;
    return value;
}

static Value make_instance_value(Instance *instance) {
    Value value = make_null();
    value.kind = VALUE_INSTANCE;
    value.as.instance_value = instance;
    return value;
}

static Value make_bound_method_value(BoundMethod *method) {
    Value value = make_null();
    value.kind = VALUE_BOUND_METHOD;
    value.as.bound_method_value = method;
    return value;
}

static Value make_list_value(ValueList *list) {
    Value value = make_null();
    value.kind = VALUE_LIST;
    value.as.list_value = list;
    return value;
}

static Environment *env_new(Environment *parent) {
    Environment *env = (Environment *)xmalloc(sizeof(Environment));
    env->parent = parent;
    return env;
}

static const char *value_type_name(Value value) {
    switch (value.kind) {
        case VALUE_NULL: return "Void";
        case VALUE_INT: return "Int";
        case VALUE_FLOAT: return "Float";
        case VALUE_BOOL: return "Bool";
        case VALUE_STRING: return "String";
        case VALUE_LIST: return "List";
        case VALUE_FUNCTION:
        case VALUE_BUILTIN:
        case VALUE_BOUND_METHOD: return "Function";
        case VALUE_MODULE: return "Module";
        case VALUE_STRUCT: return "Struct";
        case VALUE_INSTANCE: return "Instance";
        default: return "Any";
    }
}

static int type_matches(const char *declared, Value value) {
    if (declared == NULL || strlen(declared) == 0 || strcmp(declared, "Any") == 0) {
        return 1;
    }
    if (strcmp(declared, "Float") == 0 && value.kind == VALUE_INT) {
        return 1;
    }
    return strcmp(declared, value_type_name(value)) == 0;
}

static void env_define(Environment *env, const char *name, Value value, const char *type_name) {
    if (env->count == env->capacity) {
        env->capacity = env->capacity == 0 ? 8 : env->capacity * 2;
        env->vars = (Variable *)realloc(env->vars, sizeof(Variable) * env->capacity);
    }
    env->vars[env->count].name = clone_str(name);
    env->vars[env->count].value = value;
    env->vars[env->count].type_name = type_name ? clone_str(type_name) : NULL;
    env->count++;
}

static Variable *env_find_local(Environment *env, const char *name) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->vars[i].name, name) == 0) {
            return &env->vars[i];
        }
    }
    return NULL;
}

static Variable *env_find(Environment *env, const char *name) {
    for (Environment *cur = env; cur != NULL; cur = cur->parent) {
        Variable *found = env_find_local(cur, name);
        if (found) {
            return found;
        }
    }
    return NULL;
}

static Value env_get(Environment *env, const char *name, const char *file_path, int line) {
    Variable *var = env_find(env, name);
    if (!var) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Unknown name `%s`.", name);
        fatal_error(file_path, line, msg, "Declare it before using it or check for a typo.");
    }
    return var->value;
}

static void env_assign(Environment *env, const char *name, Value value, const char *file_path, int line) {
    Variable *var = env_find(env, name);
    if (!var) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Unknown variable `%s`.", name);
        fatal_error(file_path, line, msg, "Declare it with `let` before assigning to it.");
    }
    if (!type_matches(var->type_name, value)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Type mismatch for `%s`: expected %s, got %s.", name, var->type_name, value_type_name(value));
        fatal_error(file_path, line, msg, "Change the value or adjust the declared type.");
    }
    var->value = value;
}

static int value_is_truthy(Value value) {
    switch (value.kind) {
        case VALUE_NULL: return 0;
        case VALUE_BOOL: return value.as.bool_value;
        case VALUE_INT: return value.as.int_value != 0;
        case VALUE_FLOAT: return fabs(value.as.float_value) > 0.0000001;
        case VALUE_STRING: return value.as.string_value && strlen(value.as.string_value) > 0;
        default: return 1;
    }
}

static double value_to_number(Value value, const char *file_path, int line) {
    if (value.kind == VALUE_INT) return (double)value.as.int_value;
    if (value.kind == VALUE_FLOAT) return value.as.float_value;
    fatal_error(file_path, line, "Expected a numeric value.", "Use Int or Float in this expression.");
    return 0;
}

static char *value_to_string_alloc(Value value) {
    char buffer[256];
    switch (value.kind) {
        case VALUE_NULL: return clone_str("null");
        case VALUE_INT:
            snprintf(buffer, sizeof(buffer), "%d", value.as.int_value);
            return clone_str(buffer);
        case VALUE_FLOAT:
            if (fabs(value.as.float_value - floor(value.as.float_value)) < 0.0000001) {
                snprintf(buffer, sizeof(buffer), "%.1f", value.as.float_value);
            } else {
                snprintf(buffer, sizeof(buffer), "%.15g", value.as.float_value);
            }
            return clone_str(buffer);
        case VALUE_BOOL:
            return clone_str(value.as.bool_value ? "true" : "false");
        case VALUE_STRING:
            return clone_str(value.as.string_value);
        case VALUE_MODULE:
            return clone_str("<module>");
        case VALUE_STRUCT:
            return clone_str("<struct>");
        case VALUE_INSTANCE:
            return clone_str("<instance>");
        case VALUE_FUNCTION:
        case VALUE_BUILTIN:
        case VALUE_BOUND_METHOD:
            return clone_str("<function>");
        case VALUE_LIST: {
            return clone_str("<list>");
        }
        default:
            return clone_str("<value>");
    }
}

static int values_equal(Value a, Value b) {
    if ((a.kind == VALUE_INT || a.kind == VALUE_FLOAT) && (b.kind == VALUE_INT || b.kind == VALUE_FLOAT)) {
        return fabs(value_to_number(a, "", 1) - value_to_number(b, "", 1)) < 0.0000001;
    }
    if (a.kind != b.kind) return 0;
    switch (a.kind) {
        case VALUE_NULL: return 1;
        case VALUE_INT: return a.as.int_value == b.as.int_value;
        case VALUE_FLOAT: return fabs(a.as.float_value - b.as.float_value) < 0.0000001;
        case VALUE_BOOL: return a.as.bool_value == b.as.bool_value;
        case VALUE_STRING: return strcmp(a.as.string_value, b.as.string_value) == 0;
        default: return a.as.string_value == b.as.string_value;
    }
}

static int count_indent(const char *line) {
    int count = 0;
    while (line[count] == ' ') count++;
    return count;
}

static int is_blank_or_comment(const char *line) {
    const char *trimmed = line;
    while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
    return *trimmed == '\0' || *trimmed == '#';
}

static char *substring(const char *start, int len) {
    char *out = (char *)xmalloc((size_t)len + 1);
    memcpy(out, start, (size_t)len);
    out[len] = '\0';
    return out;
}

static char **split_lines(const char *source, int *count) {
    int cap = 16;
    int n = 0;
    char **lines = (char **)xmalloc(sizeof(char *) * cap);
    const char *start = source;
    for (const char *p = source; ; p++) {
        if (*p == '\n' || *p == '\0') {
            if (n == cap) {
                cap *= 2;
                lines = (char **)realloc(lines, sizeof(char *) * cap);
            }
            int len = (int)(p - start);
            if (len > 0 && start[len - 1] == '\r') len--;
            lines[n++] = substring(start, len);
            if (*p == '\0') break;
            start = p + 1;
        }
    }
    *count = n;
    return lines;
}

static Token make_token(const char *type, const char *lexeme) {
    Token token;
    token.type = clone_str(type);
    token.lexeme = clone_str(lexeme);
    return token;
}

static void token_list_push(TokenList *list, Token token) {
    if (list->count == list->capacity) {
        list->capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        list->items = (Token *)realloc(list->items, sizeof(Token) * list->capacity);
    }
    list->items[list->count++] = token;
}

static TokenList lex_expression(const char *file_path, int line, const char *text) {
    TokenList list = {0};
    for (int i = 0; text[i] != '\0'; ) {
        char c = text[i];
        if (isspace((unsigned char)c)) {
            i++;
            continue;
        }
        if (isalpha((unsigned char)c) || c == '_') {
            int start = i++;
            while (isalnum((unsigned char)text[i]) || text[i] == '_') i++;
            token_list_push(&list, make_token("id", substring(text + start, i - start)));
            continue;
        }
        if (isdigit((unsigned char)c)) {
            int start = i++;
            while (isdigit((unsigned char)text[i]) || text[i] == '.') i++;
            token_list_push(&list, make_token("num", substring(text + start, i - start)));
            continue;
        }
        if (c == '"' || c == '\'') {
            char quote = c;
            i++;
            int start = i;
            while (text[i] && text[i] != quote) i++;
            if (!text[i]) {
                fatal_error(file_path, line, "Unterminated string literal.", "Close the string with the same quote character.");
            }
            token_list_push(&list, make_token("str", substring(text + start, i - start)));
            i++;
            continue;
        }
        if ((c == '=' || c == '!' || c == '>' || c == '<') && text[i + 1] == '=') {
            char pair[3] = {c, '=', '\0'};
            token_list_push(&list, make_token(pair, pair));
            i += 2;
            continue;
        }
        if (strchr("()+-*/.,><", c)) {
            char one[2] = {c, '\0'};
            token_list_push(&list, make_token(one, one));
            i++;
            continue;
        }
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unexpected character `%c`.", c);
            fatal_error(file_path, line, msg, "Remove it or replace it with valid Fluxa syntax.");
        }
    }
    token_list_push(&list, make_token("eof", ""));
    return list;
}

typedef struct {
    const char *file_path;
    int line;
    TokenList tokens;
    int pos;
} ExprParser;

static Expression *parse_expression(ExprParser *parser);

static Token *peek_token(ExprParser *parser) {
    return &parser->tokens.items[parser->pos];
}

static int token_is(ExprParser *parser, const char *type) {
    return strcmp(peek_token(parser)->type, type) == 0;
}

static int token_match(ExprParser *parser, const char *type) {
    if (token_is(parser, type)) {
        parser->pos++;
        return 1;
    }
    return 0;
}

static int token_match_id(ExprParser *parser, const char *id) {
    if (token_is(parser, "id") && strcmp(peek_token(parser)->lexeme, id) == 0) {
        parser->pos++;
        return 1;
    }
    return 0;
}

static Token *token_consume(ExprParser *parser, const char *type, const char *message) {
    if (token_is(parser, type)) {
        return &parser->tokens.items[parser->pos++];
    }
    fatal_error(parser->file_path, parser->line, message, "Check the punctuation and missing identifiers.");
    return NULL;
}

static Expression *expr_new(ExpressionKind kind, const char *file_path, int line) {
    Expression *expr = (Expression *)xmalloc(sizeof(Expression));
    expr->kind = kind;
    expr->file_path = clone_str(file_path);
    expr->line = line;
    return expr;
}

static Expression *parse_primary(ExprParser *parser) {
    if (token_match(parser, "num")) {
        Token *prev = &parser->tokens.items[parser->pos - 1];
        Expression *expr = expr_new(EXPR_LITERAL, parser->file_path, parser->line);
        if (strchr(prev->lexeme, '.') != NULL) expr->literal_value = make_float(strtod(prev->lexeme, NULL));
        else expr->literal_value = make_int(atoi(prev->lexeme));
        return expr;
    }
    if (token_match(parser, "str")) {
        Token *prev = &parser->tokens.items[parser->pos - 1];
        Expression *expr = expr_new(EXPR_LITERAL, parser->file_path, parser->line);
        expr->literal_value = make_string(prev->lexeme);
        return expr;
    }
    if (token_match_id(parser, "true")) {
        Expression *expr = expr_new(EXPR_LITERAL, parser->file_path, parser->line);
        expr->literal_value = make_bool(1);
        return expr;
    }
    if (token_match_id(parser, "false")) {
        Expression *expr = expr_new(EXPR_LITERAL, parser->file_path, parser->line);
        expr->literal_value = make_bool(0);
        return expr;
    }
    if (token_match_id(parser, "null")) {
        Expression *expr = expr_new(EXPR_LITERAL, parser->file_path, parser->line);
        expr->literal_value = make_null();
        return expr;
    }
    if (token_match(parser, "id")) {
        Token *prev = &parser->tokens.items[parser->pos - 1];
        Expression *expr = expr_new(EXPR_IDENTIFIER, parser->file_path, parser->line);
        expr->as.identifier.name = clone_str(prev->lexeme);
        return expr;
    }
    if (token_match(parser, "(")) {
        Expression *expr = parse_expression(parser);
        token_consume(parser, ")", "Expected `)` after expression.");
        return expr;
    }
    fatal_error(parser->file_path, parser->line, "Could not parse expression.", "Check the syntax near this expression.");
    return NULL;
}

static Expression *parse_call(ExprParser *parser) {
    Expression *expr = parse_primary(parser);
    while (1) {
        if (token_match(parser, "(")) {
            Expression *call = expr_new(EXPR_CALL, parser->file_path, parser->line);
            call->as.call.callee = expr;
            while (!token_is(parser, ")")) {
                expression_list_push(&call->as.call.args, parse_expression(parser));
                if (!token_match(parser, ",")) break;
            }
            token_consume(parser, ")", "Expected `)` after arguments.");
            expr = call;
            continue;
        }
        if (token_match(parser, ".")) {
            Token *name = token_consume(parser, "id", "Expected member name after `.`.");
            Expression *member = expr_new(EXPR_MEMBER, parser->file_path, parser->line);
            member->as.member.target = expr;
            member->as.member.member_name = clone_str(name->lexeme);
            expr = member;
            continue;
        }
        break;
    }
    return expr;
}

static Expression *parse_unary(ExprParser *parser) {
    if (token_match(parser, "-")) {
        Expression *expr = expr_new(EXPR_UNARY, parser->file_path, parser->line);
        expr->as.unary.op = clone_str("-");
        expr->as.unary.operand = parse_unary(parser);
        return expr;
    }
    if (token_match_id(parser, "await")) {
        Expression *expr = expr_new(EXPR_AWAIT, parser->file_path, parser->line);
        expr->as.await_expr.inner = parse_unary(parser);
        return expr;
    }
    return parse_call(parser);
}

static Expression *parse_factor(ExprParser *parser) {
    Expression *expr = parse_unary(parser);
    while (token_match(parser, "*") || token_match(parser, "/")) {
        Token *prev = &parser->tokens.items[parser->pos - 1];
        Expression *binary = expr_new(EXPR_BINARY, parser->file_path, parser->line);
        binary->as.binary.left = expr;
        binary->as.binary.op = clone_str(prev->lexeme);
        binary->as.binary.right = parse_unary(parser);
        expr = binary;
    }
    return expr;
}

static Expression *parse_term(ExprParser *parser) {
    Expression *expr = parse_factor(parser);
    while (token_match(parser, "+") || token_match(parser, "-")) {
        Token *prev = &parser->tokens.items[parser->pos - 1];
        Expression *binary = expr_new(EXPR_BINARY, parser->file_path, parser->line);
        binary->as.binary.left = expr;
        binary->as.binary.op = clone_str(prev->lexeme);
        binary->as.binary.right = parse_factor(parser);
        expr = binary;
    }
    return expr;
}

static Expression *parse_comparison(ExprParser *parser) {
    Expression *expr = parse_term(parser);
    while (token_match(parser, "==") || token_match(parser, "!=") || token_match(parser, ">") ||
           token_match(parser, ">=") || token_match(parser, "<") || token_match(parser, "<=")) {
        Token *prev = &parser->tokens.items[parser->pos - 1];
        Expression *binary = expr_new(EXPR_BINARY, parser->file_path, parser->line);
        binary->as.binary.left = expr;
        binary->as.binary.op = clone_str(prev->lexeme);
        binary->as.binary.right = parse_term(parser);
        expr = binary;
    }
    return expr;
}

static Expression *parse_expression(ExprParser *parser) {
    return parse_comparison(parser);
}

static Expression *parse_expression_text(const char *file_path, int line, const char *text) {
    ExprParser parser = {0};
    parser.file_path = file_path;
    parser.line = line;
    parser.tokens = lex_expression(file_path, line, text);
    return parse_expression(&parser);
}

static Statement *stmt_new(StatementKind kind, const char *file_path, int line) {
    Statement *stmt = (Statement *)xmalloc(sizeof(Statement));
    stmt->kind = kind;
    stmt->file_path = clone_str(file_path);
    stmt->line = line;
    return stmt;
}

static int is_identifier_name(const char *text) {
    if (!text || !*text) return 0;
    if (!(isalpha((unsigned char)text[0]) || text[0] == '_')) return 0;
    for (int i = 1; text[i]; i++) {
        if (!(isalnum((unsigned char)text[i]) || text[i] == '_')) return 0;
    }
    return 1;
}

static StatementList parse_block(Parser *parser, int *index, int indent, int strict);

static Statement *parse_statement(Parser *parser, int *index, int indent) {
    char *line = parser->lines[*index];
    int line_no = *index + 1;
    char *trimmed = str_trim(line);

    if (starts_with(trimmed, "import ")) {
        Statement *stmt = stmt_new(STMT_IMPORT, parser->file_path, line_no);
        stmt->as.import_stmt.module_name = clone_str(str_trim(trimmed + 7));
        (*index)++;
        return stmt;
    }

    if (starts_with(trimmed, "from ")) {
        char *rest = clone_str(trimmed + 5);
        char *marker = strstr(rest, " import ");
        if (!marker) fatal_error(parser->file_path, line_no, "Invalid import syntax.", "Use: from module import name");
        *marker = '\0';
        char *module_name = str_trim(rest);
        char *members_text = marker + 8;
        Statement *stmt = stmt_new(STMT_FROM_IMPORT, parser->file_path, line_no);
        stmt->as.from_import_stmt.module_name = clone_str(module_name);
        char *copy = clone_str(members_text);
        char *tok = strtok(copy, ",");
        while (tok) {
            stmt->as.from_import_stmt.members = (char **)realloc(
                stmt->as.from_import_stmt.members,
                sizeof(char *) * (stmt->as.from_import_stmt.member_count + 1)
            );
            stmt->as.from_import_stmt.members[stmt->as.from_import_stmt.member_count++] = clone_str(str_trim(tok));
            tok = strtok(NULL, ",");
        }
        (*index)++;
        return stmt;
    }

    if (starts_with(trimmed, "let ")) {
        char *body = clone_str(trimmed + 4);
        char *eq = strchr(body, '=');
        if (!eq) fatal_error(parser->file_path, line_no, "Invalid variable declaration.", "Use `let total = 10` or `let total: Int = 10`.");
        *eq = '\0';
        char *left = str_trim(body);
        char *right = str_trim(eq + 1);
        Statement *stmt = stmt_new(STMT_VAR, parser->file_path, line_no);
        char *colon = strchr(left, ':');
        if (colon) {
            *colon = '\0';
            stmt->as.var_stmt.name = clone_str(str_trim(left));
            stmt->as.var_stmt.type_name = clone_str(str_trim(colon + 1));
        } else {
            stmt->as.var_stmt.name = clone_str(left);
        }
        stmt->as.var_stmt.expr = parse_expression_text(parser->file_path, line_no, right);
        (*index)++;
        return stmt;
    }

    if (starts_with(trimmed, "async func ") || starts_with(trimmed, "func ")) {
        int is_async = starts_with(trimmed, "async func ");
        char *sig = clone_str(trimmed + (is_async ? 11 : 5));
        if (!ends_with(sig, ":")) {
            fatal_error(parser->file_path, line_no, "Invalid function declaration.", "Use: func add(a: Int, b: Int) -> Int:");
        }
        sig[strlen(sig) - 1] = '\0';
        char *arrow = strstr(sig, "->");
        char *return_type = clone_str("Void");
        if (arrow) {
            *arrow = '\0';
            return_type = clone_str(str_trim(arrow + 2));
        }
        char *open = strchr(sig, '(');
        char *close = strrchr(sig, ')');
        if (!open || !close || close < open) {
            fatal_error(parser->file_path, line_no, "Invalid function declaration.", "Use: func add(a: Int, b: Int) -> Int:");
        }
        *open = '\0';
        *close = '\0';
        Statement *stmt = stmt_new(STMT_FUNC, parser->file_path, line_no);
        stmt->as.func_stmt.name = clone_str(str_trim(sig));
        stmt->as.func_stmt.return_type = return_type;
        stmt->as.func_stmt.is_async = is_async;
        char *params_copy = clone_str(open + 1);
        char *tok = strtok(params_copy, ",");
        while (tok) {
            char *part = str_trim(tok);
            if (*part) {
                char *colon = strchr(part, ':');
                Parameter *param = &stmt->as.func_stmt.params[stmt->as.func_stmt.param_count++];
                if (colon) {
                    *colon = '\0';
                    param->name = clone_str(str_trim(part));
                    param->type_name = clone_str(str_trim(colon + 1));
                } else {
                    param->name = clone_str(part);
                    param->type_name = clone_str("Any");
                }
            }
            tok = strtok(NULL, ",");
        }
        (*index)++;
        if (*index >= parser->line_count || count_indent(parser->lines[*index]) <= indent) {
            fatal_error(parser->file_path, line_no, "Expected an indented block.", "Add an indented block after the line ending with `:`.");
        }
        stmt->as.func_stmt.body = parse_block(parser, index, count_indent(parser->lines[*index]), 1);
        return stmt;
    }

    if (starts_with(trimmed, "struct ")) {
        char *body = clone_str(trimmed + 7);
        if (!ends_with(body, ":")) fatal_error(parser->file_path, line_no, "Invalid struct declaration.", "Use: struct User:");
        body[strlen(body) - 1] = '\0';
        Statement *stmt = stmt_new(STMT_STRUCT, parser->file_path, line_no);
        stmt->as.struct_stmt.name = clone_str(str_trim(body));
        (*index)++;
        if (*index >= parser->line_count || count_indent(parser->lines[*index]) <= indent) {
            fatal_error(parser->file_path, line_no, "Expected an indented block.", "Add an indented block after the line ending with `:`.");
        }
        int child_indent = count_indent(parser->lines[*index]);
        while (*index < parser->line_count) {
            char *member_line = parser->lines[*index];
            if (is_blank_or_comment(member_line)) {
                (*index)++;
                continue;
            }
            int member_indent = count_indent(member_line);
            if (member_indent < child_indent) break;
            if (member_indent != child_indent) fatal_error(parser->file_path, *index + 1, "Unexpected indentation.", "Use consistent 4-space indentation.");
            char *member_trim = str_trim(member_line);
            if (starts_with(member_trim, "func ")) {
                Statement *method_stmt = parse_statement(parser, index, child_indent);
                stmt->as.struct_stmt.methods = (FunctionDef **)realloc(
                    stmt->as.struct_stmt.methods,
                    sizeof(FunctionDef *) * (stmt->as.struct_stmt.method_count + 1)
                );
                stmt->as.struct_stmt.methods[stmt->as.struct_stmt.method_count++] = &method_stmt->as.func_stmt;
            } else {
                char *field_copy = clone_str(member_trim);
                char *colon = strchr(field_copy, ':');
                if (!colon) fatal_error(parser->file_path, *index + 1, "Only fields and methods are allowed inside a struct.", "Use `name: Type` or `func`.");
                *colon = '\0';
                char *name = str_trim(field_copy);
                char *rest = str_trim(colon + 1);
                char *eq = strchr(rest, '=');
                char *type_name = NULL;
                Expression *default_expr = NULL;
                if (eq) {
                    *eq = '\0';
                    type_name = clone_str(str_trim(rest));
                    default_expr = parse_expression_text(parser->file_path, *index + 1, str_trim(eq + 1));
                } else {
                    type_name = clone_str(rest);
                    if (strcmp(type_name, "Int") == 0) default_expr = parse_expression_text(parser->file_path, *index + 1, "0");
                    else if (strcmp(type_name, "Float") == 0) default_expr = parse_expression_text(parser->file_path, *index + 1, "0.0");
                    else if (strcmp(type_name, "String") == 0) default_expr = parse_expression_text(parser->file_path, *index + 1, "\"\"");
                    else if (strcmp(type_name, "Bool") == 0) default_expr = parse_expression_text(parser->file_path, *index + 1, "false");
                    else default_expr = parse_expression_text(parser->file_path, *index + 1, "null");
                }
                if (stmt->as.struct_stmt.field_count == stmt->as.struct_stmt.field_capacity) {
                    stmt->as.struct_stmt.field_capacity = stmt->as.struct_stmt.field_capacity == 0 ? 4 : stmt->as.struct_stmt.field_capacity * 2;
                    stmt->as.struct_stmt.fields = (FieldDef *)realloc(stmt->as.struct_stmt.fields, sizeof(FieldDef) * stmt->as.struct_stmt.field_capacity);
                }
                FieldDef *field = &stmt->as.struct_stmt.fields[stmt->as.struct_stmt.field_count++];
                field->name = clone_str(name);
                field->type_name = type_name;
                field->default_value = default_expr;
                (*index)++;
            }
        }
        return stmt;
    }

    if (starts_with(trimmed, "if ")) {
        char *body = clone_str(trimmed + 3);
        if (!ends_with(body, ":")) fatal_error(parser->file_path, line_no, "Invalid if statement.", "Use: if condition:");
        body[strlen(body) - 1] = '\0';
        Statement *stmt = stmt_new(STMT_IF, parser->file_path, line_no);
        stmt->as.if_stmt.condition = parse_expression_text(parser->file_path, line_no, str_trim(body));
        (*index)++;
        if (*index >= parser->line_count || count_indent(parser->lines[*index]) <= indent) {
            fatal_error(parser->file_path, line_no, "Expected an indented block.", "Add an indented block after the line ending with `:`.");
        }
        int child_indent = count_indent(parser->lines[*index]);
        stmt->as.if_stmt.then_block = parse_block(parser, index, child_indent, 1);
        if (*index < parser->line_count) {
            char *next = parser->lines[*index];
            if (!is_blank_or_comment(next) && count_indent(next) == indent && strcmp(str_trim(next), "else:") == 0) {
                (*index)++;
                if (*index >= parser->line_count || count_indent(parser->lines[*index]) <= indent) {
                    fatal_error(parser->file_path, line_no, "Expected an indented block.", "Add an indented block after `else:`.");
                }
                stmt->as.if_stmt.has_else = 1;
                stmt->as.if_stmt.else_block = parse_block(parser, index, count_indent(parser->lines[*index]), 1);
            }
        }
        return stmt;
    }

    if (starts_with(trimmed, "while ")) {
        char *body = clone_str(trimmed + 6);
        if (!ends_with(body, ":")) fatal_error(parser->file_path, line_no, "Invalid while statement.", "Use: while condition:");
        body[strlen(body) - 1] = '\0';
        Statement *stmt = stmt_new(STMT_WHILE, parser->file_path, line_no);
        stmt->as.while_stmt.condition = parse_expression_text(parser->file_path, line_no, str_trim(body));
        (*index)++;
        stmt->as.while_stmt.body = parse_block(parser, index, count_indent(parser->lines[*index]), 1);
        return stmt;
    }

    if (starts_with(trimmed, "for ")) {
        char *body = clone_str(trimmed + 4);
        if (!ends_with(body, ":")) fatal_error(parser->file_path, line_no, "Invalid for statement.", "Use: for item in range(0, 10):");
        body[strlen(body) - 1] = '\0';
        char *in = strstr(body, " in ");
        if (!in) fatal_error(parser->file_path, line_no, "Invalid for loop header.", "Use: for item in iterable:");
        *in = '\0';
        Statement *stmt = stmt_new(STMT_FOR, parser->file_path, line_no);
        stmt->as.for_stmt.var_name = clone_str(str_trim(body));
        stmt->as.for_stmt.iterable = parse_expression_text(parser->file_path, line_no, str_trim(in + 4));
        (*index)++;
        stmt->as.for_stmt.body = parse_block(parser, index, count_indent(parser->lines[*index]), 1);
        return stmt;
    }

    if (starts_with(trimmed, "return")) {
        Statement *stmt = stmt_new(STMT_RETURN, parser->file_path, line_no);
        char *rest = str_trim(trimmed + 6);
        if (*rest) stmt->as.return_stmt.expr = parse_expression_text(parser->file_path, line_no, rest);
        (*index)++;
        return stmt;
    }

    char *eq = strchr(trimmed, '=');
    if (eq && strstr(trimmed, "==") == NULL) {
        int left_len = (int)(eq - trimmed);
        char *left = substring(trimmed, left_len);
        left = str_trim(left);
        if (is_identifier_name(left)) {
            Statement *stmt = stmt_new(STMT_ASSIGN, parser->file_path, line_no);
            stmt->as.assign_stmt.name = clone_str(left);
            stmt->as.assign_stmt.expr = parse_expression_text(parser->file_path, line_no, str_trim(eq + 1));
            (*index)++;
            return stmt;
        }
    }

    Statement *stmt = stmt_new(STMT_EXPR, parser->file_path, line_no);
    stmt->as.expr_stmt.expr = parse_expression_text(parser->file_path, line_no, trimmed);
    (*index)++;
    return stmt;
}

static StatementList parse_block(Parser *parser, int *index, int indent, int strict) {
    StatementList list = {0};
    while (*index < parser->line_count) {
        if (is_blank_or_comment(parser->lines[*index])) {
            (*index)++;
            continue;
        }
        int current_indent = count_indent(parser->lines[*index]);
        if (current_indent < indent) break;
        if (strict && current_indent != indent) {
            fatal_error(parser->file_path, *index + 1, "Unexpected indentation.", "Use consistent 4-space indentation.");
        }
        if (!strict && current_indent > indent) {
            fatal_error(parser->file_path, *index + 1, "Unexpected indentation.", "Blocks should start after a line ending with `:`.");
        }
        statement_list_push(&list, parse_statement(parser, index, current_indent));
    }
    return list;
}

static Module *parse_module(const char *file_path, const char *source) {
    Module *module = (Module *)xmalloc(sizeof(Module));
    module->name = clone_str(file_path);
    module->file_path = clone_str(file_path);
    Parser parser = {0};
    parser.file_path = module->file_path;
    parser.lines = split_lines(source, &parser.line_count);
    int index = 0;
    module->statements = parse_block(&parser, &index, 0, 0);
    return module;
}

static int is_declaration_statement(Statement *stmt) {
    return stmt->kind == STMT_IMPORT ||
           stmt->kind == STMT_FROM_IMPORT ||
           stmt->kind == STMT_FUNC ||
           stmt->kind == STMT_STRUCT;
}

static FunctionDef *find_main_function(Module *module) {
    for (int i = 0; i < module->statements.count; i++) {
        Statement *stmt = module->statements.items[i];
        if (stmt->kind == STMT_FUNC && strcmp(stmt->as.func_stmt.name, "main") == 0) {
            return &stmt->as.func_stmt;
        }
    }
    return NULL;
}

static void validate_module_layout(Module *module) {
    FunctionDef *main_def = find_main_function(module);
    for (int i = 0; i < module->statements.count; i++) {
        Statement *stmt = module->statements.items[i];
        if (!is_declaration_statement(stmt) && main_def == NULL) {
            fatal_error(
                stmt->file_path,
                stmt->line,
                "Top-level executable code requires `main`.",
                "Start the file with `func main() -> Void:` and move executable code inside it."
            );
        }
    }
}

static FunctionDef *validate_entry_module(Module *module) {
    validate_module_layout(module);

    if (module->statements.count == 0) {
        fatal_error(module->file_path, 1, "Entry file is empty.", "Start the file with `func main() -> Void:`.");
    }

    Statement *first = NULL;
    for (int i = 0; i < module->statements.count; i++) {
        Statement *stmt = module->statements.items[i];
        if (stmt->kind == STMT_IMPORT || stmt->kind == STMT_FROM_IMPORT) {
            continue;
        }
        first = stmt;
        break;
    }
    if (!first) {
        fatal_error(module->file_path, 1, "Entry file must contain `main`.", "Add `func main() -> Void:` or `async func main() -> Void:`.");
    }
    if (first->kind != STMT_FUNC || strcmp(first->as.func_stmt.name, "main") != 0) {
        fatal_error(
            first->file_path,
            first->line,
            "The entry file must have `main` as the first non-import statement.",
            "Make the first non-import top-level statement `func main() -> Void:` or `async func main() -> Void:`."
        );
    }

    for (int i = 1; i < module->statements.count; i++) {
        Statement *stmt = module->statements.items[i];
        if (!is_declaration_statement(stmt)) {
            fatal_error(
                stmt->file_path,
                stmt->line,
                "Top-level executable code is not allowed in the entry file.",
                "Move variables, prints, loops and other actions inside `main`."
            );
        }
    }

    return &first->as.func_stmt;
}

static Value eval_expression(Interpreter *interpreter, Environment *env, Expression *expr);
static void execute_statement(Interpreter *interpreter, Environment *env, Statement *stmt);
static void execute_block(Interpreter *interpreter, Environment *env, StatementList *block);
static Value invoke_function(Interpreter *interpreter, FunctionDef *def, Environment *call_parent, Value *args, int argc, const char *file_path, int line, Instance *bound_instance);

static Value builtin_print(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter;
    (void)env;
    for (int i = 0; i < argc; i++) {
        char *text = value_to_string_alloc(args[i]);
        printf("%s", text);
        if (i + 1 < argc) printf(" ");
    }
    printf("\n");
    return make_null();
}

static Value builtin_input(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter;
    (void)env;
    if (argc > 1) {
        fatal_error(file_path, line, "input expects zero or one argument.", "Use input() or input(\"Prompt: \").");
    }
    if (argc == 1) {
        char *prompt = value_to_string_alloc(args[0]);
        printf("%s", prompt);
    }

    char buffer[2048];
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return make_string("");
    }

    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        buffer[len - 1] = '\0';
        len--;
    }
    return make_string(buffer);
}

static Value builtin_range(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter;
    (void)env;
    if (!(argc == 1 || argc == 2)) {
        fatal_error(file_path, line, "range expects 1 or 2 arguments.", "Use range(end) or range(start, end).");
    }
    int start = argc == 1 ? 0 : (int)value_to_number(args[0], file_path, line);
    int end = argc == 1 ? (int)value_to_number(args[0], file_path, line) : (int)value_to_number(args[1], file_path, line);
    ValueList *list = (ValueList *)xmalloc(sizeof(ValueList));
    for (int i = start; i < end; i++) value_list_push(list, make_int(i));
    return make_list_value(list);
}

static Value builtin_run(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter;
    (void)env;
    (void)args;
    (void)argc;
    fatal_error(file_path, line, "`run(...)` is no longer needed.", "Define `main` first in the file. Fluxa calls it automatically.");
    return make_null();
}

static Value builtin_math_sqrt(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter; (void)env; (void)argc;
    return make_float(sqrt(value_to_number(args[0], file_path, line)));
}

static Value builtin_math_pow(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter; (void)env; (void)argc;
    return make_float(pow(value_to_number(args[0], file_path, line), value_to_number(args[1], file_path, line)));
}

static Value builtin_math_abs(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter; (void)env; (void)argc;
    return make_float(fabs(value_to_number(args[0], file_path, line)));
}

static Value builtin_strings_upper(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter; (void)env; (void)argc; (void)file_path; (void)line;
    char *copy = clone_str(args[0].as.string_value);
    for (int i = 0; copy[i]; i++) copy[i] = (char)toupper((unsigned char)copy[i]);
    return make_string(copy);
}

static Value builtin_strings_lower(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter; (void)env; (void)argc; (void)file_path; (void)line;
    char *copy = clone_str(args[0].as.string_value);
    for (int i = 0; copy[i]; i++) copy[i] = (char)tolower((unsigned char)copy[i]);
    return make_string(copy);
}

static Value builtin_strings_trim(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter; (void)env; (void)argc; (void)file_path; (void)line;
    char *copy = clone_str(args[0].as.string_value);
    return make_string(str_trim(copy));
}

static Value builtin_strings_repeat(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter; (void)env; (void)argc;
    const char *text = args[0].as.string_value;
    int times = (int)value_to_number(args[1], file_path, line);
    if (times < 0) {
        fatal_error(file_path, line, "repeat count cannot be negative.", "Pass 0 or a positive number.");
    }
    size_t text_len = strlen(text);
    char *result = (char *)xmalloc(text_len * (size_t)times + 1);
    result[0] = '\0';
    for (int i = 0; i < times; i++) strcat(result, text);
    return make_string(result);
}

static Value builtin_async_sleep(Interpreter *interpreter, Environment *env, Value *args, int argc, const char *file_path, int line) {
    (void)interpreter; (void)env; (void)argc;
    Sleep((DWORD)(value_to_number(args[0], file_path, line) * 1000.0));
    return make_null();
}

static RuntimeModule *runtime_module_new(const char *name) {
    RuntimeModule *module = (RuntimeModule *)xmalloc(sizeof(RuntimeModule));
    module->name = clone_str(name);
    module->env = env_new(NULL);
    return module;
}

static void seed_stdlib(Environment *env) {
    env_define(env, "print", make_builtin("print", builtin_print), "Any");
    env_define(env, "input", make_builtin("input", builtin_input), "Any");
    env_define(env, "range", make_builtin("range", builtin_range), "Any");
    env_define(env, "run", make_builtin("run", builtin_run), "Any");

    RuntimeModule *math_mod = runtime_module_new("math");
    env_define(math_mod->env, "pi", make_float(3.141592653589793), "Float");
    env_define(math_mod->env, "e", make_float(2.718281828459045), "Float");
    env_define(math_mod->env, "sqrt", make_builtin("sqrt", builtin_math_sqrt), "Any");
    env_define(math_mod->env, "pow", make_builtin("pow", builtin_math_pow), "Any");
    env_define(math_mod->env, "abs", make_builtin("abs", builtin_math_abs), "Any");
    env_define(env, "math", make_module_value(math_mod), "Any");

    RuntimeModule *strings_mod = runtime_module_new("strings");
    env_define(strings_mod->env, "upper", make_builtin("upper", builtin_strings_upper), "Any");
    env_define(strings_mod->env, "lower", make_builtin("lower", builtin_strings_lower), "Any");
    env_define(strings_mod->env, "trim", make_builtin("trim", builtin_strings_trim), "Any");
    env_define(strings_mod->env, "repeat", make_builtin("repeat", builtin_strings_repeat), "Any");
    env_define(env, "strings", make_module_value(strings_mod), "Any");

    RuntimeModule *io_mod = runtime_module_new("io");
    env_define(io_mod->env, "print", make_builtin("print", builtin_print), "Any");
    env_define(io_mod->env, "input", make_builtin("input", builtin_input), "Any");
    env_define(env, "io", make_module_value(io_mod), "Any");

    RuntimeModule *async_mod = runtime_module_new("asyncs");
    env_define(async_mod->env, "sleep", make_builtin("sleep", builtin_async_sleep), "Any");
    env_define(env, "asyncs", make_module_value(async_mod), "Any");
}

static char *read_file_text(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = (char *)xmalloc((size_t)size + 1);
    fread(buffer, 1, (size_t)size, f);
    buffer[size] = '\0';
    fclose(f);
    return buffer;
}

static char *dirname_of(const char *path) {
    char *copy = clone_str(path);
    char *slash = strrchr(copy, '\\');
    if (!slash) slash = strrchr(copy, '/');
    if (slash) *slash = '\0';
    return copy;
}

static Module *interpreter_load_module(Interpreter *interpreter, const char *path) {
    for (int i = 0; i < interpreter->module_count; i++) {
        if (_stricmp(interpreter->modules[i]->file_path, path) == 0) {
            return interpreter->modules[i];
        }
    }
    char *source = read_file_text(path);
    if (!source) fatal_error(path, 1, "Source file not found.", "Check the file path and try again.");
    Module *module = parse_module(path, source);
    if (interpreter->module_count == interpreter->module_capacity) {
        interpreter->module_capacity = interpreter->module_capacity == 0 ? 8 : interpreter->module_capacity * 2;
        interpreter->modules = (Module **)realloc(interpreter->modules, sizeof(Module *) * interpreter->module_capacity);
    }
    interpreter->modules[interpreter->module_count++] = module;
    return module;
}

static Value env_to_module_value(const char *name, Environment *env) {
    RuntimeModule *module = runtime_module_new(name);
    module->env = env;
    return make_module_value(module);
}

static Value import_module(Interpreter *interpreter, Environment *env, const char *requester_path, const char *module_name, int line) {
    Variable *stdlib = env_find(env, module_name);
    if (stdlib && stdlib->value.kind == VALUE_MODULE) return stdlib->value;

    char *base = dirname_of(requester_path);
    char full[MAX_PATH];
    snprintf(full, sizeof(full), "%s\\%s.flx", base, module_name);
    Module *module = NULL;
    char *source = read_file_text(full);
    if (source) {
        free(source);
        module = interpreter_load_module(interpreter, full);
    } else {
        snprintf(full, sizeof(full), "%s\\%s.flx", interpreter->exe_dir, module_name);
        module = interpreter_load_module(interpreter, full);
    }
    Environment *module_env = env_new(NULL);
    seed_stdlib(module_env);
    execute_block(interpreter, module_env, &module->statements);
    return env_to_module_value(module_name, module_env);
}

static Value instance_get_field(Instance *instance, const char *name, const char *file_path, int line) {
    for (int i = 0; i < instance->field_count; i++) {
        if (strcmp(instance->fields[i].name, name) == 0) return instance->fields[i].value;
    }
    for (int i = 0; i < instance->def->method_count; i++) {
        if (strcmp(instance->def->methods[i]->name, name) == 0) {
            BoundMethod *bound = (BoundMethod *)xmalloc(sizeof(BoundMethod));
            bound->def = instance->def->methods[i];
            bound->instance = instance;
            return make_bound_method_value(bound);
        }
    }
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "`%s` has no field `%s`.", instance->def->name, name);
        fatal_error(file_path, line, msg, "Check the field name or define it in the struct.");
    }
    return make_null();
}

static Value eval_call(Interpreter *interpreter, Environment *env, Value callee, ExpressionList *args, const char *file_path, int line);

static Value eval_expression(Interpreter *interpreter, Environment *env, Expression *expr) {
    switch (expr->kind) {
        case EXPR_LITERAL:
            return expr->literal_value;
        case EXPR_IDENTIFIER:
            return env_get(env, expr->as.identifier.name, expr->file_path, expr->line);
        case EXPR_AWAIT:
            return eval_expression(interpreter, env, expr->as.await_expr.inner);
        case EXPR_UNARY: {
            Value operand = eval_expression(interpreter, env, expr->as.unary.operand);
            if (strcmp(expr->as.unary.op, "-") == 0) {
                if (operand.kind == VALUE_INT) return make_int(-operand.as.int_value);
                return make_float(-value_to_number(operand, expr->file_path, expr->line));
            }
            fatal_error(expr->file_path, expr->line, "Unsupported unary operator.", "Use a supported operator like `-value`.");
        }
        case EXPR_BINARY: {
            Value left = eval_expression(interpreter, env, expr->as.binary.left);
            Value right = eval_expression(interpreter, env, expr->as.binary.right);
            char *op = expr->as.binary.op;
            if (strcmp(op, "+") == 0) {
                if (left.kind == VALUE_STRING || right.kind == VALUE_STRING) {
                    char *a = value_to_string_alloc(left);
                    char *b = value_to_string_alloc(right);
                    char *joined = (char *)xmalloc(strlen(a) + strlen(b) + 1);
                    strcpy(joined, a);
                    strcat(joined, b);
                    return make_string(joined);
                }
                if (left.kind == VALUE_INT && right.kind == VALUE_INT) return make_int(left.as.int_value + right.as.int_value);
                return make_float(value_to_number(left, expr->file_path, expr->line) + value_to_number(right, expr->file_path, expr->line));
            }
            if (strcmp(op, "-") == 0) {
                if (left.kind == VALUE_INT && right.kind == VALUE_INT) return make_int(left.as.int_value - right.as.int_value);
                return make_float(value_to_number(left, expr->file_path, expr->line) - value_to_number(right, expr->file_path, expr->line));
            }
            if (strcmp(op, "*") == 0) {
                if (left.kind == VALUE_INT && right.kind == VALUE_INT) return make_int(left.as.int_value * right.as.int_value);
                return make_float(value_to_number(left, expr->file_path, expr->line) * value_to_number(right, expr->file_path, expr->line));
            }
            if (strcmp(op, "/") == 0) {
                return make_float(value_to_number(left, expr->file_path, expr->line) / value_to_number(right, expr->file_path, expr->line));
            }
            if (strcmp(op, "==") == 0) return make_bool(values_equal(left, right));
            if (strcmp(op, "!=") == 0) return make_bool(!values_equal(left, right));
            if (strcmp(op, ">") == 0) return make_bool(value_to_number(left, expr->file_path, expr->line) > value_to_number(right, expr->file_path, expr->line));
            if (strcmp(op, ">=") == 0) return make_bool(value_to_number(left, expr->file_path, expr->line) >= value_to_number(right, expr->file_path, expr->line));
            if (strcmp(op, "<") == 0) return make_bool(value_to_number(left, expr->file_path, expr->line) < value_to_number(right, expr->file_path, expr->line));
            if (strcmp(op, "<=") == 0) return make_bool(value_to_number(left, expr->file_path, expr->line) <= value_to_number(right, expr->file_path, expr->line));
            fatal_error(expr->file_path, expr->line, "Unsupported operator.", "Use a basic arithmetic or comparison operator.");
        }
        case EXPR_MEMBER: {
            Value target = eval_expression(interpreter, env, expr->as.member.target);
            if (target.kind == VALUE_MODULE) {
                return env_get(target.as.module_value->env, expr->as.member.member_name, expr->file_path, expr->line);
            }
            if (target.kind == VALUE_INSTANCE) {
                return instance_get_field(target.as.instance_value, expr->as.member.member_name, expr->file_path, expr->line);
            }
            fatal_error(expr->file_path, expr->line, "This value has no member.", "Use `.` only with modules and struct instances.");
        }
        case EXPR_CALL: {
            Value callee = eval_expression(interpreter, env, expr->as.call.callee);
            return eval_call(interpreter, env, callee, &expr->as.call.args, expr->file_path, expr->line);
        }
        default:
            fatal_error(expr->file_path, expr->line, "Internal parser error.", "Unsupported expression kind.");
    }
    return make_null();
}

static Instance *instance_new(StructDef *def) {
    Instance *instance = (Instance *)xmalloc(sizeof(Instance));
    instance->def = def;
    return instance;
}

static void instance_set_field(Instance *instance, const char *name, Value value) {
    if (instance->field_count == instance->field_capacity) {
        instance->field_capacity = instance->field_capacity == 0 ? 4 : instance->field_capacity * 2;
        instance->fields = (Variable *)realloc(instance->fields, sizeof(Variable) * instance->field_capacity);
    }
    instance->fields[instance->field_count].name = clone_str(name);
    instance->fields[instance->field_count].value = value;
    instance->fields[instance->field_count].type_name = NULL;
    instance->field_count++;
}

static Value invoke_function(Interpreter *interpreter, FunctionDef *def, Environment *call_parent, Value *args, int argc, const char *file_path, int line, Instance *bound_instance) {
    Environment *call_env = env_new(def->closure ? def->closure : call_parent);
    int arg_index = 0;
    for (int i = 0; i < def->param_count; i++) {
        Parameter *param = &def->params[i];
        Value value = make_null();
        if (bound_instance && i == 0 && strcmp(param->name, "self") == 0) {
            value = make_instance_value(bound_instance);
        } else if (arg_index < argc) {
            value = args[arg_index++];
        }
        if (!type_matches(param->type_name, value)) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Type mismatch for parameter `%s`: expected %s, got %s.", param->name, param->type_name, value_type_name(value));
            fatal_error(file_path, line, msg, "Pass a value of the right type.");
        }
        env_define(call_env, param->name, value, param->type_name);
    }
    int prev_returning = interpreter->returning;
    Value prev_return_value = interpreter->return_value;
    interpreter->returning = 0;
    interpreter->return_value = make_null();
    execute_block(interpreter, call_env, &def->body);
    Value result = interpreter->returning ? interpreter->return_value : make_null();
    interpreter->returning = prev_returning;
    interpreter->return_value = prev_return_value;
    if (!type_matches(def->return_type, result)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Return type mismatch in `%s`: expected %s, got %s.", def->name, def->return_type, value_type_name(result));
        fatal_error(file_path, line, msg, "Return a value of the declared type.");
    }
    return result;
}

static Value eval_call(Interpreter *interpreter, Environment *env, Value callee, ExpressionList *args, const char *file_path, int line) {
    Value arg_values[64];
    for (int i = 0; i < args->count; i++) {
        arg_values[i] = eval_expression(interpreter, env, args->items[i]);
    }
    if (callee.kind == VALUE_BUILTIN) {
        return callee.as.builtin_value->fn(interpreter, env, arg_values, args->count, file_path, line);
    }
    if (callee.kind == VALUE_FUNCTION) {
        return invoke_function(interpreter, callee.as.function_value, env, arg_values, args->count, file_path, line, NULL);
    }
    if (callee.kind == VALUE_BOUND_METHOD) {
        return invoke_function(interpreter, callee.as.bound_method_value->def, env, arg_values, args->count, file_path, line, callee.as.bound_method_value->instance);
    }
    if (callee.kind == VALUE_STRUCT) {
        StructDef *def = callee.as.struct_value;
        Instance *instance = instance_new(def);
        for (int i = 0; i < def->field_count; i++) {
            Value initial = i < args->count
                ? arg_values[i]
                : eval_expression(interpreter, env, def->fields[i].default_value);
            if (!type_matches(def->fields[i].type_name, initial)) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Type mismatch for field `%s`: expected %s, got %s.", def->fields[i].name, def->fields[i].type_name, value_type_name(initial));
                fatal_error(file_path, line, msg, "Pass constructor arguments of the right type.");
            }
            instance_set_field(instance, def->fields[i].name, initial);
        }
        return make_instance_value(instance);
    }
    fatal_error(file_path, line, "This value is not callable.", "Only functions, methods and structs can be called.");
    return make_null();
}

static void execute_statement(Interpreter *interpreter, Environment *env, Statement *stmt) {
    if (interpreter->returning) return;
    switch (stmt->kind) {
        case STMT_IMPORT: {
            Value module_value = import_module(interpreter, env, stmt->file_path, stmt->as.import_stmt.module_name, stmt->line);
            env_define(env, stmt->as.import_stmt.module_name, module_value, "Any");
            return;
        }
        case STMT_FROM_IMPORT: {
            Value module_value = import_module(interpreter, env, stmt->file_path, stmt->as.from_import_stmt.module_name, stmt->line);
            for (int i = 0; i < stmt->as.from_import_stmt.member_count; i++) {
                char *member = stmt->as.from_import_stmt.members[i];
                Value value = env_get(module_value.as.module_value->env, member, stmt->file_path, stmt->line);
                env_define(env, member, value, "Any");
            }
            return;
        }
        case STMT_VAR: {
            Value value = eval_expression(interpreter, env, stmt->as.var_stmt.expr);
            if (!type_matches(stmt->as.var_stmt.type_name, value)) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Type mismatch for `%s`: expected %s, got %s.", stmt->as.var_stmt.name, stmt->as.var_stmt.type_name, value_type_name(value));
                fatal_error(stmt->file_path, stmt->line, msg, "Change the value or adjust the declared type.");
            }
            env_define(env, stmt->as.var_stmt.name, value, stmt->as.var_stmt.type_name);
            return;
        }
        case STMT_ASSIGN:
            env_assign(env, stmt->as.assign_stmt.name, eval_expression(interpreter, env, stmt->as.assign_stmt.expr), stmt->file_path, stmt->line);
            return;
        case STMT_EXPR:
            (void)eval_expression(interpreter, env, stmt->as.expr_stmt.expr);
            return;
        case STMT_IF:
            if (value_is_truthy(eval_expression(interpreter, env, stmt->as.if_stmt.condition))) {
                execute_block(interpreter, env_new(env), &stmt->as.if_stmt.then_block);
            } else if (stmt->as.if_stmt.has_else) {
                execute_block(interpreter, env_new(env), &stmt->as.if_stmt.else_block);
            }
            return;
        case STMT_WHILE:
            while (value_is_truthy(eval_expression(interpreter, env, stmt->as.while_stmt.condition))) {
                execute_block(interpreter, env_new(env), &stmt->as.while_stmt.body);
                if (interpreter->returning) return;
            }
            return;
        case STMT_FOR: {
            Value iterable = eval_expression(interpreter, env, stmt->as.for_stmt.iterable);
            if (iterable.kind != VALUE_LIST) fatal_error(stmt->file_path, stmt->line, "For loop expects an iterable list.", "Use range(...) or another list-producing value.");
            for (int i = 0; i < iterable.as.list_value->count; i++) {
                Environment *loop_env = env_new(env);
                env_define(loop_env, stmt->as.for_stmt.var_name, iterable.as.list_value->items[i], "Any");
                execute_block(interpreter, loop_env, &stmt->as.for_stmt.body);
                if (interpreter->returning) return;
            }
            return;
        }
        case STMT_FUNC: {
            stmt->as.func_stmt.closure = env;
            env_define(env, stmt->as.func_stmt.name, make_function(&stmt->as.func_stmt), "Any");
            return;
        }
        case STMT_STRUCT: {
            StructDef *def = (StructDef *)xmalloc(sizeof(StructDef));
            def->name = clone_str(stmt->as.struct_stmt.name);
            def->fields = stmt->as.struct_stmt.fields;
            def->field_count = stmt->as.struct_stmt.field_count;
            def->methods = stmt->as.struct_stmt.methods;
            def->method_count = stmt->as.struct_stmt.method_count;
            def->closure = env;
            for (int i = 0; i < def->method_count; i++) def->methods[i]->closure = env;
            env_define(env, def->name, make_struct_value(def), "Any");
            return;
        }
        case STMT_RETURN:
            interpreter->return_value = stmt->as.return_stmt.expr ? eval_expression(interpreter, env, stmt->as.return_stmt.expr) : make_null();
            interpreter->returning = 1;
            return;
    }
}

static void execute_block(Interpreter *interpreter, Environment *env, StatementList *block) {
    for (int i = 0; i < block->count; i++) {
        execute_statement(interpreter, env, block->items[i]);
        if (interpreter->returning) return;
    }
}

static char *json_escape(const char *text) {
    size_t len = strlen(text);
    char *out = (char *)xmalloc(len * 2 + 1);
    int j = 0;
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\\' || text[i] == '"') out[j++] = '\\';
        out[j++] = text[i];
    }
    out[j] = '\0';
    return out;
}

static void build_module_json(Interpreter *interpreter, const char *file_path) {
    Module *module = interpreter_load_module(interpreter, file_path);
    validate_module_layout(module);
    char *dir = dirname_of(file_path);
    char build_dir[MAX_PATH];
    char out[MAX_PATH];
    snprintf(build_dir, sizeof(build_dir), "%s\\build", dir);
    CreateDirectoryA(build_dir, NULL);
    const char *base = strrchr(file_path, '\\') ? strrchr(file_path, '\\') + 1 : file_path;
    char stem[256];
    strncpy(stem, base, sizeof(stem) - 1);
    stem[sizeof(stem) - 1] = '\0';
    char *dot = strrchr(stem, '.');
    if (dot) *dot = '\0';
    snprintf(out, sizeof(out), "%s\\%s.fluxa.json", build_dir, stem);
    char *escaped = json_escape(file_path);
    FILE *f = fopen(out, "wb");
    if (!f) fatal_error(file_path, 1, "Could not write build output.", "Check directory permissions and try again.");
    fprintf(f, "{\n  \"file\": \"%s\",\n  \"statements\": %d,\n  \"version\": \"%s\"\n}\n", escaped, module->statements.count, FLUXA_VERSION);
    fclose(f);
    printf("Built %s -> %s\n", strrchr(file_path, '\\') ? strrchr(file_path, '\\') + 1 : file_path, out);
}

static void check_module(Interpreter *interpreter, const char *file_path) {
    Module *module = interpreter_load_module(interpreter, file_path);
    validate_module_layout(module);
}

static void run_module(Interpreter *interpreter, const char *file_path) {
    Module *module = interpreter_load_module(interpreter, file_path);
    FunctionDef *main_def = validate_entry_module(module);
    Environment *env = env_new(NULL);
    seed_stdlib(env);
    execute_block(interpreter, env, &module->statements);
    Value no_args[1];
    (void)invoke_function(interpreter, main_def, env, no_args, 0, module->file_path, 1, NULL);
}

static char *path_join(const char *a, const char *b) {
    char *out = (char *)xmalloc(strlen(a) + strlen(b) + 2);
    strcpy(out, a);
    strcat(out, "\\");
    strcat(out, b);
    return out;
}

int main(int argc, char **argv) {
    Interpreter interpreter = {0};
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    char *slash = strrchr(exe_path, '\\');
    if (slash) {
        *slash = '\0';
        interpreter.exe_dir = clone_str(exe_path);
    } else {
        interpreter.exe_dir = clone_str(".");
    }

    if (argc <= 1) {
        printf("Fluxa CLI\n");
        printf("Usage:\n");
        printf("  fluxa file.flx\n");
        printf("  fluxa run file.flx\n");
        printf("  fluxa check file.flx\n");
        printf("  fluxa build file.flx\n");
        printf("  fluxa version\n");
        return 0;
    }

    if (strcmp(argv[1], "version") == 0) {
        printf("Fluxa %s\n", FLUXA_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "build") == 0) {
        if (argc < 3) fatal_error("<cli>", 1, "Missing file for build.", "Use: fluxa build file.flx");
        build_module_json(&interpreter, argv[2]);
        return 0;
    }

    if (strcmp(argv[1], "check") == 0) {
        if (argc < 3) fatal_error("<cli>", 1, "Missing file for check.", "Use: fluxa check file.flx");
        check_module(&interpreter, argv[2]);
        return 0;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) fatal_error("<cli>", 1, "Missing file for run.", "Use: fluxa run file.flx");
        run_module(&interpreter, argv[2]);
        return 0;
    }

    if (ends_with(argv[1], ".flx")) {
        run_module(&interpreter, argv[1]);
        return 0;
    }

    fatal_error("<cli>", 1, "Unknown command.", "Use `fluxa run file.flx`, `fluxa check file.flx`, `fluxa build file.flx` or `fluxa version`.");
    return 1;
}
