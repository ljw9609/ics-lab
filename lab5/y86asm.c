#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "y86asm.h"

line_t *y86bin_listhead = NULL;   /* the head of y86 binary code line list*/
line_t *y86bin_listtail = NULL;   /* the tail of y86 binary code line list*/
int y86asm_lineno = 0; /* the current line number of y86 assemble code */

#define err_print(_s, _a ...) do { \
  if (y86asm_lineno < 0) \
    fprintf(stderr, "[--]: "_s"\n", ## _a); \
  else \
    fprintf(stderr, "[L%d]: "_s"\n", y86asm_lineno, ## _a); \
} while (0);

int vmaddr = 0;    /* vm addr */

/* register table */
reg_t reg_table[REG_CNT] = {
    {"%eax", REG_EAX},
    {"%ecx", REG_ECX},
    {"%edx", REG_EDX},
    {"%ebx", REG_EBX},
    {"%esp", REG_ESP},
    {"%ebp", REG_EBP},
    {"%esi", REG_ESI},
    {"%edi", REG_EDI},
};

regid_t find_register(char *name)   //done!
{   
	int i;
    for(i = 0;i < REG_CNT;i++){
        if(!strncmp(name, reg_table[i].name, 4))
            return reg_table[i].id;
    }
    return REG_ERR;
}

/* instruction set */
instr_t instr_set[] = {
    {"nop", 3,   HPACK(I_NOP, F_NONE), 1 },
    {"halt", 4,  HPACK(I_HALT, F_NONE), 1 },
    {"rrmovl", 6,HPACK(I_RRMOVL, F_NONE), 2 },
    {"cmovle", 6,HPACK(I_RRMOVL, C_LE), 2 },
    {"cmovl", 5, HPACK(I_RRMOVL, C_L), 2 },
    {"cmove", 5, HPACK(I_RRMOVL, C_E), 2 },
    {"cmovne", 6,HPACK(I_RRMOVL, C_NE), 2 },
    {"cmovge", 6,HPACK(I_RRMOVL, C_GE), 2 },
    {"cmovg", 5, HPACK(I_RRMOVL, C_G), 2 },
    {"irmovl", 6,HPACK(I_IRMOVL, F_NONE), 6 },
    {"rmmovl", 6,HPACK(I_RMMOVL, F_NONE), 6 },
    {"mrmovl", 6,HPACK(I_MRMOVL, F_NONE), 6 },
    {"addl", 4,  HPACK(I_ALU, A_ADD), 2 },
    {"subl", 4,  HPACK(I_ALU, A_SUB), 2 },
    {"andl", 4,  HPACK(I_ALU, A_AND), 2 },
    {"xorl", 4,  HPACK(I_ALU, A_XOR), 2 },
    {"jmp", 3,   HPACK(I_JMP, C_YES), 5 },
    {"jle", 3,   HPACK(I_JMP, C_LE), 5 },
    {"jl", 2,    HPACK(I_JMP, C_L), 5 },
    {"je", 2,    HPACK(I_JMP, C_E), 5 },
    {"jne", 3,   HPACK(I_JMP, C_NE), 5 },
    {"jge", 3,   HPACK(I_JMP, C_GE), 5 },
    {"jg", 2,    HPACK(I_JMP, C_G), 5 },
    {"call", 4,  HPACK(I_CALL, F_NONE), 5 },
    {"ret", 3,   HPACK(I_RET, F_NONE), 1 },
    {"pushl", 5, HPACK(I_PUSHL, F_NONE), 2 },
    {"popl", 4,  HPACK(I_POPL, F_NONE),  2 },
    {".byte", 5, HPACK(I_DIRECTIVE, D_DATA), 1 },
    {".word", 5, HPACK(I_DIRECTIVE, D_DATA), 2 },
    {".long", 5, HPACK(I_DIRECTIVE, D_DATA), 4 },
    {".pos", 4,  HPACK(I_DIRECTIVE, D_POS), 0 },
    {".align", 6,HPACK(I_DIRECTIVE, D_ALIGN), 0 },
    {NULL, 1,    0   , 0 } //end
};

instr_t *find_instr(char *name) //done!
{   
	int i;
    for(i = 0;instr_set[i].name;i++){
        if(strncmp(instr_set[i].name, name, instr_set[i].len) == 0)
            return &instr_set[i];
    }
    return NULL;
}

/* symbol table (don't forget to init and finit it) */
symbol_t *symtab = NULL;

/*
 * find_symbol: scan table to find the symbol
 * args
 *     name: the name of symbol
 *
 * return
 *     symbol_t: the 'name' symbol
 *     NULL: not exist
 */
//done!
symbol_t *find_symbol(char *name)
{   
    symbol_t *ptr = symtab->next;
    while(ptr){
        if(!strcmp(ptr->name, name))
            return ptr;
        ptr = ptr->next;
    }
    return NULL;
}

/*
 * add_symbol: add a new symbol to the symbol table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
//done!
int add_symbol(char *name)
{    
    /* check duplicate */
    symbol_t *ptr = symtab->next;
    while(ptr){
        if(!strcmp(ptr->name, name))
            return -1;
        ptr = ptr->next;
    }
    /* create new symbol_t (don't forget to free it)*/
    symbol_t *new_ptr = (symbol_t*)malloc(sizeof(symbol_t));
    new_ptr->name = name;
    new_ptr->addr = vmaddr;
    new_ptr->next = NULL;
    /* add the new symbol_t to symbol table */
    new_ptr->next = symtab->next;
    symtab->next = new_ptr;

    return 0;
}

