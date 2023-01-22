#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/stat.h>
#include <libgen.h>
#include <fcntl.h>
#include "fsms.h"
#include "chasm_types.h"
#include "dict.h"
#include "defdict.h"
#include "charmopcodes.h"

/* enums, structs, and array ins[]
 * chasm types are found in the file chasm_types.h. The types are the following.
 * toks_t - defines enumeration values for tokens
 * tokv_t - defines struct to hold token values of a specific token
 * toki_t - defines struct to hold token information for each line 
 *        - has line unique info (e.g. linenum)
 *        - has individual token info
 *    enums toks_t has a corresponding char *toks_t_str[]. Each element of toks_t_str
 *    is the string of the enum value. For example, toks_t enum data is "data"
 *    C allocates integers to enum value beginning at 0. For example,
 *    enum toks_t { data, text, ...}, data is 0, text is 1, and so on
 *    You can use an enum tag as an index into the array to access the string.
 *    For example, printf("%s\n", toks_t_str[label]); prints label
 * inst_c - defines enumeration values for Charm instruction catetories
 *    ldrstr, arilog, movcmp, branch, miscos
 *
 * inst_t - defines enumeration values for Charm instructions
 * inst_info - defines struct to hold instruction infomation
 *    struct inst_info ins[] is an array with information for each instruction.
 *    ins is indexed with an inst_t enum value. 
 *    For example ins[bgt] selects instn info for the bgt instruction
 *
 * charm_tools generates types inst_c, inst_t, inst_info and the array ins[]
 * charm_insts.txt contains the input to charm_tools
 * charm_tools output is to stdout, which can be redirected to file if desired
 * After running charm_tools, you do two things.
 * 1. replace inst_c, inst_t, and inst_info in the file chasm_types.h with those 
 *    generated by charm_tools
 * 2. replace the array ins[] immediately following this comment with the one
 *    generated by charm_tools
 */

struct inst_info ins[] = {
    {0x72646c, "ldr", ldr, ldrstr, 0x00000010}, {0x62646c, "ldb", ldb, ldrstr, 0x00000020}, 
    {0x727473, "str", str, ldrstr, 0x00000030}, {0x627473, "stb", stb, ldrstr, 0x00000040}, 
    {0x646461, "add", add, arilog, 0x00000050}, {0x627573, "sub", sub, arilog, 0x00000051}, 
    {0x6c756d, "mul", mul, arilog, 0x00000052}, {0x766964, "div", dIv, arilog, 0x00000053}, 
    {0x646f6d, "mod", mod, arilog, 0x00000054}, {0x646e61, "and", and, arilog, 0x00000055}, 
    {0x72726f, "orr", orr, arilog, 0x00000056}, {0x726f65, "eor", eor, arilog, 0x00000057}, 
    {0x636461, "adc", adc, arilog, 0x00000058}, {0x636273, "sbc", sbc, arilog, 0x00000059}, 
    {0x666461, "adf", adf, arilog, 0x0000005a}, {0x667573, "suf", suf, arilog, 0x0000005b}, 
    {0x66756d, "muf", muf, arilog, 0x0000005c}, {0x666964, "dif", dif, arilog, 0x0000005d}, 
    {0x766f6d, "mov", mov, movcmp, 0x00000000}, {0x61766d, "mva", mva, movcmp, 0x00000001}, 
    {0x706d63, "cmp", cmp, movcmp, 0x00000002}, {0x747374, "tst", tst, movcmp, 0x00000003}, 
    {0x716574, "teq", teq, movcmp, 0x00000004}, {0x666873, "shf", shf, movcmp, 0x00000005}, 
    {0x616873, "sha", sha, movcmp, 0x00000006}, {0x746f72, "rot", rot, movcmp, 0x00000007}, 
    {0x656e6f, "one", one, movcmp, 0x00000008}, {0x697466, "fti", fti, movcmp, 0x00000009}, 
    {0x667469, "itf", itf, movcmp, 0x0000000a}, {0x666d63, "cmf", cmf, movcmp, 0x0000000b}, 
    {0x6c6162, "bal", bal, branch, 0x00000000}, {0x716562, "beq", beq, branch, 0x00000001}, 
    {0x656e62, "bne", bne, branch, 0x00000002}, {0x746c62, "blt", blt, branch, 0x00000003}, 
    {0x656c62, "ble", ble, branch, 0x00000004}, {0x746762, "bgt", bgt, branch, 0x00000005}, 
    {0x656762, "bge", bge, branch, 0x00000006}, {0x726c62, "blr", blr, branch, 0x00000007}, 
    {0x72656b, "ker", ker, miscos, 0x000000c0}, {0x677273, "srg", srg, miscos, 0x000000c1}, 
    {0x696f69, "ioi", ioi, miscos, 0x000000c2}, {0x696672, "rfi", rfi, miscos, 0x000000c3}, 
    {0x646b6d, "mkd", mkd, miscos, 0x000000c4}, {0x736b6d, "mks", mks, miscos, 0x000000c5}, 
    {0x696461, "adi", adi, arilog, 0x00000050}, {0x696273, "sbi", sbi, arilog, 0x00000051}, 
};

