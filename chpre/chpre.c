#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "chpre.h"

struct def {
    char *define;     // define string
    char *param;      // param #define psh(param)
    char *substring;  // substitute string
    char *actparam;   // actual param, r0 of psh(r0)
};
char num_defines = 0;  // number of #defines
struct def defines[MAX_DEFINES] = { 0 };

enum tok_type { id, l_paren, r_paren, comma, other };
struct tok {
    enum tok_type tt;
    char value[MAX_TOK_LEN]; // for id
};
struct tok toks[MAX_TOKS];
int num_toks = 0, comments = 0;;

void print_tok(struct tok t) {
    printf("tok - type: %d, value: %s\n", t.tt, t.value);
}

// line points to a non-whitespace char
// def is 1 for a #define and 0 for using a define
int tokenize(char *line, int def) {
    //printf("begin tokenize\n");
    char *p = line, *substring = "GGGGG";
    int ti = 0; // counts toks
    int found_r_paren = 0, found_first_r_paren = 1;
    for (ti = 0; *p && ti < MAX_TOKS; ti++) {
        if (*p >= 'a' && *p <= 'z' || *p >= 'A' && *p <= 'Z') {
            toks[ti].tt = id;
            int j = 0;
            for (j = 0; (*p >= 'a' && *p <= 'z' || *p >= 'A' && *p <= 'Z' || *p >= '0' && *p <= '9'); j++)
                toks[ti].value[j] = *p++;
            toks[ti].value[j] = 0; // null terminate id
        }
        else if (*p == '(') {
            toks[ti].tt = l_paren;
            strcpy(toks[ti].value, "(");
            p++;
        }
        else if (*p == ')') {
            toks[ti].tt = r_paren;
            strcpy(toks[ti].value, ")");
            found_r_paren = 1;
            p++;
        }
        else if (*p == ',') {
            toks[ti].tt = comma;
            strcpy(toks[ti].value, ",");
            p++;
        }
        else if (*p != ' ' && *p != '\t') {
            toks[ti].tt = other;
            strcpy(toks[ti].value, " ");
            p++;
        }
        while (*p == ' ' || *p == '\t') p++; // skip spaces/tabs, get to next token
        if (found_r_paren && found_first_r_paren) {
            substring = p;
            found_first_r_paren = 0;
            //printf("FOUND R PAREN: %s\n", substring);
        }
    }
    num_toks = ti;
    char *same = "NOT";
    if (comments) {
        printf("end tokenize\n");
        for (int i = 0; i < ti; i++) {
            printf("toks[%d]: ", i);
            print_tok(toks[i]);
            same = "SAME";
        }
    }
    if (def && toks[0].tt == id && toks[1].tt == l_paren && toks[2].tt == id && toks[3].tt == r_paren) {
        defines[num_defines].define = strdup(toks[0].value);
        defines[num_defines].param = strdup(toks[2].value);
        defines[num_defines].actparam = strdup(toks[2].value);
        defines[num_defines].substring = strdup(substring);
        printf("// %s\n", line);
        if (comments) {
            printf("// %s: %s\n", same, line);
            printf("// #def str: \"%s\"", defines[num_defines].define);
            printf(" param: \"%s\"", defines[num_defines].param);
            printf(" sub str: \"%s\"", defines[num_defines].substring);
            printf("\n");
        }
        num_defines++;
        return 1;
    }
    return 0;
}

// line points to a non-whitespace char
int check_def_func(char *line) {
    char *place;
    if (strstr(line, "(") && strstr(line, ")"))
        return tokenize(line, 1);
    return 0;
}


void do_define(char *line) {
    char *p = line;
    while (*p != ' ')p++; // skip #define, get to space
    while (*p == ' ' || *p == '\t') p++; // skip spaces/tabs, get to define string
    if (check_def_func(p)) // 1 return means #define f(param)
        return;

    char numchars = 0;
    for (char *q = p; *q != ' '; q++, numchars++); // count define str
    defines[num_defines].define = malloc(numchars+1);
    for (int i = 0; i < numchars; i++)
        defines[num_defines].define[i] = *p++;
    defines[num_defines].define[numchars] = 0;
    while (*p == ' ') p++; // skip spaces, get to substitute string
    numchars = 0;
    for (char *q = p; *q != 0; q++, numchars++); // count substitute str
    defines[num_defines].substring = malloc(numchars+1);
    for (int i = 0; i < numchars; i++)
        defines[num_defines].substring[i] = *p++;

    printf("// %s\n", line);
    if (comments) {
        printf("// #def str: \"%s\"", defines[num_defines].define);
        printf(" sub str: \"%s\"", defines[num_defines].substring);
        if (defines[num_defines].param)
            printf(" param: \"%s\"\n", defines[num_defines].param);
        printf("\n");
    }
    num_defines++;
}

