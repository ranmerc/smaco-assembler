#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// For storing line wise instructions
struct instructions
{
    int count;
    char tokens[4][8];
};

// Symbol table
struct symbol_table
{
    char *symbol;
    int addr;
    int used;
    int defined;
    int type; // 0 = memory operand. 1 = symbol
    int value;
    struct symbol_table *next;
};

struct obj_ins
{
    int addr;
    int opcode;
    int register_operand;
    int memory_operand;
};

//Functions
int is_number(char *);
void check_extension(char *fn[], int count);
void open_file(char *);
int in_list(char *str, const char *list[], int num);
int in_list_r(char *str, char *list[], int num); // is this really required?
int is_symbol(char *);
void build_sym_table();
void add_symbol(char *, int, int, int);
void print_sym_table();
void error_sym_table();
void variant_1();
void variant_2();
void obj_code();

// Globals
char *filename;        // Assembly File
char *output_filename; // Generated Object File
char *extension;
struct instructions ins[64];       // Array of line wise instructions
int start_addr;                    // Program Start Address
int end_addr;                      // Program End Address
struct symbol_table *start = NULL; // Pointer to Starting Symbol in Symbol Table
struct obj_ins obj[64];

// Predefined Tables
const char *imperative_opcode[11] = {
    "STOP",
    "ADD",
    "SUB",
    "MULT",
    "MOVER",
    "MOVEM",
    "COMP",
    "BC",
    "DIV",
    "READ",
    "PRINT",
};

const char *declarative_opcode[2] = {
    "DC",
    "DS",
};

const char *assembler_directive[2] = {
    "START",
    "END",
};

const char *registers[4] = {
    "AREG",
    "BREG",
    "CREG",
    "DREG",
};

const char *condition_code[6] = {
    "LT",
    "LE",
    "EQ",
    "GT",
    "GE",
    "ANY",
};

const char *ad_assembler_directive[3] = {
    "ORIGIN",
    "EQU",
    "LTORG",
};

// Helper Functions

// checks for valid constants
// 100 or '100'
int is_number(char *str)
{
    int i;
    if (str[0] == '\'' && str[strlen(str) - 1] == '\'')
    {
        for (i = 1; i < strlen(str) - 1; i++)
        {
            if (!isdigit(str[i]))
                return 0;
        }
        return 1;
    }
    for (i = 0; i < strlen(str); i++)
        if (!isdigit(str[i]))
            return 0;
    return atoi(str);
}

// checks for valid symbols
int is_symbol(char *str)
{
    // added ad_assembly_directive late expect errors
    if (in_list(str, imperative_opcode, 11) || in_list(str, declarative_opcode, 2) || in_list(str, assembler_directive, 2) || in_list(str, condition_code, 6) || in_list(str, registers, 4) || in_list(str, ad_assembler_directive, 3))
        return 0;
    int i;
    for (i = 0; i < strlen(str); ++i)
    {
        if (!isupper(str[i]) || !isalpha(str[i]))
            return 0;
    }
    return 1;
}

// cuts and returns part of string
// absolutely no exception is handled
char *str_cut(char *str, int start, int num)
{
    char *temp = (char *)malloc(sizeof(char) * (num + 1));
    strncpy(temp, str + start, num);
    temp[num] = '\0';
    return temp;
}
// Checks if the filename has proper
// extension and assigns filename
// and extension globals
void check_extension(char *fn[], int count)
{
    if (count < 2 || count > 3)
    {
        printf("\nE : Unexpected Number Of Arguments\n");
        exit(0);
    }
    output_filename = (char *)malloc(sizeof(char) * 256);
    if (count == 3)
    {
        // TODO : check if entered filename has obj extension
        strcpy(output_filename, fn[2]);
        strcat(output_filename, ".obj");
    }
    else
    {
        strcpy(output_filename, "output.obj");
    }
    char *token;
    filename = (char *)malloc(sizeof(char) * 256);
    extension = (char *)malloc(sizeof(char) * 256);
    strcpy(filename, strtok(fn[1], "."));
    if ((token = strtok(NULL, "")) != NULL)
        strcpy(extension, token);
    if (strcmpi(extension, "asm"))
    {
        printf("\nE : asm File Expected\n");
        exit(0);
    }
}