char *toks_t_str[] = { "data", "text", "label", "string", "inst", "comment",
                        "reg", "comma", "number", "leftbrack", "rightbrack", "exclam", "ident", "err", "endd", "define" };

#define MAX_LINE 128        // maximum length of asm prog line
#define MAX_TOKENS 15       // maximum number of tokens on asm program line
#define MAX_PROG_LINES 2000 // maximum number of lines in an asm program

static char buf[MAX_LINE];              // lines from .chasm file read into buf
static char tokbuf[MAX_LINE*2];         // buf for line with whitespace around , [ and ]
static int  linenum = 1;                // counts lines in file

struct toki_t tok;                      // used to tokenize a line
struct toki_t toks[MAX_PROG_LINES];     // all tokenized lines are stored in toks

/*
 toksvalue - determine token type and value of the string t and put value in tok.toks[n].toktype, .tokv
 struct toki_t tok is built for each line in assembly file
 tok.toks[] is array of tokens on the current line
 The function tokenize() is filling in tok.toks[], which calls toksvalue for each string on line
 inputs
  char *t - address of string on line that is a token
   *t can point to any string on a line.
   Sample ldr r0, [r13, #5] - *t can point to "r0", ",", "[", "r13", ",", "#5", and "]"
  n - the token number in toki_t tok to compute value. for ldr r0, [r13, #5], r0 has an n value of 1,
      [ has an n value of 2, r13 has an n value of 3, and so on.
   n is used as an index into tok.toks[]
  struct inst_info ins[] - array of charm instructions - used to see if *t is type inst
 output
  struct toki_t tok.toks[n].toktype - the string in *t can be and of the enum toks_t values
   examples: ldr is inst, .data is data, r0 is reg, 0x5 is number, gusty is ident, [ is leftbrack
  struct toki_t tok.toks[n].tokv - the value of the token for reg and number tokens
   example: r8 has a value of 8 and 0xf has a value of 15
  struct toki)t tok.insttype, .instcate, and .instopcd - values when the token is inst
 */
void toksvalue(char *t, int n) {
    tok.toks[n].tokv = 0;
    for (int i = 0; i < sizeof(ins) / sizeof(struct inst_info); i++) // check for insts
        //if (strcmp(t, ins[i].inst_str) == 0) {
        if (*(int*)t == ins[i].inst_int) { // inst_int is integer of 3 char instruction
            tok.toks[n].toktype = inst;
            tok.insttype = ins[i].inst_t;
            tok.instcate = ins[i].inst_c;
            tok.instopcd = ins[i].opcode;
            return;
        }
    int base = 0;
    if (t[0] == '/' && t[1] == '/')
        tok.toks[n].toktype = comment;
    else if (strcmp(t, ".data") == 0)
        tok.toks[n].toktype = data;
    else if (strcmp(t, ".stack") == 0)
        tok.toks[n].toktype = data;
    else if (strcmp(t, ".text") == 0)
        tok.toks[n].toktype = text;
    else if (strcmp(t, ".label") == 0 || strcmp(t, ".local") == 0 || strcmp(t, ".extern") == 0)
        tok.toks[n].toktype = label;
    else if (strcmp(t, ".string") == 0)
        tok.toks[n].toktype = string;
    else if (strcmp(t, "#define") == 0)
        tok.toks[n].toktype = def;
    else if (t[0] == ',')
        tok.toks[n].toktype = comma;
    else if (t[0] == '[')
        tok.toks[n].toktype = leftbrack;
    else if (t[0] == ']')
        tok.toks[n].toktype = rightbrack;
    else if (t[0] == '!')
        tok.toks[n].toktype = exclam;
    else if (isreg(t)) {  // could update isreg() to look for pc, lr, and sp
        tok.toks[n].toktype = reg;
        tok.toks[n].tokv = atoi(t+1);
    }
    else if (t[0] == 'p' && t[1] == 'c') { // && t[2] == '\0' I think this is needed
        tok.toks[n].toktype = reg;
        tok.toks[n].tokv = 15;
    }
    else if (t[0] == 'l' && t[1] == 'r') {
        tok.toks[n].toktype = reg;
        tok.toks[n].tokv = 14;
    }
    else if (t[0] == 's' && t[1] == 'p') {
        tok.toks[n].toktype = reg;
        tok.toks[n].tokv = 13;
    }
    else if ((base = isnumber(t))) {
        tok.toks[n].toktype = number;
        int digitpos = 0;
        if (t[0] == '#')
            digitpos = 1;
        if (base == 16)
            digitpos += 2;
        tok.toks[n].tokv = (int)strtol(t+digitpos, NULL, base);
    }
    else if (isid(t))
        tok.toks[n].toktype = ident;
    else  {
        tok.toks[n].toktype = err;
        tok.linetype = err; // any err token causes line to be err
    }
}

