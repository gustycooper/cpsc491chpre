#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chasm_types.h"
#include "defdict.h"

// Dictionary implementation - simple version
// See https://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)HashTables.html?highlight=%28CategoryAlgorithmNotes%29
// for a more complex version
#define MULTIPLIER (37) // nice little prime number
unsigned long def_hash_function(const char *s) {
    unsigned long h;
    unsigned const char *us = (unsigned const char *)s; // use values >= 0
    h = 0;
    while(*us != '\0') {
        h = h * MULTIPLIER + *us;
        us++;
    } 
    return h;
}

struct elem {
    struct elem *next;
    char *key;
    enum defdicttype type;
    struct deftok *head;
    char *defvalue;
    int ivalue;
};
#define DICT_SIZE 31
struct elem *defdict[DICT_SIZE] = { 0 };
int def_num_elems = 0;



struct tokv_t get(struct deftok *head, int index) {
    struct deftok *current = head;
    for (int i = 0; i < index; i++) {
        current = current->next;
    }
    return current->tok;
}


/*
 defdictput - puts key, value pair in symbol table
 input
  struct defdictval *dv - struc to put, has key and value
 output
  0 - success
  1 - duplicate symbol
 */
int defdictputval(struct defdictval *dv) {
    char *key = dv->key;
    struct elem *e;
    unsigned long h;
    h = def_hash_function(key) % DICT_SIZE;
    e = defdict[h];
    while (e != NULL) {
        if (strcmp(e->key, key) == 0) {
            //printf("duplicate symbol: %s", key);
            return 1;
        }
        e = e->next;
    }
    e = malloc(sizeof(*e));
    e->key = strdup(key);
    e->type = dv->type;
    e->defvalue = dv->defvalue;
    e->head = dv->head;
    e->ivalue = dv->ivalue;
    e->next = defdict[h];
    defdict[h] = e;
    def_num_elems++;
    /* grow table if there is not enough room
    if(def_num_elems >= DICT_SIZE * MAX_LOAD_FACTOR) {
        grow(d);
    }
    */
    return 0;
}

int defdictgetval(const char *key, struct defdictval *dv) {
    for (struct elem *e = defdict[def_hash_function(key) % DICT_SIZE]; e != 0; e = e->next) {
        if (!strcmp(e->key, key)) {
            dv->key = strdup(key);
            dv->type = e->type;
            dv->head = e->head;
            if (e->defvalue)
                dv->defvalue = strdup(e->defvalue);
            else
                dv->defvalue = 0;
            dv->ivalue = e->ivalue;
            return 0;
        }
    }
    return 1;
}

void defdictprint(int verbose) {
    if (verbose)
        printf("**** defdict contents ****\n");
    int totelems = 0, totcols = 0;
    for (int i = 0; i < DICT_SIZE; i++) {
        int col = 0;
        for (struct elem *e = defdict[i]; e != 0; e = e->next) {
            if (e->type == DEFNUM) { // regular #define <id> <num>
                if (verbose)
                    printf("key: %s, value: %s \n", e->key, e->defvalue);
                else
                    printf("%s %s \n", e->key, e->defvalue);
                totelems++;
                col++;
            }
            else if (e->type == DEFPARAMS) {
                printf("\nkey: %s", e->key);
                for (int i = 0; get(e->head, i).toktype != endd; i++) {
                    printf("\nElement %d: %s\n", i, get(e->head, i).tok_str);
                }
            }
        }
        if (col > 1)
            totcols = totcols + (col - 1);
    }
    if (verbose)
        printf("\ntotelems: %d, totcols: %d\n", totelems, totcols);
}

