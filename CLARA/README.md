# CLARA: CL Advanced Runtime Assembler
###### *Coming soon: CLARE, CLOE, CLARISSA, COLETTE, CLEO? IDK...*

A compiler for a WIP RISC assembler language.

## RISCM Instruction Set

Changes to be made.

### Operand Type Reference

**i8** - immediate 8 bit (n)  
**i16** - immediate 16 bit (n)  
**i32** - immediate 32 bit (n)  
**lv8** - 8 bit local variable index (locals[n])  
**lv16** - 16 bit local variable index (locals[n])  
**lv32** - 32 bit local variable index (locals[n])    
**v8** - 8 bit global variable index (globals[n])  
**v16** - 16 bit global variable index (globals[n])  
**v32** - 32 bit global variable index (globals[n])  
**s32** - 32 bit string index (strings[n])  
**r32** - script-relative 32 bit offset  
**ptr32** - 32 bit pointer  

### Misc Operations

| Opcode | Mnemonic | Size       | Op1     | Op2   | Op3   | Op... | Description |
---------|----------|------------|---------|-------|-------|-------|--------------
| -      | nop      | 01         |         |       |       |       | No operation
| -      | break    | 01         |         |       |       |       | Breakpoint
| -      | throw    | 02         | i8      |       |       |       | Throws an exception, popping Op1(n) items off the stack to the exception data

### Stack Manipulation
| Opcode | Mnemonic | Size       | Op1     | Op2   | Op3   | Op... | Description |
---------|----------|------------|---------|-------|-------|-------|-------------
| -      | pushn    | 01         |         |       |       |       | Push 0-value onto the stack
| -      | pushb    | 02         | i8      |       |       |       | Push integer (Op1) onto the stack
| -      | pushw    | 03         | i16     |       |       |       | Push integer (Op1) onto the stack
| -      | pushd    | 05         | i32     |       |       |       | Push integer (Op1) onto the stack
| -      | pushq    | 09         | i64     |       |       |       | Push integer (Op1) onto the stack
| -      | pushf    | 05         | i32     |       |       |       | Push float (Op1) onto the stack (essentially `exf, pushd`)
| -      | pushqf   | 09         | i64     |       |       |       | Push float (Op1) onto the stack (essentially `exf, pushq`)
| -      | pushss   | 05         | s32     |       |       |       | Push pointer from the strings segment at offset Op1
| -      | pushds   | 05         | s32     |       |       |       | Push pointer from the data segment at offset Op1
| -      | pushab   | 01         |         |       |       |       | Pops 1 pointer off the stack and pushes the byte pointed by it onto the stack
| -      | pushaw   | 01         |         |       |       |       | Pops 1 pointer off the stack and pushes the word pointed by it onto the stack
| -      | pushad   | 01         |         |       |       |       | Pops 1 pointer off the stack and pushes the double word pointed by it onto the stack
| -      | pushaf   | 01         |         |       |       |       | Pops 1 pointer off the stack and pushes the float pointed by it onto the stack
| -      | pop      | 02         | i8      |       |       |       | Pops Op1(n) items off the stack
| -      | popln    | 02         | lv8     |       |       |       | Pop item off the stack and store to local variable in near range (Op1)
| -      | popl     | 03         | lv16    |       |       |       | Pop item off the stack and store to local variable (Op1)
| -      | pople    | 05         | lv32    |       |       |       | Pop item off the stack and store to local variable in extended range (Op1)
| -      | popv     | 03         | v16     |       |       |       | Pop item off the stack and store to global variable (Op1)
| -      | popve    | 05         | v32     |       |       |       | Pop item off the stack and store to global variable in extended range (Op1)
| -      | swap     | 01         |         |       |       |       | Pops 2 items off the stack, reverses their order and pushes them back onto the stack
| -      | dup      | 01         |         |       |       |       | Pushes the item off the top of the stack back once
| -      | dupe     | 02         | i8      |       |       |       | Pushes the item off the top of the stack back Op1(n) times

### Variable Access
| Opcode | Mnemonic | Size       | Op1     | Op2   | Op3   | Op... | Description |
---------|----------|------------|---------|-------|-------|-------|-------------
| -      | local    | 01         |         |       |       |       | Pops the last item off the stack and pushes the address of local variable s0
| -      | global   | 01         |         |       |       |       | Pops the last item off the stack and pushes the address of global variable s0
| -      | array    | 02         | i8      |       |       |       | Pops 2 items off the stack, pushes an array index pointer where s0 is the index, s1 is the offset (base address) and (Op1) is the element size

