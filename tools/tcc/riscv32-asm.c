/*************************************************************/
/*
 *  RISCV32 assembler for TCC
 *  Modified from riscv64 version
 *
 */

#ifdef TARGET_DEFS_ONLY

#define CONFIG_TCC_ASM
#define NB_ASM_REGS 32

ST_FUNC void g(int c);
ST_FUNC void gen_le16(int c);
ST_FUNC void gen_le32(int c);

/*************************************************************/
#else
/*************************************************************/
#define USING_GLOBALS
#include "tcc.h"
#include "riscv_utils.h"

enum {
    OPT_REG,
    OPT_IM12S,
    OPT_IM21,
    OPT_IM32,
};
#define OP_REG    (1 << OPT_REG)
#define OP_IM32   (1 << OPT_IM32)
#define OP_IM21   (1 << OPT_IM21)
#define OP_IM12S  (1 << OPT_IM12S)

// I don't think we need these anymore, but I haven't deleted them yet
#define ENCODE_RS1(register_index) ((register_index) << 15)
#define ENCODE_RS2(register_index) ((register_index) << 20)
#define ENCODE_RD(register_index) ((register_index) << 7)


// --------------------------- boilerplate tcc requirements ----------------- //
ST_FUNC void g(int i)
{
    int ind1;
    if (nocode_wanted)
        return;
    ind1 = ind + 1;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    cur_text_section->data[ind] = i;
    ind = ind1;
}

ST_FUNC void gen_le16 (int i)
{
    int ind1;
    if (nocode_wanted)
        return;
    ind1 = ind + 4;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    cur_text_section->data[ind++] = i & 0xFF;
    cur_text_section->data[ind++] = (i >> 8) & 0xFF;
}

ST_FUNC void gen_le32 (int i)
{
    int ind1;
    if (nocode_wanted)
        return;
    ind1 = ind + 4;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    cur_text_section->data[ind++] = i & 0xFF;
    cur_text_section->data[ind++] = (i >> 8) & 0xFF;
    cur_text_section->data[ind++] = (i >> 16) & 0xFF;
    cur_text_section->data[ind++] = (i >> 24) & 0xFF;
}
ST_FUNC void gen_expr32(ExprValue *pe)
{
    gen_le32(pe->v);
}

// ---------------------- end boilerplate tcc requirements ----------------- //


typedef struct Operand {
    uint32_t type;
    union {
        uint8_t reg;
        uint16_t regset;
        ExprValue e;
    };
} Operand;

/* Parse a text containing operand and store the result in OP */
static void parse_operand(TCCState *s1, Operand *op)
{
    ExprValue e;
    int8_t reg;

    op->type = 0;

    if ((reg = asm_parse_regvar(tok)) != -1) {
        next(); // skip register name
        op->type = OP_REG;
        op->reg = (uint8_t) reg;
        return;
    } else if (tok == '$') {
        /* constant value */
        next(); // skip '#' or '$'
    }
    asm_expr(s1, &e);
    op->type = OP_IM32;
    op->e = e;
    if (!op->e.sym) {
        // check if we are in a 12b signed range
        if ((int) op->e.v >= -2048 && (int) op->e.v < 2048) {
            op->type = OP_IM12S;
        }
        // check if we are within a 21b range
        if ((unsigned int) op->e.v <= 0x1fffff) {
            op->type = OP_IM21;
        }
    }
    else {
        char type[8];
        int value;
        switch(op->type) {
            case OP_REG:
                strcpy(type, "REG");
                value = op->reg;
                break;
            case OP_IM12S:
                strcpy(type, "IM12S");
                value = op->e.v;
                break;
            case OP_IM21:
                strcpy(type, "IM21");
                value = op->e.v;
                break;
            case OP_IM32:
                strcpy(type, "IM32");
                value = op->e.v;
                break;
        }
        tcc_error_noabort("Operand type: %s: %d", type, value);
        tcc_error_noabort("%s", get_tok_str(tok, NULL));
        expect("operand");
    }
}