// Checks if file exists and
// builds the instruction table
void open_file(char *fn)
{
    FILE *fp = fopen(strcat(fn, ".asm"), "r");
    if (fp == NULL)
    {
        printf("\nE : FILE %s.asm DOES NOT EXIST\n", fn);
        exit(0);
    }
    char *buff = malloc(sizeof(char) * 1024);
    char *res;
    //reading first line to set start_addr
    if ((res = fgets(buff, 1024, fp)) != NULL)
    {
        //removes trailing new line if exists
        if (res[strlen(res) - 1] == '\n')
            res[strlen(res) - 1] = '\0';
        char *tok = strtok(res, " ");
        //handles empty first line
        if (tok == NULL)
        {
            printf("\nE : Expected START on Line 1\n");
            fclose(fp);
            exit(0);
        }
        if (!strcmp(tok, "START"))
        {
            char *addr = strtok(NULL, " ");
            if (addr == NULL)
                start_addr = 100;
            else
            {
                //validates starting address
                if (is_number(addr))
                {
                    start_addr = atoi(addr);
                    if (start_addr < 100 || start_addr > 999)
                    {
                        printf("\nE : Starting Address Out Of Range(100-999)\n");
                        fclose(fp);
                        exit(0);
                    }
                }
                else
                {
                    printf("\nE : Starting Address Not Numeric\n");
                    fclose(fp);
                    exit(0);
                }
            }
        }
        else
        {
            printf("\nE : Expected START on Line 1\n");
            fclose(fp);
            exit(0);
        }
        //if more than 2 tokens are present
        if (strtok(NULL, " ") != NULL)
        {
            printf("\nE : Invalid START Statement\n");
            printf("Usage : START <constant>\n");
            fclose(fp);
            exit(0);
        }
    }
    else
    {
        printf("\nE : File Empty\n");
        fclose(fp);
        exit(0);
    }
    int i = 0;
    while ((res = fgets(buff, 1024, fp)) != NULL)
    {
        //removes trailing new line
        if (res[strlen(res) - 1] == '\n')
            res[strlen(res) - 1] = '\0';
        char *tok = strtok(res, " ");
        //handles empty line
        if (tok == NULL)
            continue;
        if (!strcmp(tok, "END"))
            break;
        int j = 0;
        while (tok != NULL)
        {
            if (j > 3)
            {
                printf("\nE : Unexpected Number of Tokens on Line %d\n", i + 1);
                fclose(fp);
                exit(0);
            }
            strcpy(ins[i].tokens[j++], tok);
            // TODO : Fix LOOP READ, A B
            // will tokenize on comma after 1st token
            tok = strtok(NULL, " ,");
        }
        ins[i].count = j;
        ++i;
    }
    if (res == NULL)
    {
        printf("\nE : Expected END at Last Line\n");
        fclose(fp);
        exit(0);
    }
    end_addr = start_addr + i - 1;
    if (end_addr < start_addr)
    {
        printf("\nE : No Statement to Process\n");
        fclose(fp);
        exit(0);
    }
    fclose(fp);
}

// Checks if passed string is in the list and
// and returns the index+1 if found
int in_list(char *str, const char *list[], int num)
{
    int i;
    for (i = 0; i < num; i++)
    {
        if (!strcmp(str, list[i]))
            return i + 1;
    }
    return 0;
}

// non constant version of in_list
// TODO : is this really required
int in_list_r(char *str, char *list[], int num)
{
    int i;
    for (i = 0; i < num; i++)
    {
        if (!strcmp(str, list[i]))
            return 1;
    }
    return 0;
}