/*
 insertwhitespace - insert whitespace around chasm separators
 input
  static char buf[] - has a line read from an assembly file, read by main
  static char separators[] - static array with chasm separators
 output
  static char tokbuf[] - buf with whitespace inserted around separators
 sample
   ldr r0,[r13,#5] becomes ldr r0 , [ r13 , #5 ] - there is a trailing space
 returns
  length of tokbuf
 */
static char separators[] = ",[]!";      // chasm separators ldr r0, [r5, #3]!
int insertwhitespace() { // build tokbuf[]. insert whitespace around separators
    int j = 0, comment = 0;
    for (int i = 0; i < strlen(buf); i++) {
        if (strchr(separators, buf[i])) {
            tokbuf[j++] = ' ';
            tokbuf[j++] = buf[i];
            tokbuf[j++] = ' ';
        }
        else {
            if (buf[i] == '/' && buf[i+1] == '/')
                comment = 1;
            if (!comment && buf[i] >= 'A' && buf[i] <= 'Z')
                buf[i] |= 0x20; // convert upper case to lower
            tokbuf[j++] = buf[i];
        }
    }
    tokbuf[j++] = ' '; // put space at end in front of \0
    tokbuf[j++] = 0;
    return j;
}

/*
 tokenize - tokenize a line into a struct toki_t tok
 Each line of the assembly file is tokenized into tok
 inputs
  char buf[] - has a line read from an assembly file
  char tokbuf[] - buf with whitespace around separators
  char whitespace[] - array with whitespace chars
 output
  struct toki_t tok - a tokenized line
 */
static char whitespace[] = " \t\r\n\v"; // definition of whitespace - tokenize skips whitespace
void tokenize() {
    insertwhitespace();
    char *s = tokbuf;
    char *end_buf = tokbuf + strlen(tokbuf);
    int numtoks = 0;
    int toknum = 0;
    tok.linetype = comment;                            // assume a blank line
    while (1) {
        toknum = numtoks;
        while (s < end_buf && strchr(whitespace, *s))  // skip whitespace
            s++;
        if (*s == 0)                                   // eol - done
            break;
        char *t = s;                                   // t is address of start of token string
        while (s < end_buf && !strchr(whitespace, *s)) // consume non-whitespace, adv to end of token string
            s++;
        numtoks++;                                     // count tokens on line
        *s = 0;                                        // null terminate token string
        tok.toks[toknum].tok_str = strdup(t);          // dup token string 
        toksvalue(t, toknum);                          // determine token and its value
        if (toknum == 0)
            tok.linetype = tok.toks[0].toktype;
        if (tok.toks[toknum].toktype == comment) {     // stop calling toksvalue() when comment found
            *s = ' ';
            tok.toks[toknum].tok_str = strdup(t);      // dup remainder of line
            break;
        }
    }
    tok.toks[numtoks++].toktype = endd;                // Place endd token on toks
    tok.numtoks = numtoks;                             // number of tokens on this line
}

/*
 printtok - print a struct toki_t to stdout
 input
  struct toki_t t - the tokenized value of a line in the assembly file
 output
  tokenized line displayed on stdout
 */
void printtok(struct toki_t t) {
    printf("linenum: %d, line type: %s, numtoks: %d, address: %d section: %d\n", t.linenum, toks_t_str[t.linetype], t.numtoks, t.address, t.section);
    if (t.linetype == inst) {
        printf("  inst type: %s, ", ins[t.insttype].inst_str);
        printf("  inst cate: %d, ", t.instcate);
        printf("  inst opcd: 0x%08x\n", t.instopcd);
    }
    for (int j = 0; j < t.numtoks; j++) {
        printf("t.toks[%d].tok_str: %7s | ", j, t.toks[j].tok_str);
        printf("t.toks[%d].toktype: %10s | ", j, toks_t_str[t.toks[j].toktype]);
        printf("t.toks[%d].tokv   : %7d | ", j, t.toks[j].tokv);
        printf("\n");
    }
    printf("\n");
}

/*
 addrsymopcode - generate symbol table and put address in each line's toks[i].address member
 inputs
  struct toki_t toks[] - array of all lines in assembly file - tokenized
 output
  toks[i].address - lines that are number or inst have an address
  toks[i].instopcd - lines that are inst have an instruction opcode
  symbol table - lines that are labels are added to symbol table - key is label, value is address
  NOTE: Symbol table maintained via dictput function
  struct toki_t toks[].address - address of all labels, numbers, and instructions filled in
  struct toki_t toks[].instopcd - opcodes of instructions filled in
 */