/* relocation table (don't forget to init and finit it) */
reloc_t *reltab = NULL;

/*
 * add_reloc: add a new relocation to the relocation table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
void add_reloc(char *name, bin_t *bin)
{
    /* create new reloc_t (don't forget to free it)*/
    reloc_t* reloc = (reloc_t*)malloc(sizeof(reloc_t));
    reloc->y86bin = bin;
    reloc->name = name;
    reloc->next = NULL;
    /* add the new reloc_t to relocation table */
    reloc->next = reltab->next;
    reltab->next = reloc;   //添加在reloc表首
}


/* macro for parsing y86 assembly code */
#define IS_DIGIT(s) ((*(s)>='0' && *(s)<='9') || *(s)=='-' || *(s)=='+')
#define IS_LETTER(s) ((*(s)>='a' && *(s)<='z') || (*(s)>='A' && *(s)<='Z'))
#define IS_COMMENT(s) (*(s)=='#')
#define IS_REG(s) (*(s)=='%')
#define IS_IMM(s) (*(s)=='$')

#define IS_BLANK(s) (*(s)==' ' || *(s)=='\t')
#define IS_END(s) (*(s)=='\0')

#define SKIP_BLANK(s) do {  \
  while(!IS_END(s) && IS_BLANK(s))  \
    (s)++;    \
} while(0);

/* return value from different parse_xxx function */
typedef enum { PARSE_ERR=-1, PARSE_REG, PARSE_DIGIT, PARSE_SYMBOL, 
    PARSE_MEM, PARSE_DELIM, PARSE_INSTR, PARSE_LABEL} parse_t;

/*
 * parse_instr: parse an expected data token (e.g., 'rrmovl')
 * args
 *     ptr: point to the start of string
 *     inst: point to the inst_t within instr_set
 *
 * return
 *     PARSE_INSTR: success, move 'ptr' to the first char after token,
 *                            and store the pointer of the instruction to 'inst'
 *     PARSE_ERR: error, the value of 'ptr' and 'inst' are undefined
 */
//done!
parse_t parse_instr(char **ptr, instr_t **inst)
{
    /* skip the blank */
    char* p = *ptr;
    SKIP_BLANK(p);
    if(IS_END(p)) 
        return PARSE_ERR; 
    /* find_instr and check end */
    instr_t* temp_inst;
    temp_inst = find_instr(p);
    if(temp_inst == NULL) 
        return PARSE_ERR;
    if(!IS_BLANK(p+temp_inst->len)&&!IS_END(p+temp_inst->len))
        return PARSE_ERR;
    /* set 'ptr' and 'inst' */
    *ptr = p + temp_inst->len;  //指针后移，指令长度个字节
    *inst = temp_inst;

    return PARSE_INSTR;
}

