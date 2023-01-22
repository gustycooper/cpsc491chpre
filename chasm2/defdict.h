enum defdicttype { DEFNUM, DEFID, DEFPARAMS };
struct defdictval {
    char *key;
    enum defdicttype type;
    char *defvalue;
    int ivalue;
};
int defdictputval(struct defdictval *);
int defdictgetval(const char *, struct defdictval *);
void defdictprint(int);