void addrsymopcode() {
    int address = 0;  // assume code/data is at address 0
    enum sections section = DATA; // section: DATA, TEXT
    for (int i = 0; i < linenum-1; i++) {
        if (toks[i].linetype == data)
            section = DATA;
        else if (toks[i].linetype == text)
            section = TEXT;
        // .text 0x200 and .data 0x300 directives control addresses of code/data
        if ((toks[i].linetype == data || toks[i].linetype == text) && toks[i].numtoks == 3)
            address = toks[i].toks[1].tokv; // update address based on .data and .text directives
        else if (toks[i].linetype == label) {
            if (toks[i].toks[0].tok_str[2] == 'a') { // .label directive
                toks[i].address = address;
                toks[i].section = section;
                if (dictput(toks[i].toks[1].tok_str, toks[i].address, toks[i].section)) // add to symbol table
                    toks[i].linetype = err;     // duplicate symbol
            }
            else {
                toks[i].address = address;
                toks[i].section = section;
                struct dictval dv;
                dv.key = toks[i].toks[1].tok_str;
                dv.section = toks[i].section;
                if (toks[i].toks[0].tok_str[2] == 'o') { // .local directive
                    dv.ivalue = toks[i].address;
                    dv.type = LOCAL;
                    dv.svalue = 0;
                }
                else {
                    dv.type = STRING;
                    dv.ivalue = 0;
                    dv.svalue = "XXXXX";
                }
                if (dictputval(&dv))
                    toks[i].linetype = err;     // duplicate symbol
            }
        }
        else if (toks[i].linetype == number || toks[i].linetype == inst || toks[i].linetype == ident) {
            toks[i].address = address;
            address+= 4;
            if (toks[i].linetype == inst) {
                toks[i].instopcd = isinst(toks[i]);
                if (toks[i].instopcd < 0)
                    toks[i].linetype = err;
               /*
                Arithmetic logic instructions can have op2 as a register or a 16-bit 2's complement immediate.
                add r1, r2, r3  // 3 register operands
                add r1, r2, 3   // op2 is the number 3
                Parsing to this point has all opcodes as those for 3 register operands.
                At this point we update the opcode for instructions that have op2 as an immediate
                */
                if (toks[i].instcate == arilog) { // arithmetic logic instruction
                    if (toks[i].toks[5].toktype == reg || toks[i].toks[5].toktype == number) {
                        if (toks[i].toks[5].toktype == number) { // op2 is an immediate
                            toks[i].instopcd &= 0x0f;  // convert instruction from 0x5? to 0x6?
                            toks[i].instopcd |= 0x60;
                        }
                    }
                    else
                        toks[i].linetype = err;
                }
            }
        }
        else if (toks[i].linetype == string && toks[i].toks[1].toktype == comment) {
            toks[i].address = address;
            //char *str = strdup(toks[i].toks[1].tok_str + 2);
            int l = strlen(toks[i].toks[1].tok_str + 2) - 1; // skip comment chars and subtract extra space
            if (l % 4 == 0)
                l++;                             // add one to null terminate the string
            //int l4 = l + 3 & ~0x3;             // make l a multiple of 4
            address = address + (l + 3 & ~0x3);  // make l a multiple of 4
        }
    }
}

/*
 identifiers - lookup identifiers in symbol table and assign the value to toks[i].toks[j].tokv
 inputs
  struct toki_t toks[] - array of all lines in assembly file - tokenized with addresses
 output
  struct toki_t toks[].toks[j].tokv - For all tokens that are identifiers, the function identifiers places their value
  in the tokv member. The value is their addres. The function addrsymopcode filled in the values of indentifiers
  NOTE: Symbol table is used to lookup identifier and return its value (address)
 */
void identifiers() {
    for (int i = 0; i < linenum-1; i++)
        if (toks[i].linetype == inst || toks[i].linetype == ident)
            for (int j = 0; j < toks[i].numtoks-1; j++) // numtoks-1 to ignore endd token
                if (toks[i].toks[j].toktype == ident) {
                // HERE
                    struct dictval dv;
                    if (dictgetval(toks[i].toks[j].tok_str, &dv))
                        toks[i].linetype = err;
                    else {
                        if (dv.type == INT || dv.type == LOCAL)
                            toks[i].toks[j].tokv = dv.ivalue;
                        else
                            toks[i].toks[j].tokv = 0xfffff;
                    }
                    //toks[i].toks[j].tokv = dictget(toks[i].toks[j].tok_str);
                    //if (toks[i].toks[j].tokv == -1000001)
                    //    toks[i].linetype = err;
                }
}

/*
 errors - generate error messages if any
 inputs
  struct toki_t toks[] - array of all lines in assembly file - tokenized with errors (if any)
 output
  error messages to stdout
 return 
  0 - no errors in assembly input
  1 - errors in assembly input
 */