// Adds the symbol to Symbol Table
// if no addr then addr = 0
void add_symbol(char *str, int addr, int def, int type)
{
    struct symbol_table *sym = (struct symbol_table *)malloc(sizeof(struct symbol_table));
    sym->symbol = (char *)malloc(sizeof(char) * 32);
    strcpy(sym->symbol, str);
    sym->value = 0;
    if (def == 0)
    {
        struct symbol_table *curr = start;
        while (curr != NULL)
        {
            if (!strcmp(curr->symbol, str) && curr->type == type)
            {
                curr->used = 1;
                return;
            }
            curr = curr->next;
        }
        sym->used = 1;
        sym->defined = 0;
    }
    else
    {
        struct symbol_table *curr = start;
        while (curr != NULL)
        {
            if (!strcmp(curr->symbol, str) && curr->type == type)
            {
                ++curr->defined;
                curr->addr = addr;
                return;
            }
            curr = curr->next;
        }
        sym->defined = 1;
        sym->used = 0;
    }
    sym->addr = addr;
    sym->type = type;
    sym->next = NULL;
    if (start == NULL)
    {
        start = sym;
    }
    else
    {
        struct symbol_table *cur = start;
        while (cur->next != NULL)
            cur = cur->next;
        cur->next = sym;
    }
}

