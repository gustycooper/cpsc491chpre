enum defdicttype { DEFNUM, DEFID, DEFPARAMS };
struct defdictval {
    char *key;
    enum defdicttype type;
    struct deftok *head;
    char *defvalue;
    int ivalue;
};

struct deftok {
    struct tokv_t tok;
    struct deftok *next;
};


struct tokv_t get(struct deftok *, int);
int defdictputval(struct defdictval *);
int defdictgetval(const char *, struct defdictval *);
void defdictprint(int);