int errors() {
    int errorfound = 0;
    for (int i = 0; i < linenum-1; i++) {
        if (toks[i].linetype > comment && toks[i].linetype != number && toks[i].linetype != ident && toks[i].linetype != def) {
            fprintf(stderr, "+++ Error +++ Line number: %d\n", toks[i].linenum);
            for (int j = 0; j < toks[i].numtoks-1; j++) // numtoks-1 to ignore endd token
                fprintf(stderr, "%s ", toks[i].toks[j].tok_str);
            fprintf(stderr, "\n");
            errorfound = 1;
        // Check for other errors, e.g. add r, r5, r5   .label without following num or inst
        }
    }
    return errorfound;
}

int verbose = 0;  // non-zero turns on verbose output - debug information
int symbols = 1;  // zero suppresses symbols from output - useful for OS

/*
 TODO
 I currently recoginize instructions referencing external symbols as those with the largest 20-bit address.
 Symbols in dictionary can be type INT or type STRING. 
 Type INT symbols have an integer value. Type STRING symbols do not.
 Type STRING symbols are added to dictionary for .extern gusty directives.
   gusty has a string value of XXXXX
 When a symbol of type STRING is fetched from the dictionary, the token value is set to 0xfffff.
   toks[i].toks[j].tokv = 0xfffff;
 I use this 0xfffff value in generated code to recognize ldr, ldb, str, stb, and mva instructions
 that are referencing external symbols.
 This approach works, but it is an awkward design - using the largest 20-bit address to mark externals.
 */

/*
 generatecode - generate code to stdout
 inputs
  struct toki_t toks[] - array of all lines in assembly file
  A toki_t struct has all needed to generate a 32-bit hex value for the code
 output
  code is displayed on stdout
 */