/*
 * parse_delim: parse an expected delimiter token (e.g., ',')
 * args
 *     ptr: point to the start of string
 *
 * return
 *     PARSE_DELIM: success, move 'ptr' to the first char after token
 *     PARSE_ERR: error, the value of 'ptr' and 'delim' are undefined
 */
//done!
parse_t parse_delim(char **ptr, char delim)
{
    /* skip the blank and check */
    char* p = *ptr;
    SKIP_BLANK(p);
    if(IS_END(p))
        return PARSE_ERR;
    if(*p != delim)
        return PARSE_ERR;
    /* set 'ptr' */
    *ptr = p + 1;   //指针后移，符号1字节
    return PARSE_DELIM;
}

/*
 * parse_reg: parse an expected register token (e.g., '%eax')
 * args
 *     ptr: point to the start of string
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_REG: success, move 'ptr' to the first char after token, 
 *                         and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr' and 'regid' are undefined
 */
//done!
parse_t parse_reg(char **ptr, regid_t *regid)
{
    /* skip the blank and check */
    char* p = *ptr;
    SKIP_BLANK(p);
    if(IS_END(p))
        return PARSE_ERR;
    if(*p != '%')
        return PARSE_ERR;
    /* find register */
    regid_t temp_reg;
    temp_reg = find_register(p);
    if(temp_reg == REG_ERR)
        return PARSE_ERR;
    /* set 'ptr' and 'regid' */
    *ptr = p + 4;   //指针后移，register4个字节
    *regid = temp_reg;
    return PARSE_REG;
}

/*
 * parse_symbol: parse an expected symbol token (e.g., 'Main')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_SYMBOL: success, move 'ptr' to the first char after token,
 *                               and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' and 'name' are undefined
 */
//done!
parse_t parse_symbol(char **ptr, char **name)
{
    /* skip the blank and check */
    char* p = *ptr;
    SKIP_BLANK(p);
    if(IS_END(p) || !IS_LETTER(p))
        return PARSE_ERR;
    /* allocate name and copy to it */
    int name_len = 0;
    while(IS_LETTER(p+name_len) || IS_DIGIT(p+name_len)){
        name_len ++;
    }
    char *temp_name;
    temp_name = (char*)malloc(name_len+1);  //末尾\0 
    int i;
    for(i = 0;i < name_len;i++){
        temp_name[i] = p[i];
    }
    temp_name[name_len] = '\0'; //末尾设\0
    /* set 'ptr' and 'name' */
    *ptr = p + name_len;    //指针右移，symbol长度
    *name = temp_name;
    return PARSE_SYMBOL;
}

/*
 * parse_digit: parse an expected digit token (e.g., '0x100')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, move 'ptr' to the first char after token
 *                            and store the value of digit to 'value'
 *     PARSE_ERR: error, the value of 'ptr' and 'value' are undefined
 */
//done!
parse_t parse_digit(char **ptr, long *value)
{
    /* skip the blank and check */
    char* p = *ptr;
    SKIP_BLANK(p);
    if(IS_END(p) || !IS_DIGIT(p))
        return PARSE_ERR;
    /* calculate the digit, (NOTE: see strtoll()) */
    char* pEnd;
    long l_value = 0;
    int base = 0;

    if(*(p) == '0' && *(p + 1) == 'x')    //判断进制
        base = 16;
    else 
        base = 10;

    l_value = strtoll(p,&pEnd,base);
    /* set 'ptr' and 'value' */
    p = pEnd;
    *ptr = p;
    *value = l_value;
    return PARSE_DIGIT;
}