### Arithimetic/Bitwise/Conversion Operations
| Opcode | Mnemonic | Size       | Op1     | Op2   | Op3   | Op... | Description |
---------|----------|------------|---------|-------|-------|-------|-------------
| -      | exf      | 01         |         |       |       |       | Sets the floating-point flag (FF) for the next instruction
| -      | sf       | 01         |         |       |       |       | Sets the sign flag (SF) for the next instruction
| -      | inc      | 01         |         |       |       |       | Pops 1 item off the stack, increments it, and pushes the resulting value
| -      | dec      | 01         |         |       |       |       | Pops 1 item off the stack, decrements it, and pushes the resulting value
| -      | add      | 01         |         |       |       |       | Pops 2 items off the stack, adds s1 to s0 and pushes the result
| -      | sub      | 01         |         |       |       |       | Pops 2 items off the stack, subtracts s1 from s0 and pushes the result
| -      | mul      | 01         |         |       |       |       | Pops 2 items off the stack, multiplies s1 by s0 and pushes the result
| -      | div      | 01         |         |       |       |       | Pops 2 items off the stack, divides s1 by s0 and pushes the result
| -      | mod      | 01         |         |       |       |       | Pops 2 items off the stack, performs a remainder operation on s1 with s0 and pushes the result
| -      | not      | 01         |         |       |       |       | Pops 1 item off the stack, performs a bitwise NOT operation on it and pushes the result
| -      | and      | 01         |         |       |       |       | Pops 2 items off the stack, performs a bitwise AND operation on s1 with s0 and pushes the result
| -      | or       | 01         |         |       |       |       | Pops 2 items off the stack, performs a bitwise OR operation on s1 with s0 and pushes the result
| -      | xor      | 01         |         |       |       |       | Pops 2 items off the stack, performs a bitwise XOR operation on s1 with s0 and pushes the result
| -      | shl      | 01         |         |       |       |       | Pops 2 items off the stack, performs a bitwise SHL operation on s1 by s0 and pushes the result
| -      | shr      | 01         |         |       |       |       | Pops 2 items off the stack, performs a bitwise SHR operation on s1 by s0 and pushes the result
| -      | neg      | 01         |         |       |       |       | Pops 1 item off the stack, negates it and pushes the result
| -      | toi      | 01         |         |       |       |       | Pops 1 item off the stack, converts it from to float, and pushes the result
| -      | tof      | 01         |         |       |       |       | Pops 1 item off the stack, converts it from to integer, and pushes the result

### Comparison
| Opcode | Mnemonic | Size       | Op1     | Op2   | Op3   | Op... | Description |
---------|----------|------------|---------|-------|-------|-------|-------------
| -      | cmpnn    | 01         |         |       |       |       | Pushes TRUE to the conditional stack if the last item on the stack is not NULL
| -      | cmpe     | 01         |         |       |       |       | Pushes TRUE to the conditional stack if s0 and s1 are equal as integers
| -      | cmpne    | 01         |         |       |       |       | Pushes TRUE to the conditional stack if s0 and s1 are inequal as integers
| -      | cmpge    | 01         |         |       |       |       | Pushes TRUE to the conditional stack if s0 is greater or equal to s1 as an integer
| -      | cmple    | 01         |         |       |       |       | Pushes TRUE to the conditional stack if s0 is lesser or equal to s1 as an integer
| -      | cmpg     | 01         |         |       |       |       | Pushes TRUE to the conditional stack if s0 is greater than s1 as an integer
| -      | cmpl     | 01         |         |       |       |       | Pushes TRUE to the conditional stack if s0 is lesser than s1 as an integer
| -      | if       | 01         |         |       |       |       | Resets the conditional stack
| -      | eval     | 02         | i8      |       |       |       | Pops (Op1) condition results, and pushes TRUE back is they are all true. If the highest-order bi of (Op1) is set, it pushes TRUE if any are true.