// Reads the instructions table and builds
// the symbol table
void build_sym_table()
{
    int i;
    for (i = 0; i < end_addr - start_addr + 1; i++)
    {
        if (ins[i].count == 4)
        {
            if (!is_symbol(ins[i].tokens[0]))
            {
                printf("\nE : Invalid Symbolic Name %s on Line %d\n", ins[i].tokens[0], i + 2);
            }
            else
            {
                // label
                add_symbol(ins[i].tokens[0], start_addr + i, 1, 1);
            }
            if (!in_list(ins[i].tokens[1], imperative_opcode, 11))
            {
                printf("\nE : Invalid Mnemonic Instruction %s on Line %d\n", ins[i].tokens[1], i + 2);
            }
            if (!in_list(ins[i].tokens[2], registers, 4))
            {
                printf("\nE : Invalid Register Operand %s on Line %d\n", ins[i].tokens[2], i + 2);
            }
            if (!is_symbol(ins[i].tokens[3]))
            {
                printf("\nE : Invalid Symbolic Name %s on Line %d\n", ins[i].tokens[3], i + 2);
            }
            else
            {
                // memory operand
                add_symbol(ins[i].tokens[3], 0, 0, 0);
            }
        }
        if (ins[i].count == 3)
        {
            // handling BC seperately because <mnemonic opcode><register><memory>
            // expects register operand at 1
            if (!strcmp(ins[i].tokens[0], "BC"))
            {
                if (!in_list(ins[i].tokens[1], condition_code, 6))
                {
                    printf("\nE : Invalid Condition Code %s on Line %d\n", ins[i].tokens[1], i + 2);
                }
                if (!is_symbol(ins[i].tokens[2]))
                {
                    printf("\nE : Invalid Symbolic Name %s on Line %d\n", ins[i].tokens[2], i + 2);
                }
                else
                {
                    add_symbol(ins[i].tokens[2], 0, 0, 1);
                }
            }
            else if (in_list(ins[i].tokens[0], imperative_opcode, 11))
            {
                if (!in_list(ins[i].tokens[1], registers, 4))
                {
                    printf("\nE : Invalid Register Operand %s on Line %d\n", ins[i].tokens[1], i + 2);
                }
                if (!is_symbol(ins[i].tokens[2]))
                {
                    printf("\nE : Invalid Symbolic Name %s on Line %d\n", ins[i].tokens[2], i + 2);
                }
                else
                {
                    //memory operand
                    add_symbol(ins[i].tokens[2], 0, 0, 0);
                }
            }
            else
            {
                if (!is_symbol(ins[i].tokens[0]))
                {
                    printf("\nE : Invalid Symbolic Name %s on Line %d\n", ins[i].tokens[0], i + 2);
                }
                else
                {
                    // here
                    //label || memory operand
                    if (in_list(ins[i].tokens[1], declarative_opcode, 2)) {
                        if(strcmp(ins[i].tokens[1], "DS") == 0){
                            add_symbol(ins[i].tokens[0], start_addr + i + is_number(ins[i].tokens[2]), 1, 0);
                        }
                        else
                            add_symbol(ins[i].tokens[0], start_addr + i, 1, 0);
                    }
                    else
                        add_symbol(ins[i].tokens[0], start_addr + i, 1, 1);
                }
                if (in_list(ins[i].tokens[1], imperative_opcode, 11))
                {
                    if (!is_symbol(ins[i].tokens[2]))
                    {
                        printf("\nE : Invalid Symbolic Name %s on Line %d\n", ins[i].tokens[2], i + 2);
                    }
                    else
                    {
                        //memory operand
                        add_symbol(ins[i].tokens[2], 0, 0, 0);
                    }
                }
                else if (in_list(ins[i].tokens[1], declarative_opcode, 2))
                {
                    if (!is_number(ins[i].tokens[2]))
                    {
                        // nTODO: validation of symbol depends on valid constant
                        printf("\nE : Invalid Constant %s on Line %d\n", ins[i].tokens[2], i + 2);
                    }
                }
                // <label> EQU <address specifier> address specifier = constant or symbol
                // does this declare symbols?
                else if (!strcmp(ins[i].tokens[1], ad_assembler_directive[1]))
                {
                    if (!is_symbol(ins[i].tokens[2]))
                    {
                        if (!is_number(ins[i].tokens[2]))
                        {
                            printf("\nE : Invalid Symbolic Name/Constant %s on Line %d\n", ins[i].tokens[2], i + 2);
                        }
                    }
                    else
                    {
                        //memory operand
                        add_symbol(ins[i].tokens[2], 0, 0, 0);
                    }
                }
                else
                {
                    printf("\nE :  Invalid Mnemonic Instruction %s on Line %d\n", ins[i].tokens[1], i + 2);
                    if (!is_symbol(ins[i].tokens[2]))
                    {
                        if (!is_number(ins[i].tokens[2]))
                        {
                            printf("\nE : Invalid Symbolic Name/Constant %s on Line %d\n", ins[i].tokens[2], i + 2);
                        }
                    }
                    else
                    {
                        //memory operand
                        add_symbol(ins[i].tokens[2], 0, 0, 0);
                    }
                }
            }
        }
        // TODO : check all cases for 3 tokens
        if (ins[i].count == 2)
        {
            if (!is_symbol(ins[i].tokens[0]))
            {
                // <mnemonic instruction> <constant>|<memory operand>
                if (!in_list(ins[i].tokens[0], imperative_opcode, 11))
                {
                    // ORIGIN <address specifier> address specifier = constant or symbol
                    if (strcmp(ins[i].tokens[0], ad_assembler_directive[0]))
                    {
                        printf("\nE : Invalid Mnemonic Instruction %s on Line %d\n", ins[i].tokens[0], i + 2);
                    }
                }
                if (!is_symbol(ins[i].tokens[1]))
                {
                    if (!is_number(ins[i].tokens[1]))
                    {
                        printf("\nE : Invalid Symbolic Name/Constant %s on Line %d\n", ins[i].tokens[1], i + 2);
                    }
                }
                else
                {
                    //memory operand
                    add_symbol(ins[i].tokens[1], 0, 0, 0);
                }
            }
            else
            {
                //label
                add_symbol(ins[i].tokens[0], 0, 1, 0);
                if (!in_list(ins[i].tokens[1], imperative_opcode, 11))
                {
                    printf("\nE : Invalid Mnemonic Instruction %s on Line %d\n", ins[i].tokens[1], i + 2);
                }
            }
        }
        if (ins[i].count == 1)
        {
            if (strcmp(ins[i].tokens[0], imperative_opcode[0]) && strcmp(ins[i].tokens[0], ad_assembler_directive[2]))
            {
                printf("\nE : Expected STOP/LTORG\n");
            }
        }
    }
}

// Prints the Symbol Table
void print_sym_table()
{
    struct symbol_table *cur = start;
    printf("\nSymbol\tAddress\tUsed\tDefined\tType\n");
    while (cur != NULL)
    {
        printf("%s\t%d\t%d\t%d", cur->symbol, cur->addr, cur->used, cur->defined);
        if (cur->type)
            printf("\tLabel\n");
        else
            printf("\tMemory Operand\n");
        cur = cur->next;
    }
}