/*
 * parse_imm: parse an expected immediate token (e.g., '$0x100' or 'STACK')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, the immediate token is a digit,
 *                            move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, the immediate token is a symbol,
 *                            move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
//done!
parse_t parse_imm(char **ptr, char **name, long *value)
{
    /* skip the blank and check */
    char* p = *ptr;
    SKIP_BLANK(p);
    if(IS_END(p) || (!IS_IMM(p) && !IS_LETTER(p)))
        return PARSE_ERR;
    /* if IS_IMM, then parse the digit */
    int cc = 0;
    if(IS_IMM(p)){
        p++;
        cc = parse_digit(&p,value);
        if(cc == PARSE_ERR) 
            return PARSE_ERR;
    }
    /* if IS_LETTER, then parse the symbol */
    if(IS_LETTER(p)){
        cc = parse_symbol(&p,name);
        if(cc == PARSE_ERR)
            return PARSE_ERR;
    }
    /* set 'ptr' and 'name' or 'value' */
    *ptr = p;   //指针已在parse_digit和parse_symbol中移动过

    return cc;
}

/*
 * parse_mem: parse an expected memory token (e.g., '8(%ebp)')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_MEM: success, move 'ptr' to the first char after token,
 *                          and store the value of digit to 'value',
 *                          and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr', 'value' and 'regid' are undefined
 */
//done!
parse_t parse_mem(char **ptr, long *value, regid_t *regid)
{
    /* skip the blank and check */
    char* p = *ptr;
    SKIP_BLANK(p);
    if(IS_END(p) || (!IS_DIGIT(p) && *p != '('))
        return PARSE_ERR;
    /* calculate the digit and register, (ex: (%ebp) or 8(%ebp)) */
    int cc = 0;
    if(IS_DIGIT(p)){    //分析访存地址的常数
        cc = parse_digit(&p,value);
        if(cc == PARSE_ERR)
            return PARSE_ERR;
    }
    else
        *value = 0;

    if(*p == '('){
        p++;
        cc = parse_reg(&p,regid);
        if(cc == PARSE_ERR)
            return PARSE_ERR;
        if(*p != ')')   //要有')'和'('对应
            return PARSE_ERR;
    }

    /* set 'ptr', 'value' and 'regid' */
    *ptr = p + 1;   //')'占一个字节
    return PARSE_MEM;
}

/*
 * parse_data: parse an expected data token (e.g., '0x100' or 'array')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, data token is a digit,
 *                            and move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, data token is a symbol,
 *                            and move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
//done!
parse_t parse_data(char **ptr, char **name, long *value)
{
    /* skip the blank and check */
    char* p = *ptr;
    SKIP_BLANK(p);
    if(IS_END(p) || (!IS_DIGIT(p)&&!IS_LETTER(p)))
        return PARSE_ERR;
    /* if IS_DIGIT, then parse the digit */
    int cc = 0;
    if(IS_DIGIT(p)){
        cc = parse_digit(&p,value);
        if(cc == PARSE_ERR)
            return PARSE_ERR;
    }
    /* if IS_LETTER, then parse the symbol */
    if(IS_LETTER(p)){
        cc = parse_symbol(&p,name);
        if(cc == PARSE_ERR)
            return PARSE_ERR;
    }
    /* set 'ptr', 'name' and 'value' */
    *ptr = p;   //指针已在parse_digit和parse_symbol中移动过
    return cc;
}

/*
 * parse_label: parse an expected label token (e.g., 'Loop:')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_LABEL: success, move 'ptr' to the first char after token
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' is undefined
 */
//done!
parse_t parse_label(char **ptr, char **name)
{
    /* skip the blank and check */
    char* p = *ptr;
    SKIP_BLANK(p);
    if(IS_END(p) || !IS_LETTER(p)){
        return PARSE_ERR;
    }
    /* allocate name and copy to it */
    int name_len = 0;
    while(IS_LETTER(p+name_len) || IS_DIGIT(p+name_len)){
        name_len ++;
    }
    if(*(p+name_len) != ':')   //Label后面有':'
        return PARSE_ERR;

    char* label;
    label = (char*)malloc(name_len+1);
    int i;
    for(i = 0;i < name_len;i++){
        label[i] = p[i];
    }
    label[name_len] = '\0';

    /* set 'ptr' and 'name' */
    *ptr = p + name_len + 1;    //别忘了':''
    *name = label;
    return PARSE_LABEL;
}