int sub_string(char *line, char *outline, char *place, char *oldstr, char *newstr) {
    int outline_i = 0, line_i = 0, line_search = 0; 
    // copy line into outline upto where #def string is
    for (char *p = line; p != place; line_i++, outline_i++, p++)
        outline[outline_i] = line[line_i];
    // copy substitute string into outline
    for (int i = 0; i < strlen(newstr); i++, outline_i++)
        outline[outline_i] = newstr[i];
    line_search = outline_i;        // continue from just past substitution
    // copy rest of line past #def string into outline
    for (line_i = line_i + strlen(oldstr); line_i < strlen(line); line_i++, outline_i++)
        outline[outline_i] = line[line_i];
    outline[outline_i+1] = 0;       // Null terminate line
    strcpy(line, outline); // replace line with the newly substituted line
    return line_search;
}

void process_line(char *line) {
    char orig_line[BUFSZ], outline[BUFSZ];
    strcpy(orig_line, line);
    memset(outline, 0, BUFSZ);              // zero outline
    char plain_line = 1, bad_line = 0;
    for (int defs = 0; defs < num_defines; defs++) {
        int line_search = 0;                // start at pos 0 of line
        char *place;
        // while line has #def strings on it
        while ((place = strstr(line+line_search, defines[defs].define))) {
            if (defines[defs].param) { // extract the string to substitute for param
                //if (!tokenize(line, 0)) {
                //    bad_line = 1;
                //    goto done;
                //}
                // place points to define string, e.g., this(abc)more
                // Extract abc and convert to thismore
                //printf("HERE - tokenize: %d\n", tokenize(line, 0));
                tokenize(line, 0);
                int bad = 1;
                for (int x = 0; x < num_toks; x++) {
                    //printf("line: %s\n", line);
                    //printf("toks: value: %s define: %s\n", toks[x].value, defines[defs].define);
                    if (toks[x].tt == id && strcmp(toks[x].value, defines[defs].define) == 0) {
                        if (toks[x+1].tt == l_paren && toks[x+2].tt == id && toks[x+3].tt == r_paren)
                            bad = 0;
                        //printf("HEREHEREHERE: xx: %d, bad: %d\n", x, bad);
                        break;
                    }
                }
                if (bad) {
                    bad_line = 1;
                    goto done;
                }
                char *p, actparam[20];
                for (p = place; *p != '('; p++);
                char *remove = p;  // points to string where ( is
                //printf("HERE, remove: %s\n", remove);
                int i = 0;
                for (p = p+1; *p != ')'; p++)
                    actparam[i++] = *p;
                actparam[i] = 0;
                //printf("HERE, actparam: %s\n", actparam);
                defines[defs].actparam = strdup(actparam);
                char *keep = p+1; // points to string just beyond )
                //printf("HERE, keep: %s\n", keep);
                for (p = keep; *p; p++)
                    *remove++ = *p;
                //printf("HERE\n");
                *remove = 0;
                //printf("MACRO LINE: %s\n", line);
            }
            line_search = sub_string(line, outline, place, defines[defs].define, defines[defs].substring);
            if (defines[defs].param) {
                place = strstr(line, defines[defs].param);
                //printf("HERE\n");
                sub_string(line, outline, place, defines[defs].param, defines[defs].actparam);
                //printf("HERE\n");
            }
            plain_line = 0;
            /*
            int outline_i = 0; 
            int line_i = 0;
            // copy line into outline upto where #def string is
            for (char *p = line; p != place; line_i++, outline_i++, p++)
                outline[outline_i] = line[line_i];
            // copy substitute string into outline
            for (int i = 0; i < strlen(defines[defs].substring); i++, outline_i++)
                outline[outline_i] = defines[defs].substring[i];
            line_search = outline_i;        // continue from just past substitution
            // copy rest of line past #def string into outline
            for (line_i = line_i + strlen(defines[defs].define); line_i < strlen(line); line_i++, outline_i++)
                outline[outline_i] = line[line_i];
            outline[outline_i+1] = 0;       // Null terminate line
            plain_line = 0;
            strcpy(line, outline); // replace line with the newly substituted line
            */
        }
    }
done:
    if (plain_line && !bad_line)
        printf("%s\n", orig_line);
    else {
        if (bad_line) {
            printf("// bad line: %s\n", orig_line);
        }
        else {
            if (comments)
                printf("// %s\n", orig_line);
            printf("%s\n", outline);
        }
    }
}

int main(int argc, char **argv) {
    if (argc == 2) {
        if (strcmp(argv[1], "-c") == 0)
            comments = 1;
    }
    char line[BUFSZ], orig_line[BUFSZ];
    int line_num = 1;
    while (1) {
        if (fgets(line, BUFSZ, stdin) == 0)
            break;
        line[strcspn(line, "\n")] = '\0';       // trim lf from line
        strcpy(orig_line, line);
        //printf("%s\n", line);
        if (line[0] == '#' && num_defines < MAX_DEFINES) {
            do_define(line);
        }
        else {
            process_line(line);
            line_num++;
        }
    }
}