### Branching
| Opcode | Mnemonic | Size       | Op1     | Op2   | Op3   | Op... | Description |
---------|----------|------------|---------|-------|-------|-------|-------------
| -      | jt       | 05         | r32     |       |       |       | Jumps to (Op1) if the last item on the conditional stack is TRUE
| -      | jnt      | 05         | r32     |       |       |       | Jumps to (Op1) if the last item on the conditional stack is FALSE
| -      | jmp      | 01         |         |       |       |       | Pops the last item off the stack and jumps to s0 (ptr32)
| -      | jmpd     | 05         | r32     |       |       |       | Jumps to (Op1)
| -      | switch   | 07         | i16     | i32   |       | i32,r32 | Pops the last item off the stack. Compares s0 to Op...[0 to (Op1)][0], jumps to Op...[n][1] on match or (Op1) if no match
| -      | rswitch  | 0C+(Op2-Op1)*4 | i32 | i32   | r32   | r32   | Pops the last item off the stack. Jumps to (Op3) if the value isnt between (Op1) and (Op2), else jumps to  Op...[s0-Op1]

### Functions
| Opcode | Mnemonic | Size       | Op1     | Op2   | Op3   | Op... | Description |
---------|----------|------------|---------|-------|-------|-------|-------------
| -      | call     | 01         |         |       |       |       | Pops the last item off the stack, pushes the current code position pointer to the call stack and jumps to s0
| -      | calld    | 05         | r32     |       |       |       | Pushes the current code position pointer to the call stack and jumps to (Op1)
| -      | enter    | 02         | i8      |       |       |       | Enters internal function. Pops (Op1) arguments from the stack to local variables 0-(Op1)
| -      | ret      | 01         |         |       |       |       | Pops the callee code position pointer from the call stack and jumps to it

> The following are more game/implementation specific - the upper-half will allow a workable VM alone.

### External Read/Write
| Opcode | Mnemonic | Size       | Op1     | Op2   | Op3   | Op... | Description |
---------|----------|------------|---------|-------|-------|-------|-------------
| -      | read     | 02         | i8      |       |       |       | Pops the last item off the stack, reads (Op1) bytes of the value at address s0 and pushes the result
| -      | write    | 02         | i8      |       |       |       | Pops 2 items off the stack, writes (Op1) bytes of the value s1 to the address s0
| -      | copy     | 01         |         |       |       |       | Pops 3 items off the stack, writes s0 bytes of data from the address s1 to address s2
| -      | fill     | 01         |         |       |       |       | Pops 3 items off the stack, writes s0 bytes of s1 to address s2
| -      | comp     | 01         |         |       |       |       | Pops 3 items off the stack, pushes TRUE to the conditional stack if s0 bytes from s1 and s2 match

### External Calling
| Opcode | Mnemonic | Size       | Op1     | Op2   | Op3   | Op... | Description |
---------|----------|------------|---------|-------|-------|-------|-------------
| -      | native   | 05         | i32     |       |       |       | Calls a native function. ...?
| -      | cmd      | 05         | i32     |       |       |       | Calls a library function. ...?
| -      | cdecl    | 02         | i8      |       |       |       | Pops 1+(Op1) items off the stack, pushes (Op1) popped args (from s1) to the real machine stack, calls s0, pops (Op1) args off the real machine stack, then pushes eax to the stack
| -      | stdc     | 02         | i8      |       |       |       | Pops 1+(Op1) items off the stack, pushes (Op1) popped args (from s1) to the real machine stack, calls s0, then pushes eax to the stack
| -      | thisc    | 02         | i8      |       |       |       | Pops 1+(Op1) items off the stack, pushes (Op1) popped args (from s2) to the real machine stack, moves s1 to ecx, calls s0 then pushes eax to the stack
| -      | fastc    | 02         | i8      |       |       |       | Pops 1+(Op1) items off the stack, pushes (Op1) popped args (from s3) to the real machine stack, moves s2 to edx, moves s1 to ecx, calls s0, pops (Op1) args off the real machine stack, then pushes eax to the stack

## CLARA Mnemonics

In CLARA, many similar instructions, e.g. the pushX instructions, can be deduced by using a single mnemonic, based on the operand types given.

### `push`

| Mnemonic | Instruction | Op1 |
|----------|-------------|-----|
| push     | pushn       | -   |
| push     | pushb       | i8  |
| push     | pushw       | i16 |
| push     | pushd       | i32 |
| push     | pushq       | i64 |