/*
 * parse_line: parse a line of y86 code (e.g., 'Loop: mrmovl (%ecx), %esi')
 * (you could combine above parse_xxx functions to do it)
 * args
 *     line: point to a line_t data with a line of y86 assembly code
 *
 * return
 *     PARSE_XXX: success, fill line_t with assembled y86 code
 *     PARSE_ERR: error, try to print err information (e.g., instr type and line number)
 */
//done!
type_t parse_line(line_t *line)
{
    char* y86asm;   //汇编码
    bin_t* y86bin;  //机器码
    char* label = NULL;   
    instr_t* instr = NULL;
    char* p;    //操作时的位置指针
    int cc; 

    /* 初始化 */
    y86bin = &line->y86bin;
    y86asm = (char*)malloc(sizeof(char)*(strlen(line->y86asm)+1));
    strcpy(y86asm,line->y86asm);
    p = y86asm;

/* when finish parse an instruction or lable, we still need to continue check 
* e.g., 
*  Loop: mrmovl (%ebp), %ecx
*           call SUM  #invoke SUM function */

    /* skip blank and check IS_END */
    int flag = 1;   //用一个循环，当未出现TYPE_ERR时就一个读下去，出现ERR后flag设为0，退出循环
while(flag == 1){

    SKIP_BLANK(p);
    if(IS_END(p)){
       break;
    }
    /* is a comment ? */
    if(IS_COMMENT(p)){
        break;
    }
    /* is a label ? */
    cc = parse_label(&p,&label);
    if(cc == PARSE_LABEL){
        if(add_symbol(label) == -1){    //重复symbol
	        line->type = TYPE_ERR;
            err_print("Dup symbol:%s",label);
            break;
        }
        else{
            line->type = TYPE_INS;
            line->y86bin.addr = vmaddr;
            continue;
        }
    }
    /* is an instruction ? */
    cc = parse_instr(&p,&instr);
    if(cc == PARSE_ERR){
        line->type = TYPE_ERR;
        err_print("wrong ins:%s",instr);
        break;
    }
    /* set type and y86bin */
    line->type = TYPE_INS;
    y86bin->addr = vmaddr;
    y86bin->codes[0] = instr->code;
    y86bin->bytes = instr->bytes;
    /* update vmaddr */  
    vmaddr += instr->bytes;  

    /* parse the rest of instruction according to the itype */
    switch(HIGH(instr->code)){
        case I_HALT:
        case I_NOP:
            continue;
        case I_ALU:     //  ALU和RRMOVL相似，都是register to register
        case I_RRMOVL:{
            regid_t regA,regB;
            cc = parse_reg(&p,&regA);
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid REG");
                flag = 0;
                break;
            }
            cc = parse_delim(&p,',');
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid ','");
                flag = 0;
                break;
            }
            cc = parse_reg(&p,&regB);
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid REG");
                flag = 0;
                break;
            }
            y86bin->codes[1] = HPACK(regA,regB);
            continue;
        }
        case I_IRMOVL:{
            char* temp_name;
            long value = 0;
            long *vp = (long*)&y86bin->codes[2];
            cc = parse_imm(&p,&temp_name,&value);
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid Immediate");
                flag = 0;
                break;
            }
            else if(cc == PARSE_SYMBOL){
                add_reloc(temp_name,y86bin);    //将符号添加到重定位表
                value = 0;
                *vp = value;
            }
            else if(cc == PARSE_DIGIT){
                *vp = value;
            }

            cc = parse_delim(&p,',');
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid ','");
                flag = 0;
                break;
            }

            regid_t regB;
            cc = parse_reg(&p,&regB);
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid REG");
                flag = 0;
                break;
            }
            y86bin->codes[1] = HPACK(REG_NONE,regB);

            continue;
        }
        case I_RMMOVL:{
            regid_t regA,regB;
            cc = parse_reg(&p,&regA);
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid REG");
                flag = 0;
                break;
            }

            cc = parse_delim(&p,',');
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid ','");
                flag = 0;
                break;
            }

            long value = 0;
            cc = parse_mem(&p,&value,&regB);
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid MEM");
                flag = 0;
                break;
            }

            y86bin->codes[1] = HPACK(regA,regB);

            long *vp = (long*)&y86bin->codes[2];
            *vp = value;

            continue;
        }
        case I_MRMOVL:{
            regid_t regA,regB;
            long value;
            cc = parse_mem(&p,&value,&regB);
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid MEM");
                flag = 0;
                break;
            }

            cc = parse_delim(&p,',');
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid ','");
                flag = 0;
                break;
            }

            cc = parse_reg(&p,&regA);
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid REG");
                flag = 0;
                break;
            }

            y86bin->codes[1] = HPACK(regA,regB);

            long *vp = (long*)&y86bin->codes[2];
            *vp = value;

            continue;
        }
        case I_JMP:     //JMP和CALL相似，都是JMP/CALL Dest
        case I_CALL:{
            char* temp_name;
            cc = parse_symbol(&p,&temp_name);
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid DEST");
                flag = 0;
                break;
            }

            add_reloc(temp_name,y86bin);    //将符号添加到重定位表

            continue;
        }
        case I_RET:
            continue;
        case I_PUSHL:   //PUSHL和POPL相似，rA:F
        case I_POPL:{
            regid_t regA;
            cc = parse_reg(&p,&regA);
            if(cc == PARSE_ERR){
                line->type = TYPE_ERR;
                err_print("Invalid REG");
                flag = 0;
                break;
            }

            y86bin->codes[1] = HPACK(regA,REG_NONE);
            continue;
        }
        case I_DIRECTIVE:{
            byte_t dtv = LOW(instr->code);
            switch(dtv){
                case D_DATA:{   //比如.long 0x1 或.long data
                    long value;
                    char* name;
                    cc = parse_data(&p,&name,&value);
                    if(cc == PARSE_ERR){
                        line->type = TYPE_ERR;
                        err_print("wrong data");
                        flag = 0;
                        break;
                    }
                    else if(cc == PARSE_SYMBOL){
                        add_reloc(name,y86bin);
                    }
                    else if(cc == PARSE_DIGIT){
                        long* vp = (long*)y86bin->codes;
                        *vp = value;
                    }
                    continue;
                }
                case D_ALIGN:{  //比如.align 4
                    long value;
                    cc = parse_digit(&p,&value);
                    if(cc == PARSE_ERR){
                        line->type = TYPE_ERR;
                        err_print("wrong allign");
                        flag = 0;
                         break;
                    }
                    long adj = 0;   //调整对齐位置
                    adj = vmaddr % value;
                    if(adj != 0){
                        vmaddr = vmaddr + (value - adj);
                    }
                    y86bin->addr = vmaddr;
                    continue;
                }
                case D_POS:{    //比如.pos 0x0
                    long value;
                    cc = parse_digit(&p,&value);
                    if(cc == PARSE_ERR){
                        line->type = TYPE_ERR;
                        err_print("wrong pos");
                        flag = 0;
                        break;
                    }
                    vmaddr = value;
                    y86bin->addr = vmaddr;
                    continue;
                }
                default:{
                    line->type = TYPE_ERR;
                    err_print("unknow dvt");
                    flag = 0;
                    break;
                }
            }
            break;
        }
        default:{
            line->type = TYPE_ERR;
            err_print("wrong instr");
            flag = 0;
            break;
        }
    }
}

    free(y86asm);
    return line->type;
}

