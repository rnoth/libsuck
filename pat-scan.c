#include <errno.h>
#include <arr.h>
#include <pat.ih>

#define prec(tok) (tab_prec[(tok)->id])

#define token(...) tok(__VA_ARGS__, 0, 0)
#define tok(i, c, ...) ((struct token[]){{ .id = i, .ch = c }})

enum state {
	st_init,
	st_str,
};

struct scanner {
	uint8_t const *src;
	struct token  *stk;
	enum state     st;
};

static uint8_t oper(uint8_t const *);

static int shunt_step(  struct token *, struct scanner *sc);
static int shunt_next(  struct token *, struct scanner *sc);
static int shunt_alt(   struct token *, struct scanner *sc);
static int shunt_char(  struct token *, struct scanner *sc);
static int shunt_close( struct token *, struct scanner *sc);
static int shunt_dot(   struct token *, struct scanner *sc);
static int shunt_eol(   struct token *, struct scanner *sc);
static int shunt_esc(   struct token *, struct scanner *sc);
static int shunt_open(  struct token *, struct scanner *sc);
static int shunt_mon(   struct token *, struct scanner *sc);

static void tok_pop_greater(struct token *, struct token *, int8_t);

static int8_t const tab_prec[] = {
	[type_sub] = 0,
	[type_nop] = 0,
	[type_alt] = 1,
	[type_cat] = 2,
	[255] = 0,
};

static int (* const tab_shunt[])() = {
	['\0'] = shunt_eol,
	['\\'] = shunt_esc,
	['.']  = shunt_dot,
	['*']  = shunt_mon,
	['?']  = shunt_mon,
	['+']  = shunt_mon,
	['|']  = shunt_alt,
	['(']  = shunt_open,
	[')']  = shunt_close,
	[255]  = 0,
};

uint8_t
oper(uint8_t const *src)
{
	uint8_t const tab[] = {
		['?'] = type_opt,
		['*'] = type_kln,
		['+'] = type_rep,
		['('] = type_sub,
	};
	return tab[*src];
}

int
pat_scan(struct token **res, void const *src)
{
	size_t const len = strlen(src) + 1;
	struct scanner sc[1] = {{ .src = src, }};
	int err;

	*res = arr_alloc(sizeof **res, len * 2);
	if (!*res) goto nomem;

	sc->stk = arr_alloc(sizeof *sc->stk, len);
	if (!sc->stk) goto nomem;

	err = shunt_step(*res, sc);
	if (err) goto finally;

finally:
	if (err) arr_free(*res);
	arr_free(sc->stk);

	return err;

nomem:
	err = ENOMEM;
	goto finally;
}

int
shunt_string(struct token *res, struct scanner *sc)
{
	++sc->src;
	if (sc->st == st_str) arr_put(sc->stk, token(type_cat));
	else sc->st = st_str;

	return shunt_step(res, sc);
}

int
shunt_next(struct token *res, struct scanner *sc)
{
	++sc->src;
	return shunt_step(res, sc);
}

int
shunt_step(struct token *res, struct scanner *sc)
{
	int (*sh)();

	sh = tab_shunt[*sc->src];
	if (sh) return sh(res, sc);
	else return shunt_char(res, sc);
}

int
shunt_alt(struct token *res, struct scanner *sc)
{
	tok_pop_greater(res, sc->stk, tab_prec[type_alt]);
	arr_put(sc->stk, token(type_alt));
	sc->st = st_init;
	return shunt_next(res, sc);
}

int
shunt_char(struct token *res, struct scanner *sc)
{
	arr_put(res, token(type_lit, *sc->src));
	return shunt_string(res, sc);
}

int
shunt_close(struct token *res, struct scanner *sc)
{
	struct token tok[1];

	if (!arr_len(sc->stk)) return PAT_ERR_BADPAREN;

	arr_get(tok, sc->stk);
	if (tok->id == type_nop) {
		arr_put(res, token(type_sub));
		sc->st = tok->ch;
		return shunt_string(res, sc);
	}

	arr_put(res, tok);
	return shunt_close(res, sc);
}

int
shunt_dot(struct token *res, struct scanner *sc)
{
	arr_put(res, token(type_cls, *sc->src));
	return shunt_string(res, sc);
}

int
shunt_eol(struct token *res, struct scanner *sc)
{
	struct token tok[1];

	if (!arr_len(sc->stk)) return 0;

	arr_get(tok, sc->stk);
	arr_put(res, tok);
	return shunt_eol(res, sc);
}

int
shunt_esc(struct token *res, struct scanner *sc)
{
	++sc->src;
	return shunt_char(res, sc);
}

int
shunt_open(struct token *res, struct scanner *sc)
{
	arr_put(sc->stk, token(type_nop, sc->st));
	sc->st = st_init;
	return shunt_next(res, sc);
}

int
shunt_mon(struct token *res, struct scanner *sc)
{
	uint8_t ty = oper(sc->src);
	arr_put(res, token(ty, *sc->src));
	return shunt_next(res, sc);
}

void
tok_pop_greater(struct token *res, struct token *stk, int8_t pr)
{
	struct token *top;

	if (arr_len(stk) == 0) return;

	top = arr_peek(stk);

	if (prec(top) < pr) return;

	arr_put(res, top);
	arr_rm(stk, arr_len(stk) - 1);

	tok_pop_greater(res, stk, pr);
}