// return false (0) if next operand not a register and true (1) if it is
static int check_register(const Operand *op)
{
    if (op->type != OP_REG) {
        tcc_error_noabort("expected a register");
        return 0;
    }
    return 1;
}


// return false (0) if next operand not an immediate and true (1) if it is
static int check_immediate(const Operand *op)
{
    if (op->type != OP_IM12S && op->type != OP_IM32 && op->type != OP_IM21) { 
        tcc_error_noabort("expected an immediate");
        return 0;
    }
    return 1;
}

static void asm_emit_opcode(uint32_t opcode) {
    int ind1 = ind + 4;
    if (nocode_wanted) {
        return;
    }
    if (ind1 > cur_text_section->data_allocated) {
        section_realloc(cur_text_section, ind1);
    }
    write32le(cur_text_section->data + ind, opcode);
    ind = ind1;
}

// implement helper functions from riscv_utils.h
// Register instructions (math and stuff)
void emit_R(uint32_t funct7, uint32_t rs2, uint32_t rs1,
            uint32_t funct3, uint32_t rd, uint32_t opcode)
{
    const uint32_t instruction = 
        (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | 
        (funct3 << 12) | (rd << 7) | opcode;

    asm_emit_opcode(instruction);
}

// immediate instructions
void emit_I(uint32_t imm, uint32_t rs1,
            uint32_t funct3, uint32_t rd, uint32_t opcode)
{
    const uint32_t instruction = 
        (imm << 20) | (rs1 << 15) | 
        (funct3 << 12) | (rd << 7) | opcode;

    asm_emit_opcode(instruction);
}

// store instructions
void emit_S(uint32_t imm, uint32_t rs2, uint32_t rs1,
            uint32_t funct3, uint32_t opcode)
{
    // we have to break up the immediate value into two sections (11:5, 4:0)
    const uint32_t imm_h = 0x7f & (imm >> 5);
    const uint32_t imm_l = 0x1f & imm;
    const uint32_t instruction = 
        (imm_h << 25) | (rs2 << 20) | (rs1 << 15) | 
        (funct3 << 12) | (imm_l << 7) | opcode;

    asm_emit_opcode(instruction);
}

// branch instructions
void emit_B(uint32_t imm, uint32_t rs2, uint32_t rs1,
            uint32_t funct3, uint32_t opcode)
{
    // we have to break up the immediate value into two sections (12,10:5, 4:1,11)
    const uint32_t imm_h = 0x7f & ((0x800 & imm >> 1) | imm ) >> 5 ;
    const uint32_t imm_l = (0x1e & imm) | (0x1 & (imm >> 11));
    const uint32_t instruction = 
        (imm_h << 25) | (rs2 << 20) | (rs1 << 15) | 
        (funct3 << 12) | (imm_l << 7) | opcode;

    asm_emit_opcode(instruction);
}

// big immediate instructions (LUI and AUIPC)
void emit_U(uint32_t imm, uint32_t rd, uint32_t opcode)
{
    // only want bits 31:12 of immediate
    const uint32_t instruction = (0xfffff000 & imm) | (rd << 7) | opcode;
    asm_emit_opcode(instruction);
}

// jump instructions
void emit_J(uint32_t imm, uint32_t rd, uint32_t opcode)
{
    // immediate value 20|10:1|11|19:12
    // mask off bits 19:12 then shift them back
    // mask off bit 11 then shift it over to bit 20
    // mask off bits 10:1, then shift over 21
    // mask off bit 21, then shift over 31
    const uint32_t immediate =  
              (((imm >> 12) &  0xff) << 12)
            | (((imm >> 11) &     1) << 20)
            | (((imm >>  1) & 0x3ff) << 21)
            | (((imm >> 20) &     1) << 31);

    const uint32_t instruction = immediate | (rd << 7) | opcode;
    asm_emit_opcode(instruction);
}

// ------------------------- end basic helper functions --------------------- //

static void asm_nullary_opcode(TCCState *s1, int token)
{
    switch (token) {
    case TOK_ASM_wfi:
        // I don't really know what WFI is
        asm_emit_opcode((0x1C << 2) | 3 | (0x105 << 20));
        return;

    default:
        expect("nullary instruction");
    }
}

// Note: Those all map to CSR--so they are pseudo-instructions.
static void asm_unary_opcode(TCCState *s1, int token)
{
    Operand op;
    parse_operand(s1, &op);
    if (token != TOK_ASM_j && op.type != OP_REG) {
        expect("register");
        return;
    }
    else if(token == TOK_ASM_j && op.type == OP_REG) {
        expect("immediate offset");
        return;
    }

    int rd = op.reg;

    switch (token) {
    case TOK_ASM_j:
        emit_J_inst(op.e.v); 
        return;
    case TOK_ASM_rdcycle:
        emit_RDCYCLE(rd);
        return;
    case TOK_ASM_rdcycleh:
        emit_RDCYCLEH(rd);
        return;
    case TOK_ASM_rdtime:
        emit_RDTIME(rd);
        return;
    case TOK_ASM_rdtimeh:
        emit_RDTIMEH(rd);
        return;
    case TOK_ASM_rdinstret:
        emit_RDINSTRET(rd);
        return;
    case TOK_ASM_rdinstreth:
        emit_RDINSTRETH(rd);
        return;
    default:
        expect("unary instruction");
    }
}

static void asm_emit_u(int token, uint32_t opcode, const Operand* rd, const Operand* rs2)
{
    if (rd->type != OP_REG) {
        tcc_error("'%s': Expected destination operand that is a register", get_tok_str(token, NULL));
        return;
    }
    if (rs2->type != OP_IM12S && rs2->type != OP_IM32) {
        tcc_error("'%s': Expected second source operand that is an immediate value", get_tok_str(token, NULL));
        return;
    } else if (rs2->e.v >= 0x100000) {
        tcc_error("'%s': Expected second source operand that is an immediate value between 0 and 0xfffff", get_tok_str(token, NULL));
        return;
    }
    /* U-type instruction:
	      31...12 imm[31:12]
	      11...7 rd
	      6...0 opcode */
    gen_le32(opcode | ENCODE_RD(rd->reg) | (rs2->e.v << 12));
}

static void asm_binary_opcode(TCCState* s1, int token)
{
    Operand ops[2];
    parse_operand(s1, &ops[0]);
    printf("hi 1\n");
    if (tok == ',') {
        next();
    }
    else {
        expect("','");
    }
    printf("hi 2\n");
    parse_operand(s1, &ops[1]);

    switch (token) {
    case TOK_ASM_la:
        emit_LA(ops[0].reg, ops[1].e.v);
        return;
    case TOK_ASM_li:
        emit_LI(ops[0].reg, ops[1].e.v);
        return;
    case TOK_ASM_lui:
        asm_emit_u(token, (0xD << 2) | 3, &ops[0], &ops[1]);
        return;
    case TOK_ASM_auipc:
        asm_emit_u(token, (0x05 << 2) | 3, &ops[0], &ops[1]);
        return;
    default:
        expect("binary instruction");
    }
}

static void asm_branch_opcode(TCCState* s1, int token)
{
    // Branch (RS1,RS2,IMM); SB-format
    uint32_t opcode = (0x18 << 2) | 3;
    uint32_t offset = 0;
    Operand ops[3];
    parse_operand(s1, &ops[0]);
    if (ops[0].type != OP_REG) {
        expect("register");
        return;
    }
    if (tok == ',')
        next();
    else
        expect("','");
    parse_operand(s1, &ops[1]);
    if (ops[1].type != OP_REG) {
        expect("register");
        return;
    }
    if (tok == ',')
        next();
    else
        expect("','");
    parse_operand(s1, &ops[2]);

    if (ops[2].type != OP_IM12S) {
        tcc_error("'%s': Expected third operand that is an immediate value between 0 and 0xfff", get_tok_str(token, NULL));
        return;
    }
    offset = ops[2].e.v;
    if (offset & 1) {
        tcc_error("'%s': Expected third operand that is an even immediate value", get_tok_str(token, NULL));
        return;
    }

    switch (token) {
    case TOK_ASM_beq:
        opcode |= 0 << 12;
        break;
    case TOK_ASM_bne:
        opcode |= 1 << 12;
        break;
    case TOK_ASM_blt:
        opcode |= 4 << 12;
        break;
    case TOK_ASM_bge:
        opcode |= 5 << 12;
        break;
    case TOK_ASM_bltu:
        opcode |= 6 << 12;
        break;
    case TOK_ASM_bgeu:
        opcode |= 7 << 12;
        break;
    default:
        expect("known branch instruction");
    }
    asm_emit_opcode(opcode | ENCODE_RS1(ops[0].reg) | ENCODE_RS2(ops[1].reg) | (((offset >> 1) & 0xF) << 8) | (((offset >> 5) & 0x1f) << 25) | (((offset >> 11) & 1) << 7) | (((offset >> 12) & 1) << 31));
}

// handle the register opcodes that don't have an immediate value (except for shifts)
static void asm_binary_register_opcode(TCCState *s1, int token)
{
    // start by getting the next three operands and check that they are all registers
    Operand rd, r1, r2;
    parse_operand(s1, &rd);
    if(!check_register(&rd)){
        tcc_error("'%s': Expected destination to be a register", get_tok_str(token, NULL));
    }

    if (tok == ','){ next(); }
    else { expect("','"); }
    parse_operand(s1, &r1);
    if(!check_register(&r1)){
        tcc_error("'%s': Expected first source operand to be a register", get_tok_str(token, NULL));
    }

    if (tok == ','){ next(); }
    else { expect("','"); }
    parse_operand(s1, &r2);
    // check that we are using a register, or that we have an immediate between 0 and 31 (for shifts)
    if(r2.type != OP_REG && r2.e.v <= 31){
        tcc_error(
            "'%s': Expected second source operand to be a register or immediate between 0 and 31",
             get_tok_str(token, NULL)
        );
        return;
    }

    // switch through the various register instructions
    switch(token) {
        case TOK_ASM_slli:
            emit_SLLI(rd.reg, r1.reg, r2.e.v); return;
        case TOK_ASM_srli:
            emit_SRLI(rd.reg, r1.reg, r2.e.v); return;
        case TOK_ASM_srai:
            emit_SRAI(rd.reg, r1.reg, r2.e.v); return;
        case TOK_ASM_add:
            emit_ADD(rd.reg, r1.reg, r2.reg); return;
        case TOK_ASM_sub:
            emit_SUB(rd.reg, r1.reg, r2.reg); return;
        case TOK_ASM_sll:
            emit_SLL(rd.reg, r1.reg, r2.reg); return;
        case TOK_ASM_slt:
            emit_SLT(rd.reg, r1.reg, r2.reg); return;
        case TOK_ASM_sltu:
            emit_SLTU(rd.reg, r1.reg, r2.reg); return;
        case TOK_ASM_xor:
            emit_XOR(rd.reg, r1.reg, r2.reg); return;
        case TOK_ASM_srl:
            emit_SRL(rd.reg, r1.reg, r2.reg); return;
        case TOK_ASM_sra:
            emit_SRA(rd.reg, r1.reg, r2.reg); return; 
        case TOK_ASM_or:
            emit_OR(rd.reg, r1.reg, r2.reg); return; 
        case TOK_ASM_and:
            emit_AND(rd.reg, r1.reg, r2.reg); return;
        case TOK_ASM_mul:
            emit_MUL(rd.reg, r1.reg, r2.reg); return; 
        case TOK_ASM_mulh:
            emit_MULH(rd.reg, r1.reg, r2.reg); return; 
        case TOK_ASM_mulhsu:
            emit_MULHSU(rd.reg, r1.reg, r2.reg); return;
        case TOK_ASM_div:
            emit_DIV(rd.reg, r1.reg, r2.reg); return; 
        case TOK_ASM_divu:
            emit_DIVU(rd.reg, r1.reg, r2.reg); return; 
        case TOK_ASM_rem:
            emit_REM(rd.reg, r1.reg, r2.reg); return; 
        case TOK_ASM_remu:
            emit_REMU(rd.reg, r1.reg, r2.reg); return; 
        default:
            expect("register instruction");
    }

}
  
static void asm_immediate_opcode(TCCState *s1, int token)
{
    // start by getting the next three operands and check that they are all registers
    Operand regs[2], imm;
    parse_operand(s1, &regs[0]);
    if(!check_register(&regs[0])){
        if(token != TOK_ASM_sb || token != TOK_ASM_sh || token != TOK_ASM_sw) {
            tcc_error(
                "'%s': Expected destination to be a register", 
                get_tok_str(token, NULL)
            );
        }
        else {
            tcc_error(
                "'%s': Expected first source operand to be a register", 
                get_tok_str(token, NULL)
            );
        }
        return;
    }

    if (tok == ','){ next(); }
    else { expect("','"); }
    parse_operand(s1, &regs[1]);
    if(!check_register(&regs[1])){
        if(token != TOK_ASM_sb || token != TOK_ASM_sh || token != TOK_ASM_sw) {
            tcc_error(
                "'%s': Expected first source operand to be a register", 
                get_tok_str(token, NULL)
            );
        }
        else {
            tcc_error(
                "'%s': Expected second source operand to be a register", 
                get_tok_str(token, NULL)
            );
        }
        return;
    }

    if (tok == ','){ next(); }
    else { expect("','"); }
    parse_operand(s1, &imm);
    // check that we have an 12bit immediate 
    if(imm.type != OP_IM12S){
        tcc_error(
            "'%s': Expected third operand that is an immediate value between 0 and 0xfff",
            get_tok_str(token, NULL)
        );
        return;
    }

    // switch through the various register instructions
    switch(token) {
        case TOK_ASM_lb:
            emit_LB(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_lh:
            emit_LH(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_lw:
            emit_LW(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_lbu:
            emit_LBU(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_lhu:
            emit_LHU(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_sb:
            emit_SB(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_sh:
            emit_SH(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_sw:
            emit_SW(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_addi:
            emit_ADDI(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_slti:
            emit_SLTI(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_sltiu:
            emit_SLTIU(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_xori:
            emit_XORI(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_ori:
            emit_ORI(regs[0].reg, regs[1].reg, imm.e.v); return;
        case TOK_ASM_andi:
            emit_ANDI(regs[0].reg, regs[1].reg, imm.e.v); return;
        default:
            expect("immediate register instruction");
    }
}

// support the Zicsr extension Takes in two arguments
// for CSRR a dest register and an immediate value
// or an immediate value and a source register
// or two immediate values
static void asm_control_status_pseudo_opcode(TCCState *s1, int token)
{
    //set to values outside of the valid range
    int rd = 64;
    int rs = 64;
    uint32_t csr = 0xffffffff;
    uint32_t imm = 0xffffffff;
    Operand temp;

    // start by getting the next two operands
    parse_operand(s1, &temp);

    // if getting the csrr instruction we want a register
    if(token == TOK_ASM_csrr){
        if(!check_register(&temp)) {
            tcc_error( "'%s': Expected destination to be a register", get_tok_str(token, NULL));
            return;
        }
        rd = temp.reg;
    }
    // otherwise we want an immediate between 0x and 0xfffff
    else {
        if (!check_immediate(&temp)){
            tcc_error(
                "'%s': Expected csr to be an immediate between 0 and 0xfffff", 
                get_tok_str(token, NULL)
            );
            return;
        }
        else if(!(temp.e.v >= 0 && temp.e.v <= 0xfffff)){
            tcc_error(
                "'%s': Expected csr to be an immediate between 0 and 0xfffff", 
                get_tok_str(token, NULL)
            );
            return;
        }
        csr = temp.e.v;
    }

    if (tok == ','){ next(); }
    else { expect("','"); }
    parse_operand(s1, &temp);
    
    // for csrr, csrwi, csrsi, csrci we can get an immediate
    if(token == TOK_ASM_csrr || token == TOK_ASM_csrwi || token == TOK_ASM_csrci){
        int is_immediate = check_immediate(&temp);
        // csrr needs an immediate between 0 and 0xfffff
        if(token == TOK_ASM_csrr) {
            if( !is_immediate || ( is_immediate && (temp.e.v >=0 && temp.e.v <= 0xfffff))) {
                tcc_error(
                    "'%s': Expected csr to be an immediate between 0 and 0xfffff", 
                    get_tok_str(token, NULL)
                );
                return;
            }
            csr = temp.e.v;
        }
        else {
            // check for an immediate between 0 and 31
            if (is_immediate && (temp.e.v >= 0 && temp.e.v <= 31)) {
                imm = temp.e.v;
            }
            else if (!is_immediate) {
                imm = temp.reg;
            }
            // otherwise we should send an error
            else {
                tcc_error(
                    "'%s': Expected second argument to be an immediate between 0 and 0x1f",
                    get_tok_str(token, NULL)
                );
                return;
            }
        }
    }
    // other instruction
    else {
        if(!check_register(&temp)) {
            tcc_error(
                "'%s': Expected second argument to be a register",
                get_tok_str(token, NULL)
            );
            return;
        }
        rs = temp.reg;
    }

    switch(token) {
        case TOK_ASM_csrr:
            emit_CSRR(rd, csr); return;
        case TOK_ASM_csrw:
            emit_CSRW(csr, rs); return;
        case TOK_ASM_csrs:
            emit_CSRS(csr, rs); return;
        case TOK_ASM_csrc:
            emit_CSRC(csr, rs); return;
        case TOK_ASM_csrwi:
            emit_CSRWI(csr, imm); return;
        case TOK_ASM_csrsi:
            emit_CSRSI(csr, imm); return;
        case TOK_ASM_csrci:
            emit_CSRCI(csr, imm); return;
        default:
            expect("control and status instruction"); return;
    }
}
// support the Zicsr extension Takes in three arguments a dest register, immediate value, and a
// source register or a source immediate
static void asm_control_status_opcode(TCCState *s1, int token)
{
    // start by getting the next three operands and check that they are all registers
    Operand rd, csr;
    Operand temp, rs1, uimm;
    parse_operand(s1, &rd);
    if(!check_register(&rd)){
        tcc_error( "'%s': Expected destination to be a register", get_tok_str(token, NULL));
        return;
    }

    if (tok == ','){ next(); }
    else { expect("','"); }
    parse_operand(s1, &csr);
    
    if(!check_immediate(&csr)){
        tcc_error(
            "'%s': Expected csr to be an immediate between 0 and 0xfffff", 
            get_tok_str(token, NULL)
        );
        return;
    }
    // range check the second immediate
    else if(!(csr.e.v >= 0 && csr.e.v <= 0xfffff)){
        tcc_error(
            "'%s': Expected csr to be an immediate between 0 and 0xfffff", 
            get_tok_str(token, NULL)
        );
        return;
    }

    if (tok == ','){ next(); }
    else { expect("','"); }
    parse_operand(s1, &uimm);

    if (tok == ','){ next(); }
    else { expect("','"); }
    parse_operand(s1, &temp);
    // check that we are using a register, or that we have an immediate between 0 and 31 (for shifts)
    if(temp.type != OP_REG && temp.e.v <= 31){
        tcc_error(
            "'%s': Expected second source operand to be a register or immediate between 0 and 31",
             get_tok_str(token, NULL)
        );
        return;
    }
    rs1 = uimm = temp;

    switch(token) {
        case TOK_ASM_csrrw:
            emit_CSRRW(rd.reg, csr.e.v, rs1.reg); return;
        case TOK_ASM_csrrs:
            emit_CSRRS(rd.reg, csr.e.v, rs1.reg); return;
        case TOK_ASM_csrrc:
            emit_CSRRC(rd.reg, csr.e.v, rs1.reg); return;
        case TOK_ASM_csrrwi:
            emit_CSRRC(rd.reg, csr.e.v, rs1.e.v); return;
        case TOK_ASM_csrrsi:
            emit_CSRRC(rd.reg, csr.e.v, uimm.e.v); return;
        case TOK_ASM_csrrci:
            emit_CSRRC(rd.reg, csr.e.v, uimm.e.v); return;
        default:
            expect("control and status instruction"); return;
    }
}

// Generate code for j, jal, jalr, and jr. Can take 1, 2, or 3 arguments (from TCCState)
static void asm_jump_opcode(TCCState *s1, int token)
{
    // j takes 1 immediate argument 
    // jal can take either a register and an immediate or just an immediate
    // jalr can take either a register, or two registers and an immediate
    // jr takes a register

    Operand ops[3];

    // we always need to get the first argument
    parse_operand(s1, &ops[0]);
    
    // if we have an immediate then check if we match one of our instruction types 
    // (j offset, jal offset)
    if(ops[0].type != OP_REG) {
        switch(token) {
            case TOK_ASM_j:
                emit_J_inst(ops[0].e.v);
                break;
            case TOK_ASM_jal:
                emit_JAL_x1(ops[0].e.v);
                break;
            // other than these two instructions, we expect to have a register, so issue an error
            default:
                expect("register");
                return;
        }

        // a bit late, but check if the data type fits our expectations on its size
        // we only get here if the instructions are of type j, or jal
        if(ops[0].type != OP_IM21) {
            tcc_error(
                "'%s': Expected operand that is an immediate value between 0 and 0x1fffff", 
                get_tok_str(token, NULL)
            );
        }
        return;
    }

    // first argument is a register
    // test if we are in the jr case (jr register)
    if ( token == TOK_ASM_jr ) {
        emit_JR(ops[0].reg);    
        return;
    }

    // done with one argument instructions, get the next argument
    if (tok == ','){ next(); }
    else { expect("','"); }
    parse_operand(s1, &ops[1]);
    
    // check if we have an immediate and we are in the jal instruction
    if(ops[1].type != OP_REG ) {
        // Other instructions need a register so issue an error
        if (token != TOK_ASM_jal){ 
            expect("register");
            return;
        }
        // check if the data type fits our expectations on its size
        if(ops[1].type != OP_IM21) {
            tcc_error(
                "'%s': Expected second operand that is an immediate value between 0 and 0xfffff", 
                get_tok_str(token, NULL)
            );
            return;
        }
        emit_JAL(ops[0].reg, ops[1].e.v);
        return;
    }

    // first two arguments are registers
    // done with two argument instructions, get the next argument
    if (tok == ','){ next(); }
    else { expect("','"); }
    parse_operand(s1, &ops[2]);

    // The last argument should be a 12b immediate
    if(ops[2].type != OP_IM12S) {
        tcc_error(
            "'%s': Expected third operand that is an immediate value between 0 and 0xfffff", 
            get_tok_str(token, NULL)
        );
        return;
    }
    // the only instruction left is jalr rd, r1, offset
    if (token == TOK_ASM_jalr) {
        emit_JALR(ops[0].reg, ops[1].reg, ops[2].e.v);
        return;
    }

    // we didn't get the instruction we expected, send an error
    expect("jump or link instruction");
    return;
}

ST_FUNC void asm_opcode(TCCState *s1, int token)
{
    switch (token) {
    case TOK_ASM_fence:
        emit_FENCE_DEFAULT(); return;
    case TOK_ASM_fence_i:
        tcc_error("zfenci instruction not implemented"); return;

    case TOK_ASM_scall:
    case TOK_ASM_ecall:
        emit_ECALL(); return;

    case TOK_ASM_ebreak:
    case TOK_ASM_sbreak:
        emit_EBREAK(); return;
    
    case TOK_ASM_nop:
        emit_NOP(); return;

    case TOK_ASM_mrts:
    case TOK_ASM_mrth:
    case TOK_ASM_hrts:
    case TOK_ASM_wfi:
        asm_nullary_opcode(s1, token);
        return;

    case TOK_ASM_rdcycle:
    case TOK_ASM_rdcycleh:
    case TOK_ASM_rdtime:
    case TOK_ASM_rdtimeh:
    case TOK_ASM_rdinstret:
    case TOK_ASM_rdinstreth:
        asm_unary_opcode(s1, token);
        return;

    case TOK_ASM_la:
    case TOK_ASM_li:
    case TOK_ASM_lui:
    case TOK_ASM_auipc:
        asm_binary_opcode(s1, token);
        return;

    case TOK_ASM_lb:
    case TOK_ASM_lh:
    case TOK_ASM_lw:
    case TOK_ASM_lbu:
    case TOK_ASM_lhu:
    case TOK_ASM_sb:
    case TOK_ASM_sh:
    case TOK_ASM_sw:
    case TOK_ASM_addi:
    case TOK_ASM_slti:
    case TOK_ASM_sltiu:
    case TOK_ASM_xori:
    case TOK_ASM_ori:
    case TOK_ASM_andi:
        asm_immediate_opcode(s1, token);
        return;

    case TOK_ASM_slli:
    case TOK_ASM_srli:
    case TOK_ASM_srai:
    case TOK_ASM_add:
    case TOK_ASM_sub:
    case TOK_ASM_sll:
    case TOK_ASM_slt:
    case TOK_ASM_sltu:
    case TOK_ASM_xor:
    case TOK_ASM_srl:
    case TOK_ASM_sra:
    case TOK_ASM_or:
    case TOK_ASM_and:
    case TOK_ASM_mul:
    case TOK_ASM_mulh:
    case TOK_ASM_mulhsu:
    case TOK_ASM_div:
    case TOK_ASM_divu:
    case TOK_ASM_rem:
    case TOK_ASM_remu:
        asm_binary_register_opcode(s1, token);
        return;

    case TOK_ASM_beq:
    case TOK_ASM_bne:
    case TOK_ASM_blt:
    case TOK_ASM_bge:
    case TOK_ASM_bltu:
    case TOK_ASM_bgeu:
        asm_branch_opcode(s1, token);
        return;

    case TOK_ASM_j:
    case TOK_ASM_jal:
    case TOK_ASM_jalr:
    case TOK_ASM_jr:
        asm_jump_opcode(s1, token);
        return;

    // control and status extension
    case TOK_ASM_csrrw:
    case TOK_ASM_csrrs:
    case TOK_ASM_csrrc:
    case TOK_ASM_csrrwi:
    case TOK_ASM_csrrsi:
    case TOK_ASM_csrrci:
        asm_control_status_opcode(s1, token);
        return;

    // control and status pseudoinstructions
    case TOK_ASM_csrr:
    case TOK_ASM_csrw:
    case TOK_ASM_csrs:
    case TOK_ASM_csrc:
    case TOK_ASM_csrwi:
    case TOK_ASM_csrsi:
    case TOK_ASM_csrci:
        asm_control_status_pseudo_opcode(s1, token);


    default:
        expect("known instruction");
    }
}

ST_FUNC void subst_asm_operand(CString *add_str, SValue *sv, int modifier)
{
    tcc_error("RISCV64 asm not implemented.");
}

/* generate prolog and epilog code for asm statement */
ST_FUNC void asm_gen_code(ASMOperand *operands, int nb_operands,
                         int nb_outputs, int is_output,
                         uint8_t *clobber_regs,
                         int out_reg)
{
}

ST_FUNC void asm_compute_constraints(ASMOperand *operands,
                                    int nb_operands, int nb_outputs,
                                    const uint8_t *clobber_regs,
                                    int *pout_reg)
{
}

ST_FUNC void asm_clobber(uint8_t *clobber_regs, const char *str)
{
    int reg;
    TokenSym *ts;

    if (!strcmp(str, "memory") ||
        !strcmp(str, "cc") ||
        !strcmp(str, "flags"))
        return;
    ts = tok_alloc(str, strlen(str));
    reg = asm_parse_regvar(ts->tok);
    if (reg == -1) {
        tcc_error("invalid clobber register '%s'", str);
    }
    clobber_regs[reg] = 1;
}

ST_FUNC int asm_parse_regvar (int t)
{
    if (t >= TOK_ASM_zero && t <= TOK_ASM_pc) { /* register name */
        switch (t) {
            case TOK_ASM_pc:
                return -1; // TODO: Figure out where it can be used after all
            default:
                return t - TOK_ASM_zero;
        }
    } else
        return -1;
}

/*************************************************************/
#endif /* ndef TARGET_DEFS_ONLY */