/*
 * assemble: assemble an y86 file (e.g., 'asum.ys')
 * args
 *     in: point to input file (an y86 assembly file)
 *
 * return
 *     0: success, assmble the y86 file to a list of line_t
 *     -1: error, try to print err information (e.g., instr type and line number)
 */
int assemble(FILE *in)
{
    static char asm_buf[MAX_INSLEN]; /* the current line of asm code */
    line_t *line;
    int slen;
    char *y86asm;

    /* read y86 code line-by-line, and parse them to generate raw y86 binary code list */
    while (fgets(asm_buf, MAX_INSLEN, in) != NULL) {
        slen  = strlen(asm_buf);
        if ((asm_buf[slen-1] == '\n') || (asm_buf[slen-1] == '\r')) { 
            asm_buf[--slen] = '\0'; /* replace terminator */
        }

        /* store y86 assembly code */
        y86asm = (char *)malloc(sizeof(char) * (slen + 1)); // free in finit
        strcpy(y86asm, asm_buf);

        line = (line_t *)malloc(sizeof(line_t)); // free in finit
        memset(line, '\0', sizeof(line_t));

        /* set defualt */
        line->type = TYPE_COMM;
        line->y86asm = y86asm;
        line->next = NULL;

        /* add to y86 binary code list */
        y86bin_listtail->next = line;
        y86bin_listtail = line;
        y86asm_lineno ++;

        /* parse */
        if (parse_line(line) == TYPE_ERR)
            return -1;
    }

    /* skip line number information in err_print() */
    y86asm_lineno = -1;
    return 0;
}