void generatecode() {
    int instr, opcode, rd, rm, rn, imm;
    for (int i = 0; i < linenum-1; i++) {
        if (toks[i].linetype == inst) {
          char xtern = 0;
          opcode = toks[i].instopcd;
          switch (toks[i].instcate) {
            case ldrstr:
              rd = toks[i].toks[1].tokv;
              switch (opcode & 0xf) {
                case ADDR: // ldr rd, addr
                    imm = toks[i].toks[3].tokv & 0xfffff;
                    instr = opcode << 24 | rd << 20 | imm;
                    if (imm == 0xfffff) // This works for now. Largest 20-bit addr
                        xtern = 1;
                    break;
                case BASE: // ldr rd, [rm]
                    rm = toks[i].toks[4].tokv;
                    instr = opcode << 24 | rd << 20 | rm << 16;
                    break;
                case BASE_OFF: case PREINC_OFF: // ldr rd, [rm, #3] and ldr rd, [rm, #3]!
                    rm = toks[i].toks[4].tokv;
                    imm = toks[i].toks[6].tokv & 0xffff;
                    instr = opcode << 24 | rd << 20 | rm << 16 | imm;
                    break;
                case BASE_REG: case PREINC_REG: // ldr rd, [rm, rn] and ldr rd, [rm, rn]!
                    rm = toks[i].toks[4].tokv;
                    rn = toks[i].toks[6].tokv;
                    instr = opcode << 24 | rd << 20 | rm << 16 | rn << 12;
                    break;
                case POSTINC_OFF: // ldr rd, [rm], #3
                    rm = toks[i].toks[4].tokv;
                    imm = toks[i].toks[7].tokv & 0xffff;
                    instr = opcode << 24 | rd << 20 | rm << 16 | imm;
                    break;
                case POSTINC_REG: // ldr rd, [rm], rn
                    rm = toks[i].toks[4].tokv;
                    rn = toks[i].toks[7].tokv;
                    instr = opcode << 24 | rd << 20 | rm << 16 | rn << 12;
                    break;
                default:
                    instr = -1;
                    break;
              }
              if (verbose)
                  printf("ldrstr: ");
              if (xtern) { // extract opcode and reg from instr
                  int opreg = (instr >> 20) & 0xfff; // & incase >> is arithmetic
                  printf("0x%03x", opreg);
                  printf("XXXXX");
                  printf(" %s\n", toks[i].toks[3].tok_str);
              }
              else
                  printf("0x%08x\n", instr);
                
              break;
            case arilog:;
              rd = toks[i].toks[1].tokv;
              rm = toks[i].toks[3].tokv;
              rn = toks[i].toks[5].tokv;
              if (opcode >> 4 == ADD_RD_RM_RN)
                  instr = opcode << 24 | rd << 20 | rm << 16 | rn << 12;
              else {
                  imm = toks[i].toks[5].tokv & 0xffff;
                  instr = opcode << 24 | rd << 20 | rm << 16 | imm;
              }
              if (verbose)
                  printf("arilog: 0x%08x\n", instr);
              else
                  printf("0x%08x\n", instr);
              break;
            case movcmp:
              if (opcode >> 4 == MOV_RD_RM) { // mov r,r -> 0x70 for r,r and 0x80 for r, #n
                  rd = toks[i].toks[1].tokv;
                  rm = toks[i].toks[3].tokv;
                  instr = opcode << 24 | rd << 20 | rm << 16;
              }
              else {                  // mov r,#n -> 0x70 for r,r and 0x80 for r, #n
                  rd = toks[i].toks[1].tokv;
                  imm = toks[i].toks[3].tokv & 0xfffff;
                  instr = opcode << 24 | rd << 20 | imm;
                  if ((opcode & 0xf) == MVA && imm == 0xfffff) { // This works for now. Largest 20-bit addr
                      printf("0x%02x%xXXXXX %s\n", opcode, rd, toks[i].toks[3].tok_str);
                      break;
                  }
              }
              if (verbose)
                  printf("movcmp: 0x%08x\n", instr);
              else
                  printf("0x%08x\n", instr);
              break;
            case branch:
            /*
             branches have three opcodes: 0x9?, 0xa?, and 0xb?.
             The upper 4-bits of opcode determines the dest, which can be label, [r], or !label. For example,
             bal label  has opcode 0x90
             bal [r0]   has opcode 0xa0
             bal !label has opcode 0xb0
             The bottom 4-bits of opcode determines bal, beq, bne, blt, ble, bgt, bge, and blt
             The following code examines the upper 4-bits of the opcode to determine dest format.
             */
              if (opcode >> 4 == B_REG) { // bal [r]
                  rd = toks[i].toks[2].tokv;
                  instr = opcode << 24 | rd << 20;
              }
              else if (opcode >> 4 == B_ADDR) { // bal addr
                  imm = toks[i].toks[1].tokv & 0xfffff;
                  instr = opcode << 24 |  imm;
              }
              else { // B_REL: bal !addr - label is in toks[2], ! is in toks[1]
                  int offset = toks[i].toks[2].tokv - toks[i].address;
                  imm = offset & 0xfffff;
                  instr = opcode << 24 |  imm;
              }
              if (verbose)
                  printf("branch: 0x%08x\n", instr);
              else
                  printf("0x%08x\n", instr);
              break;
            case miscos:
              if ((opcode & 0xf) <= RFI) {
                  imm = toks[i].toks[1].tokv & 0xfffff; // ker #num, srg #num, ioi #num, rfi
                  instr = opcode << 24 | imm;
                  if (verbose)
                      printf("miscos: 0x%08x\n", instr);
                  else
                      printf("0x%08x\n", instr);
              }
              else { // mkd and mks instructions
                  rd = toks[i].toks[1].tokv;
                  rm = toks[i].toks[3].tokv;
                  instr = opcode << 24 | rd << 20 | rm << 16;
                  if (verbose)
                      printf("miscos: 0x%08x\n", instr);
                  else
                      printf("0x%08x\n", instr);
              }
              break;
            default:
              printf("default\n");
              break;
          }
        }
        else if (toks[i].linetype == number || toks[i].linetype == ident) {
            if (verbose)
                printf("number: 0x%08x\n", toks[i].toks[0].tokv);
            else
                printf("0x%08x\n", toks[i].toks[0].tokv);
        }
        else if (toks[i].linetype == string && toks[i].toks[1].toktype == comment) {
            int l = strlen(toks[i].toks[1].tok_str + 2) - 1; // skip comment chars and subtract extra space
            char strbuf[80];
            for (int i = 0; i < 80; i++)
                strbuf[i] = 0;
            strncpy(strbuf, toks[i].toks[1].tok_str + 2, l);
            if (l % 4 == 0)
                l++;                 // add one to null terminate the string
            int l4 = l + 3 & ~0x3;   // make l a multiple of 4
            if (verbose)
                printf(".string %s, len: %lu, len-1: %d, mul4: %d\n", strbuf, strlen(strbuf), l, l4);
            for (int i = 0; i < l4; i+=4) {
                int v = strbuf[i]<<24 | strbuf[i+1]<<16 | strbuf[i+2]<<8 | strbuf[i+3];
                if (verbose)
                    printf("string: 0x%08x\n", v);
                else
                    printf("0x%08x\n", v);
            }
        }
        else if (toks[i].linetype == text || toks[i].linetype == data) {
            printf("%s", toks[i].toks[0].tok_str);
            if (toks[i].numtoks >= 3) // .text 100 - 3 accomodates endd token
                printf(" 0x%x", toks[i].toks[1].tokv);
            printf("\n");
        }
     }
}

static char *listing = NULL;