// Displays Error Related to Symbol Table
void error_sym_table()
{
    struct symbol_table *curr = start;
    char *redeclared[32];
    int c = 0;
    while (curr != NULL)
    {
        struct symbol_table *cur = start;
        int count = 0;
        while (cur != NULL)
        {
            // count the number of occurences of
            // particular symbol
            if (!strcmp(cur->symbol, curr->symbol))
                ++count;
            cur = cur->next;
        }
        // checks if a particular symbol occurs
        // more than once in symbol list
        if (count > 1)
        {
            // checks if symbol is already recognised as redeclared
            if (!in_list_r(curr->symbol, redeclared, c))
                redeclared[c++] = curr->symbol;
        }
        else
        {
            if (curr->used == 1 && curr->defined == 0)
                printf("\nE : Symbol %s Used but not Defined\n", curr->symbol);
            if (curr->used == 0 && curr->defined == 1)
                printf("\nW : Symbol %s Defined but not Used\n", curr->symbol);
            if (curr->defined > 1)
                printf("\nE : Re-declaration of Symbol %s\n", curr->symbol);
        }
        curr = curr->next;
    }
    // printing the redeclared array
    int i;
    for (i = 0; i < c; ++i)
    {
        printf("\nE : Re-declaration of Symbol %s\n", redeclared[i]);
    }
}

// Prints the Variant I of given program
void variant_1()
{
    printf("\nVARIANT I - ");
    int i = 0;
    printf("\n<AD,1> <C,%d>\n", start_addr);
    for (; i < end_addr - start_addr + 1; ++i)
    {
        int j = 0;
        for (; j < ins[i].count; ++j)
        {
            if (j == 0 && is_symbol(ins[i].tokens[j]))
                continue;
            int pos;
            if ((pos = in_list(ins[i].tokens[j], imperative_opcode, 11)))
            {
                printf("<IS,%d> ", pos - 1);
            }
            if ((pos = in_list(ins[i].tokens[j], ad_assembler_directive, 3)))
            {
                printf("<AD,%d> ", pos + 2);
            }
            if ((pos = in_list(ins[i].tokens[j], declarative_opcode, 2)))
            {
                printf("<DL,%d> ", pos);
            }
            if ((pos = in_list(ins[i].tokens[j], registers, 4)))
            {
                printf("%d ", pos);
            }
            if ((pos = in_list(ins[i].tokens[j], condition_code, 6)))
            {
                printf("%d ", pos);
            }
            if (is_number(ins[i].tokens[j]))
            {
                if (ins[i].tokens[j][0] == '\'' || ins[i].tokens[j][strlen(ins[i].tokens[j])] == '\'')
                {
                    printf("<C,");
                    int k = 1;
                    for (; k < strlen(ins[i].tokens[j]) - 1; k++)
                        printf("%c", ins[i].tokens[j][k]);
                    printf("> ");
                }
                else
                    printf("<C,%d> ", atoi(ins[i].tokens[j]));
            }
            if (is_symbol(ins[i].tokens[j]))
            {
                struct symbol_table *cur = start;
                int k = 0;
                while (cur != NULL)
                {
                    ++k;
                    if (!strcmp(cur->symbol, ins[i].tokens[j]))
                    {
                        printf("<S,%d> ", k);
                    }
                    cur = cur->next;
                }
            }
        }
        printf("\n");
    }
    printf("<AD,2>\n");
}

// Prints the Variant II of given program
void variant_2()
{
    printf("\nVARIANT II - ");
    int i = 0;
    printf("\n<AD,1> <C,%d>\n", start_addr);
    for (; i < end_addr - start_addr + 1; ++i)
    {
        int j = 0;
        for (; j < ins[i].count; ++j)
        {
            if (j == 0 && is_symbol(ins[i].tokens[j]))
                continue;
            int pos;
            if ((pos = in_list(ins[i].tokens[j], imperative_opcode, 11)))
            {
                printf("<IS,%d> ", pos - 1);
            }
            else if ((pos = in_list(ins[i].tokens[j], ad_assembler_directive, 3)))
            {
                printf("<AD,%d> ", pos + 2);
            }
            else if ((pos = in_list(ins[i].tokens[j], declarative_opcode, 2)))
            {
                printf("<DL,%d> ", pos);
            }
            else if (is_number(ins[i].tokens[j]))
            {
                if (ins[i].tokens[j][0] == '\'' || ins[i].tokens[j][strlen(ins[i].tokens[j])] == '\'')
                {
                    printf("<C,");
                    int k = 1;
                    for (; k < strlen(ins[i].tokens[j]) - 1; k++)
                        printf("%c", ins[i].tokens[j][k]);
                    printf("> ");
                }
                else
                    printf("<C,%d> ", atoi(ins[i].tokens[j]));
            }
            else
                printf("%s ", ins[i].tokens[j]);
        }
        printf("\n");
    }
    printf("<AD,2>\n");
}