/*
 * relocate: relocate the raw y86 binary code with symbol address
 *
 * return
 *     0: success
 *     -1: error, try to print err information (e.g., addr and symbol)
 */
int relocate(void)
{
    reloc_t *rtmp = NULL;
    
    rtmp = reltab->next;
    while (rtmp) {
        /* find symbol */
        symbol_t *symb = find_symbol(rtmp->name);
        if(symb == NULL){
            err_print("Unknown symbol:'%s'",rtmp->name);
            return -1;
        }
        /* relocate y86bin according itype */
        byte_t itype = HIGH(rtmp->y86bin->codes[0]);
        long* vp;
        switch(itype){
            case I_IRMOVL:{
                vp = (long*)&rtmp->y86bin->codes[2]; 
                break;
            }
            case I_JMP:
            case I_CALL:{
                vp = (long*)&rtmp->y86bin->codes[1];
                break;
            }
            default:{
                vp = (long*)&rtmp->y86bin->codes[0];
                break;
            }
        }

        *vp = symb->addr;
        /* next */
        rtmp = rtmp->next;
    }
    return 0;
}


/*
 * binfile: generate the y86 binary file
 * args
 *     out: point to output file (an y86 binary file)
 *
 * return
 *     0: success
 *     -1: error
 */
//done!
int binfile(FILE *out)
{
    /* prepare image with y86 binary code */
    line_t *line_ptr = y86bin_listhead;
    int pos = 0;
    char space = '\0';
    /* binary write y86 code to output file (NOTE: see fwrite()) */    
    while(line_ptr != NULL){
	    int flag = 1;  //先判断line_ptr是不是最后一个合法的操作
	    line_t *temp = line_ptr->next;
	    while(temp != NULL){
	        if(temp->y86bin.bytes != 0)
		       flag = 0;
	        temp = temp->next;
	    }

    	if(pos < line_ptr->y86bin.addr && flag == 0){  //填补空格
    		int adj = line_ptr->y86bin.addr - pos;
    		int i;
    		for(i = 0;i < adj;i++){
    			size_t cc = fwrite(&space,sizeof(char),1,out);
    		}
    		pos = line_ptr->y86bin.addr;
    	}

    	size_t cc_2 = fwrite(line_ptr->y86bin.codes, sizeof(byte_t), line_ptr->y86bin.bytes, out);
		if(cc_2 != line_ptr->y86bin.bytes) 
			return -1;

		pos += line_ptr->y86bin.bytes;
		line_ptr = line_ptr->next;
   
    }

    return 0;
}


/* whether print the readable output to screen or not ? */
bool_t screen = TRUE; 

static void hexstuff(char *dest, int value, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        char c;
        int h = (value >> 4*i) & 0xF;
        c = h < 10 ? h + '0' : h - 10 + 'a';
        dest[len-i-1] = c;
    }
}