void generatelisting() {
    FILE *f = fopen(listing, "w");
    for (int i = 0; i < linenum-1; i++) {
        fprintf(f, "0x%08x  ", toks[i].address);
        for (int j = 0; j < toks[i].numtoks-1; j++) {
            fprintf(f, "%s ", toks[i].toks[j].tok_str);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

// dodefines processes lines 
// 1. looking for macros on #define lines and processing them
// 2. looking for macro invocations and performing substitutions
void dodefines() {
    for (int i = 0; i < linenum-1; i++) {
        // #define must be on line before its use.
        if (toks[i].linetype == def) { // check for proper #define statement
        // #define <id> <num> looks like the following token stream
        // tok[0].str -> #define, tok[0].type -> define, tok[0].tokv -> 0
        // tok[1].str -> <id>,    tok[1].type -> ident,  tok[1].tokv -> 0
        // tok[2].str -> <num>,   tok[2].type -> number, tok[2].tokv -> value of <num>
        // tok[3].str -> NULL,    tok[3].type -> endd,   tok[3].tokv -> 0
            struct defdictval ddv;
            if (toks[i].numtoks == 4 && toks[i].toks[1].toktype == ident && toks[i].toks[2].toktype == number) {
                ddv.key = strdup(toks[i].toks[1].tok_str);
                ddv.type = DEFNUM;
                ddv.defvalue = strdup(toks[i].toks[2].tok_str);
                ddv.ivalue = toks[i].toks[2].tokv;
                defdictputval(&ddv);
                // add string to symbol dictionary to prevent reusing #define <id> as a label somewhere
                // omit this for now because it puts #define <id> into .ymbl section of .o
                // We do not need this symbols in the .o. Create a way to mark these so as not to include them
                //dictput(strdup(toks[i].toks[1].tok_str), 0, 0);
            }
            else if (toks[i].numtoks == 4 && toks[i].toks[1].toktype == ident) {
                ddv.key = strdup(toks[i].toks[1].tok_str);
                ddv.type = DEFID;
                ddv.defvalue = strdup(toks[i].toks[2].tok_str);
                ddv.ivalue = toks[i].toks[2].tokv;
                defdictputval(&ddv);
                // add string to symbol dictionary to prevent reusing #define <id> as a label somewhere
                // omit this for now because it puts #define <id> into .ymbl section of .o
                // We do not need this symbols in the .o. Create a way to mark these so as not to include them
                //dictput(strdup(toks[i].toks[1].tok_str), 0, 0);
            }
            else { // error
                toks[i].linetype = err;
            }
        }
        if (toks[i].linetype != comment && toks[i].linetype != def) {
            for (int j = 0; j < toks[i].numtoks-1; j++) {
                if (toks[i].toks[j].toktype == ident) {
                    struct defdictval ddv;
                    int x = defdictgetval(toks[i].toks[j].tok_str, &ddv);
                    if (x == 0) {
                        if (ddv.type == DEFNUM) {
                            toks[i].toks[j].toktype = number;
                            toks[i].toks[j].tok_str = ddv.defvalue;
                            toks[i].toks[j].tokv = ddv.ivalue;
                        }
                        else if (ddv.type == DEFID) {
                            toks[i].toks[j].tok_str = ddv.defvalue;
                            tok = toks[i];
                            toksvalue(toks[i].toks[j].tok_str, j);
                            toks[i] = tok;
                        }
                    }
                }
            }
        }
    }
}

/*
  $ chasm code.s : asm code.s into code.o
  $ chasm -v code.s : asm code.s into code.o, verbose mode
  $ chasm -y code.s : asm code.s into code.o, omit symbols from .o
  $ chasm -lfile.txt code.s : asm code.s into code.o, generate listing file.txt
  The long versions of the options are invoked a la 
  $ chasm -verbose code.s
 */
static struct option long_options[] = {
  {"verbose",no_argument,       0,  'v' },
  {"symbol", no_argument,       0,  'y' },
  {"list",   required_argument, 0,  'l' },
  {0,        0,                 0,   0  }
};

/*
 get_opts - called to process command line options
 input
  count - copy of argc
  args  - copy of argv
 output
  listing - -lfile sets listing to "file"
  return of 0 is error
  return of non 0 is index in argv of assembly file name
 */
int get_opts(int count, char *args[]) {
    int opt, good = 1, long_index;
    while (good && (opt = getopt_long(count, args, "l:vy", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'l':
                listing = strdup(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            case 'y':
                symbols = 0;
                break;
            default:
                fprintf(stderr, "Invalid invocation.\n");
                good = 0;
                break;    
        }
    }
    if(good && optind > count-1) {
        fprintf(stderr, "Number of arguments. %d\n", optind);
        good = 0;
    }
    else if (good)
        good = optind;
    return good;

}
/*
 main - entry point for chasm.
 Options
 -l file : generate a listing in file
 -y : turn off symbols in the output file
 -v : turn on verbose more
 Sample Invocations
 ./chasm file.s
 ./chasm file.s > file.o
 ./chasm -l listfile.txt file.s - listing file is not implmented
 ./chams -y file.s
 ./chams -y file.s > file.o
 ./chasm -v file.s
 */

int main(int argc, char **argv) {
    int optind = get_opts(argc, argv);
    if (!optind) {
        fprintf(stderr, "Bad chasm command invocation!\n");
        return -1;
    }

    char *input = argv[optind];
    char *base = basename(strdup(input));
    char *dot = strrchr(base, '.');
    if (dot)
        *dot = '\0';
    char *output = malloc(256);
    strcpy(output, base);
    strcat(output, ".o");
    if (verbose)
        fprintf(stderr, "input: %s, output: %s\n", input, output);
    // An open-do-close pattern to process the assembly file
    FILE *fp;
    if ((fp = fopen(input, "r")) == NULL) {
        fprintf(stderr, "File %s not found!\n", input);
        return -1;
    }
    // chasm writes to stdout. dup2 output to stdout
    int fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int dup2stat = dup2(fd, 1);
    if (fd == -1 || dup2stat == -1) {
        fprintf(stderr, "File %s not created!\n", output);
        return -1;
    }
    close(fd);
    // Loop - reads line, tokenizes line in tok, saves tok in array toks
    while (fgets(buf, MAX_LINE, fp)) {
        buf[strcspn(buf, "\n")] = '\0';
        if (verbose)
            printf("%s\n", buf);
        tokenize();              // convert buf to a struct toki_t tok
        tok.linenum = linenum;
        toks[linenum-1] = tok;   // save token in array of tokens
        linenum++;
        if (linenum >= MAX_PROG_LINES)
            fprintf(stderr, "Too many lines, Max: %d\n", MAX_PROG_LINES);
    }
    fclose(fp);                  // end open-do-close on assembly file
    dodefines();                 // create macros and substitute macro invocations
    addrsymopcode();             // create symbol table and fill in toks[].address
    identifiers();
    if (!errors()) {
        generatecode();
        if (symbols) {
            printf(".ymbl\n");
            dictprint(0); // symbol table
        }
        if (listing) {
            generatelisting();
        }
    }
    if (verbose) {
        printf("*******************************\n");
        dictprint(verbose);
        for (int i = 0; i < linenum-1; i++)
            printtok(toks[i]);
    }

    /*
    struct defdictval ddv;
    int x = defdictgetval("x", &ddv);
    if (x == 0) {
        printf("defdictget: %s, %d, %s, %d\n", ddv.key, ddv.type, ddv.defvalue, ddv.ivalue);
    }
    else
        printf("GUSTY\n");
    dictput("Gusty", 22);
    printf("dictget: %d\n", dictget("Gusty"));
    printf("dictget: %d\n", dictget("gusty"));
    dictprint();

    printf("isreg(r1): %d\n", isreg("r1"));
    printf("isreg(r1): %d\n", isreg("r9"));
    printf("isreg(r15): %d\n", isreg("r15"));
    printf("isreg(r15): %d\n", isreg("r16"));
    printf("isreg(r1x): %d\n", isreg("r1x"));
    printf("goodnumber(ab12, hexdigits): %d\n", goodnumber("ab12", hexdigits));
    printf("goodnumber(abx12, hexdigits): %d\n", goodnumber("abx12", hexdigits));
    printf("goodnumber(123, decdigits): %d\n", goodnumber("123", decdigits));
    printf("goodnumber(12x, decdigits): %d\n", goodnumber("12x", decdigits));
    printf("isnumber(123): %d\n", isnumber("123"));
    printf("isnumber(0xab12): %d\n", isnumber("0xab12"));
    printf("isnumber(#123): %d\n", isnumber("#123"));
    printf("isnumber(#0xab12): %d\n", isnumber("#0xab12"));
    printf("stol(-123): %d\n", (int)strtol("-123", NULL, 10));

    printf("isinst(): %x\n", isinst(toks[0]));
    printf("isinst(): %x\n", isinst(toks[1]));
    printf("isinst(): %x\n", isinst(toks[2]));
    printf("isinst(): %x\n", isinst(toks[3]));
    printf("isinst(): %x\n", isinst(toks[4]));
    printf("isinst(): %x\n", isinst(toks[5]));
    printf("isinst(): %x\n", isinst(toks[6]));
    printf("isinst(): %x\n", isinst(toks[7]));
    printf("isinst(): %x\n", isinst(toks[8]));
    printf("isinst(): %x\n", isinst(toks[9]));
    printf("isinst(): %x\n", isinst(toks[10]));
    printf("isinst(): %x\n", isinst(toks[11]));
    printf("isinst(): %x\n", isinst(toks[12]));
    */

}