// Generates and writes the object code to
// output file
void obj_code()
{
    // blank when DS is before STOP
    FILE *fp = fopen(output_filename, "w");
    int i = 0;
    for (i = 0; i < end_addr - start_addr + 1; ++i)
    {
        if (in_list(ins[i].tokens[1], declarative_opcode, 2))
            continue;
        fprintf(fp, "%d ", start_addr + i);
        if (!strcmp(ins[i].tokens[0], "STOP"))
        {
            fprintf(fp, "000000");
            break;
        }
        int j = 0;
        for (j = 0; j < ins[i].count; ++j)
        {
            // dont process LABEL
            if (j == 0 && is_symbol(ins[i].tokens[j]))
                continue;
            int pos;
            if ((pos = in_list(ins[i].tokens[j], imperative_opcode, 11)))
            {
                // dont add 0 to obj code if instruction number > 10
                if (pos < 11)
                    fprintf(fp, "0");
                if (!strcmp(ins[i].tokens[j], "READ") || !strcmp(ins[i].tokens[j], "PRINT"))
                {
                    fprintf(fp, "%d", pos - 1);
                    if (j == (ins[i].count - 2))
                        fprintf(fp, "0");
                }
                else
                    fprintf(fp, "%d", pos - 1);
            }
            if ((pos = in_list(ins[i].tokens[j], ad_assembler_directive, 3)))
            {
                fprintf(fp, "0%d", pos + 2);
            }
            if ((pos = in_list(ins[i].tokens[j], registers, 4)))
            {
                fprintf(fp, "%d", pos);
            }
            if ((pos = in_list(ins[i].tokens[j], condition_code, 6)))
            {
                fprintf(fp, "%d", pos);
            }
            if (is_symbol(ins[i].tokens[j]))
            {
                struct symbol_table *cur = start;
                int k = 0;
                while (cur != NULL)
                {
                    ++k;
                    if (!strcmp(cur->symbol, ins[i].tokens[j]))
                    {
                        fprintf(fp, "%d", cur->addr);
                    }
                    cur = cur->next;
                }
            }
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void load_obj_code()
{
    FILE *fp = fopen(output_filename, "r");
    if (fp == NULL)
    {
        printf("\nE : Object File %s not Found", output_filename);
        exit(0);
    }
    int i = 0;
    char *buff = malloc(sizeof(char) * 1024);
    char *res;
    res = fgets(buff, 1024, fp);
    if (res == NULL)
    {
        printf("\nE : %s File Empty\n", output_filename);
        exit(0);
    }
    while (res != NULL)
    {
        // removing new line from read buffer
        if (res[strlen(res) - 1] == '\n')
            res[strlen(res) - 1] = '\0';
        obj[i].addr = atoi(strtok(res, " "));
        char *inst = strtok(NULL, " ");
        if (!strcmp(inst, "000000"))
        {
            obj[i].addr = -1;
            break;
        }
        obj[i].opcode = atoi(str_cut(inst, 0, 2));
        obj[i].register_operand = atoi(str_cut(inst, 2, 1));
        obj[i++].memory_operand = atoi(str_cut(inst, 3, 3));
        res = fgets(buff, 32, fp);
    }
    printf("\nT : Loaded Sucessfully\n");
    fclose(fp);
}

void print_obj_code()
{
    printf("\n");
    int i = 0;
    if (obj[i].addr == -1)
    {
        printf("\nE : Object Code not Loaded Yet\n");
        return;
    }
    for (; obj[i].addr != -1; ++i)
    {
        printf("%d ", obj[i].addr);
        if (obj[i].opcode < 10)
            printf("0");
        printf("%d%d%d\n", obj[i].opcode, obj[i].register_operand, obj[i].memory_operand);
    }
}

void simulate()
{
    int i = 0;
    if (obj[i].addr == -1)
    {
        printf("\nE : Object Code not Loaded Yet\n");
        return;
    }
    int registers[4] = {0};
    int cond[5] = {0}; // {LT, LE , EQ, GT, GE}
    while (obj[i].addr != -1)
    {
        struct symbol_table *cur = start;
        while (cur->addr != obj[i].memory_operand)
        {
            if (cur->next == NULL)
                break;
            else
                cur = cur->next;
        }
        // READ
        if (obj[i].opcode == 9)
        {
            if (obj[i].register_operand != 0)
            {
                printf("\nE : Unexpected Register Operand at Line %d", i + 1);
                exit(0);
            }
            int num;
            scanf("%d", &num);
            fflush(stdin);
            cur->value = num;
        }
        // PRINT
        if (obj[i].opcode == 10)
        {
            if (obj[i].register_operand != 0)
            {
                printf("\nE : Unexpected Register Operand at Line %d", i + 1);
                exit(0);
            }
            printf("%d", cur->value);
        }
        // ADD
        if (obj[i].opcode == 1)
        {
            if (cur->value != 0)
            {
                registers[obj[i].register_operand - 1] += cur->value;
            }
        }
        // MOVEM
        if (obj[i].opcode == 5)
        {
            cur->value = registers[obj[i].register_operand - 1];
        }
        // MOVER
        if (obj[i].opcode == 4)
        {
            registers[obj[i].register_operand - 1] = cur->value;
        }
        // SUB
        if (obj[i].opcode == 2)
        {
            registers[obj[i].register_operand - 1] -= cur->value;
        }
        // MULT
        if (obj[i].opcode == 3)
        {
            registers[obj[i].register_operand - 1] *= cur->value;
        }
        // DIV
        if (obj[i].opcode == 8)
        {
            if (!cur->value)
            {
                printf("\nE : Division by Zero");
                exit(0);
            }
            registers[obj[i].register_operand - 1] /= cur->value;
        }
        // COMP
        if (obj[i].opcode == 6)
        {
            if (registers[obj[i].register_operand] == cur->value)
            {
                printf("hello");
                cond[0] = cond[3] = 0;
                cond[2] = cond[1] = cond[4] = 1;
            }
            if (registers[obj[i].register_operand] > cur->value)
            {
                cond[0] = cond[1] = cond[2] = 0;
                cond[3] = cond[4] = 1;
            }
            else
            {
                cond[2] = cond[3] = cond[4] = 0;
                cond[0] = cond[1] = 1;
            }
        }
        // BC
        if (obj[i].opcode == 7)
        {
            if (cond[obj[i].register_operand - 1] == 1)
            {
                i = cur->addr - obj[0].addr;
                printf("%d", i);
                continue;
            }
        }
        ++i;
    }
}

void simulation_menu()
{
    obj[0].addr = -1;
    while (1)
    {
        char ch[32];
        printf("\n\nSimulator Menu -\n");
        printf("a. Load");
        printf("\nb. Print");
        printf("\nc. Run");
        printf("\nd. Exit");
        printf("\nYour Choice : ");
        fgets(ch, 32, stdin);
        switch (ch[0])
        {
        case 'a':
            load_obj_code();
            break;
        case 'b':
            print_obj_code();
            break;
        case 'c':
            simulate();
            break;
        case 'd':
            return;
        default:
            printf("\nE : Invalid Option\n");
        }
    }
}

void main(int argc, char *argv[])
{
    check_extension(argv, argc);
    open_file(filename);
    build_sym_table(); //assignment 1
    print_sym_table(); //assignment 2
    error_sym_table(); //assignment 2
    // variant_1();       //assignment 3
    // variant_2();       //assignment 3
    // obj_code(); //assignment 4
    // simulation_menu();
}