void print_line(line_t *line)
{
    char buf[26];

    /* line format: 0xHHH: cccccccccccc | <line> */
    if (line->type == TYPE_INS) {
        bin_t *y86bin = &line->y86bin;
        int i;
        
        strcpy(buf, "  0x000:              | ");
        
        hexstuff(buf+4, y86bin->addr, 3);
        if (y86bin->bytes > 0)
            for (i = 0; i < y86bin->bytes; i++)
                hexstuff(buf+9+2*i, y86bin->codes[i]&0xFF, 2);
    } else {
        strcpy(buf, "                      | ");
    }

    printf("%s%s\n", buf, line->y86asm);
}

/* 
 * print_screen: dump readable binary and assembly code to screen
 * (e.g., Figure 4.8 in ICS book)
 */
void print_screen(void)
{
    line_t *tmp = y86bin_listhead->next;
    
    /* line by line */
    while (tmp != NULL) {
        print_line(tmp);
        tmp = tmp->next;
    }
}

/* init and finit */
void init(void)
{
    reltab = (reloc_t *)malloc(sizeof(reloc_t)); // free in finit
    memset(reltab, 0, sizeof(reloc_t));

    symtab = (symbol_t *)malloc(sizeof(symbol_t)); // free in finit
    memset(symtab, 0, sizeof(symbol_t));

    y86bin_listhead = (line_t *)malloc(sizeof(line_t)); // free in finit
    memset(y86bin_listhead, 0, sizeof(line_t));
    y86bin_listtail = y86bin_listhead;
    y86asm_lineno = 0;
}

void finit(void)
{
    reloc_t *rtmp = NULL;
    do {
        rtmp = reltab->next;
        if (reltab->name) 
            free(reltab->name);
        free(reltab);
        reltab = rtmp;
    } while (reltab);
    
    symbol_t *stmp = NULL;
    do {
        stmp = symtab->next;
        if (symtab->name) 
            free(symtab->name);
        free(symtab);
        symtab = stmp;
    } while (symtab);

    line_t *ltmp = NULL;
    do {
        ltmp = y86bin_listhead->next;
        if (y86bin_listhead->y86asm) 
            free(y86bin_listhead->y86asm);
        free(y86bin_listhead);
        y86bin_listhead = ltmp;
    } while (y86bin_listhead);
}

static void usage(char *pname)
{
    printf("Usage: %s [-v] file.ys\n", pname);
    printf("   -v print the readable output to screen\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int rootlen;
    char infname[512];
    char outfname[512];
    int nextarg = 1;
    FILE *in = NULL, *out = NULL;
    
    if (argc < 2)
        usage(argv[0]);
    
    if (argv[nextarg][0] == '-') {
        char flag = argv[nextarg][1];
        switch (flag) {
          case 'v':
            screen = TRUE;
            nextarg++;
            break;
          default:
            usage(argv[0]);
        }
    }

    /* parse input file name */
    rootlen = strlen(argv[nextarg])-3;
    /* only support the .ys file */
    if (strcmp(argv[nextarg]+rootlen, ".ys"))
        usage(argv[0]);
    
    if (rootlen > 500) {
        err_print("File name too long");
        exit(1);
    }


    /* init */
    init();

    
    /* assemble .ys file */
    strncpy(infname, argv[nextarg], rootlen);
    strcpy(infname+rootlen, ".ys");
    in = fopen(infname, "r");
    if (!in) {
        err_print("Can't open input file '%s'", infname);
        exit(1);
    }
    
    if (assemble(in) < 0) {
        err_print("Assemble y86 code error");
        fclose(in);
        exit(1);
    }
    fclose(in);


    /* relocate binary code */
    if (relocate() < 0) {
        err_print("Relocate binary code error");
        exit(1);
    }


    /* generate .bin file */
    strncpy(outfname, argv[nextarg], rootlen);
    strcpy(outfname+rootlen, ".bin");
    out = fopen(outfname, "wb");
    if (!out) {
        err_print("Can't open output file '%s'", outfname);
        exit(1);
    }

    if (binfile(out) < 0) {
        err_print("Generate binary file error");
        fclose(out);
        exit(1);
    }
    fclose(out);
    
    /* print to screen (.yo file) */
    if (screen)
       print_screen(); 

    /* finit */
    finit();
    return 0;
}


