#include <vector>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <jit/jit-plus.h>
#include <jit/jit.h>
#include <unordered_map>

using namespace std;

enum opcodes {
    OP_PUSHI,
    OP_LOADI,
    OP_LOADADDI,
    OP_STOREI,
    OP_LOAD,
    OP_STORE,
    OP_DUP,
    OP_DISCARD,
    OP_ADD,
    OP_ADDI,
    OP_SUB,
    OP_DIV,
    OP_MUL,
    OP_JUMP,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE,
    OP_EQUAL,
    OP_LESS,
    OP_LESS_OR_EQUAL,
    OP_GREATER,
    OP_GREATER_OR_EQUAL,
    OP_GREATER_OR_EQUALI,
    OP_POP_RES,
    OP_DONE,
    OP_PRINT,
    OP_ABORT
};

const int STACK_SIZE = 8096, MEMORY_SIZE = 140000;

void print_int(int n) {
    cout << n << "\n";
}

template<typename BaseType>
class BaseExecutor {
    /*
     * provides the skeleton of a VM for Piglet bytecode
     * has the reference implementation of all instructions that
     * uses common funcs not implemented in this class
     */
protected:
    int jump_if_true_instruction(int ip) {
        auto arg = instructions[ip];
        ip++;
        if (stack_pop()) {
            ip = arg;
        }
        return ip;
    }

    int jump_instruction(int ip) {
        auto arg = instructions[ip];
        ip++;
        ip = arg;
        return ip;
    }

    int jump_if_false_instruction(int ip) {
        auto arg = instructions[ip];
        ip++;
        if (!stack_pop()) {
            ip = arg;
        }
        return ip;
    }

    int switch_instruction(int num) {
        /*
         * accepts an instruction pointer pointing to the next instruction to execute, and returns the
         * new instruction pointer after execution
         */
        int ip = 0;
        while (ip < num) {
            ip++;
            switch (instructions[ip - 1]) {
                case OP_JUMP: {
                    ip = jump_instruction(ip);
                    break;
                }
                case OP_JUMP_IF_TRUE: {
                    ip = jump_if_true_instruction(ip);
                    break;
                }
                case OP_JUMP_IF_FALSE: {
                    ip = jump_if_false_instruction(ip);
                    break;
                }
                case OP_LOADADDI: {
                    auto arg = instructions[ip];
                    ip++;
                    stack_peek() += load_from_memory(arg);
                    break;
                }
                case OP_LOADI: {
                    auto arg = instructions[ip];
                    ip++;
                    stack_push(load_from_memory(arg));
                    break;
                }
                case OP_PUSHI: {
                    auto arg = instructions[ip];
                    ip++;
                    stack_push(arg);
                    break;
                }
                case OP_DISCARD: {
                    stack_pop();
                    break;
                }
                case OP_STOREI: {
                    auto addr = instructions[ip];
                    ip++;
                    store_to_memory(addr, stack_pop());
                    break;
                }
                case OP_LOAD: {
                    auto addr = stack_pop();
                    stack_push(load_from_memory(addr));
                    break;
                }
                case OP_STORE: {
                    auto val = stack_pop();
                    auto addr = stack_pop();
                    store_to_memory(addr, val);
                    break;
                }
                case OP_ADDI: {
                    auto arg = instructions[ip];
                    ip++;
                    stack_peek() += arg;
                    break;
                }
                case OP_DUP: {
                    stack_push(stack_peek());
                    break;
                }
                case OP_SUB: {
                    auto arg = stack_pop();
                    stack_peek() -= arg;
                    break;
                }
                case OP_ADD: {
                    auto arg = stack_pop();
                    stack_peek() += arg;
                    break;
                }
                case OP_DIV: {
                    auto arg = stack_pop();
                    auto arg2 = stack_pop();
                    stack_push(arg2 / arg);
                    break;
                }
                case OP_MUL: {
                    auto arg = stack_pop();
                    auto arg2 = stack_pop();
                    stack_push(arg * arg2);
                    break;
                }
                case OP_ABORT: {
                    return -1;
                }
                case OP_DONE: {
                    return -1;
                }
                case OP_PRINT: {
                    print(stack_pop());
                    break;
                }
                case OP_POP_RES: {
                    stack_pop();
                    break;
                }
                case OP_GREATER_OR_EQUALI: {
                    auto arg = instructions[ip];
                    auto arg2 = stack_pop();
                    ip++;
                    stack_push(arg2 >= arg);
                    break;
                }
                case OP_GREATER_OR_EQUAL: {
                    auto arg = stack_pop();
                    auto arg2 = stack_pop();
                    stack_push(arg2 >= arg);
                    break;
                }
                case OP_GREATER: {
                    auto arg = stack_pop();
                    auto arg2 = stack_pop();
                    stack_push(arg2 > arg);
                    break;
                }
                case OP_LESS: {
                    auto arg = stack_pop();
                    auto arg2 = stack_pop();
                    stack_push(arg2 < arg);
                    break;
                }
                case OP_LESS_OR_EQUAL: {
                    auto arg = stack_pop();
                    auto arg2 = stack_pop();
                    stack_push(arg2 <= arg);
                    break;
                }
                case OP_EQUAL: {
                    auto arg = stack_pop();
                    auto arg2 = stack_pop();
                    stack_push(arg2 == arg);
                    break;
                }
                case 0xcafe: {
                    // skip 0xbabe
                    ip++;
                    break;
                }
                default: {
                    throw runtime_error("not implemented instruction");
                }
            }
        }
        return ip;
    }

    virtual BaseType load_from_memory(int index) = 0;

    virtual void store_to_memory(int index, BaseType element) = 0;

    virtual BaseType stack_pop() = 0;

    virtual void stack_push(BaseType element) = 0;

    virtual void print(BaseType element) = 0;

    virtual BaseType& stack_peek() = 0;

    explicit BaseExecutor(int *instructions) : instructions(instructions) {}
    virtual ~BaseExecutor() = default;

private:
    int* instructions;
};

class jit_compiled_func : public jit_function
{
public:
    explicit jit_compiled_func(jit_context& context, int* instructions, bool is_void, int num_args, int* ip , int* memory, int* stack, int* stack_size):
                     jit_function(context),
                     instructions(instructions), num_args(num_args), ip_ptr(ip), is_void(is_void)
    {
        this->memory_outer = memory;
        this->stack_outer = stack;
        this->stack_size_ptr_outer = stack_size;
        create();
        set_recompilable();
    }

    void build() override {
        cerr << "function being built" << endl;
        auto memory = new_value(jit_type_create_pointer(jit_type_int, 1));
        memory = this->new_constant(memory_outer);
        auto stack = new_value(jit_type_create_pointer(jit_type_int, 1));
        stack = this->new_constant(stack_outer);
        jit_value stack_size = new_value(jit_type_int);
        stack_size = new_constant(0);
        auto push = [this, &stack, &stack_size] (jit_value&& value) {
            insn_store_elem(stack, stack_size, value);
            stack_size = stack_size + new_constant(1);
        };
        auto pop = [this, &stack, &stack_size] () {
            stack_size = stack_size - new_constant(1);
            return insn_load_elem(stack, stack_size, jit_type_int);
        };
        auto peek = [this, &stack, &stack_size] () {
            return insn_load_elem(stack, stack_size - new_constant(1), jit_type_int);
        };
        int& ip = *ip_ptr;
        unordered_map<int, jit_label> labels;
        while (true) {
            ip++;
            switch (instructions[ip - 1]) {
                case OP_PUSHI: {
                    auto arg = instructions[ip];
                    ip++;
                    push(new_constant(arg));
                    break;
                }
                case OP_STORE: {
                    auto val = pop();
                    auto index = pop();
                    insn_store_elem(memory, index, val);
                    break;
                }
                case OP_STOREI: {
                    auto addr = instructions[ip];
                    ip++;
                    insn_store_elem(memory, new_constant(addr), pop());
                    break;
                }
                case OP_LOAD: {
                    auto ind = pop();
                    push(insn_load_elem(memory, ind, jit_type_int));
                    break;
                }
                case OP_PRINT: {
                    auto tmp = pop().raw();
                    this->insn_call_native("print_int", (void *)(&print_int), signature_helper(jit_type_void, jit_type_int, end_params),
                                           &tmp, 1, 0);
                    break;
                }
                /*case OP_DONE: {
                    insn_return();
                    return;
                }*/
                case OP_ADD: {
                    auto arg1 = pop();
                    auto arg2 = pop();
                    push( arg1 + arg2);
                    break;
                }
                case OP_SUB: {
                    auto arg1 = pop();
                    auto arg2 = pop();
                    push( arg2 - arg1);
                    break;
                }
                case OP_MUL: {
                    auto arg1 = pop();
                    auto arg2 = pop();
                    push(arg2 * arg1);
                    break;
                }
                case OP_DIV: {
                    auto arg1 = pop();
                    auto arg2 = pop();
                    // TODO: check that arg2 is not zero
                    push(arg2 / arg1);
                    break;
                }
                case OP_EQUAL: {
                    auto arg1 = pop();
                    auto arg2 = pop();
                    push(arg2 == arg1);
                    break;
                }
                case OP_GREATER: {
                    auto arg1 = pop();
                    auto arg2 = pop();
                    push(arg2 > arg1);
                    break;
                }
                case OP_GREATER_OR_EQUAL: {
                    auto arg1 = pop();
                    auto arg2 = pop();
                    push(arg2 >= arg1);
                    break;
                }
                case OP_LESS: {
                    auto arg1 = pop();
                    auto arg2 = pop();
                    push(arg2 < arg1);
                    break;
                }
                case OP_LESS_OR_EQUAL: {
                    auto arg1 = pop();
                    auto arg2 = pop();
                    push(arg2 <= arg1);
                    break;
                }
                case OP_LOADI: {
                    auto arg = instructions[ip];
                    ip++;
                    push(insn_load_elem(memory, new_constant(arg), jit_type_int));
                    break;
                }
                case OP_ADDI: {
                    auto arg = instructions[ip];
                    ip++;
                    push(pop() + new_constant(arg));
                    break;
                }
                case OP_DUP: {
                    push(peek());
                    break;
                }
                case OP_DISCARD: {
                    pop();
                    break;
                }
                case OP_GREATER_OR_EQUALI: {
                    auto arg = instructions[ip];
                    ip++;
                    //stack[stack_size - 1] = stack[stack_size - 1] >= arg;
                    push(pop() >= new_constant(arg));
                    break;
                }
                case OP_LOADADDI: {
                    auto arg = instructions[ip];
                    ip++;
                    //stack[stack_size - 1] += memory[arg];
                    push(pop() + insn_load_elem(memory, new_constant(arg), jit_type_int));
                    break;
                }
                case 0xcafe: {
                    // skipping 0xbabe
                    ip++;
                    // ip is now pointing to the instruction after the label
                    if (labels.count(ip) == 0) {
                        labels[ip] = jit_label_undefined;
                    }
                    insn_label(labels[ip]);
                    break;
                }
                case OP_DONE: {
                    // return normally
                    insn_store_elem(new_constant(ip_ptr), new_constant(0), new_constant(ip, jit_type_int));
                    insn_return();
                    return;
                }
                case OP_JUMP: {
                    auto arg = instructions[ip];
                    ip++;
                    if (labels.count(arg) == 0) {
                        labels[arg] = jit_label_undefined;
                    }
                    insn_branch_if(new_constant(true), labels[arg]);
                    break;
                }
                case OP_JUMP_IF_TRUE: {
                    auto arg = instructions[ip];
                    ip++;
                    if (labels.count(arg) == 0) {
                        labels[arg] = jit_label_undefined;
                    }
                    insn_branch_if(pop(), labels[arg]);
                    break;
                }
                case OP_JUMP_IF_FALSE: {
                    auto arg = instructions[ip];
                    ip++;
                    if (labels.count(arg) == 0) {
                        labels[arg] = jit_label_undefined;
                    }
                    insn_branch_if_not(pop(), labels[arg]);
                    break;
                }
                default: {
                    insn_store_elem(new_constant(ip_ptr), new_constant(0), new_constant(ip, jit_type_int));
                    cout << "UNKNOWN INSTRUCTION " << instructions[ip - 1] << endl;
                    insn_return();
                    return;
                }
            }
        }
    }

protected:
    jit_type_t create_signature() override {
        auto ret_type = (is_void ? jit_type_void : jit_type_int);
        jit_type_t params[num_args];
        for (int i = 0; i < num_args; i++) {
            params[i] = jit_type_int;
        }
        return jit_type_create_signature(jit_abi_cdecl, ret_type, params, num_args, 1);
    }

private:
    int* instructions;
    int num_args;
    int* ip_ptr;
    bool is_void;
    int* memory_outer;
    int* stack_outer;
    int* stack_size_ptr_outer;
};

class VM: public BaseExecutor<int> {
public:
    // program is the pointer to instructions; size is the number of instructions
    explicit VM(int *program, int size) : BaseExecutor(program), instructions(program) {
        instructions_number = size;
    }

    void run() {
        int ip_ = 0;
        switch_instruction(instructions_number);
    }

    void jit_run() {
        int ip = 0;
        jit_context context;
        jit_compiled_func new_func(context, instructions, true, 0, &ip, memory, stack, &stack_size);
        new_func.build_start();
        new_func.build();
        jit_dump_function(stdout, new_func.raw(), "main");

        new_func.set_optimization_level(3);
        new_func.compile();
        new_func.build_end();
        jit_dump_function(stdout, new_func.raw(), "main");
        auto func = (void(*)())new_func.closure();
        double clocks_before = clock();
        func();
        cerr << "compiled function ran in " << (clock() - clocks_before) / CLOCKS_PER_SEC << endl;
    }

private:
    static bool is_jump(int instr) {
        return instr == OP_JUMP || instr == OP_JUMP_IF_FALSE || instr == OP_JUMP_IF_TRUE;
    }

    friend class jit_compiled_func;

    void store_to_memory(int addr, int value) override {
        memory[addr] = value;
    }

    void stack_push(int element) override {
        stack[stack_size] = element;
        stack_size++;
    }

    int stack_pop() override {
        return stack[--stack_size];
    }

    int load_from_memory(int index) override {
        return memory[index];
    }

    int& stack_peek() override {
        return stack[stack_size - 1];
    }

    void print(int element) override {
        cout << element << "\n";
    }

    int* instructions;
    int stack[STACK_SIZE];
    int memory[MEMORY_SIZE];
    int stack_size = 0;
    int instructions_number = 0;
};

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "usage ./vm program.pvm\n";
        return 1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        cerr << strerror(errno);
        return 1;
    }
    struct stat file_info = {};
    int ret = fstat(fd, &file_info);
    if (ret < 0) {
        cerr << strerror(errno);
        return 1;
    }
    auto file_size = file_info.st_size;
    int* memory = (int *) mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (memory == MAP_FAILED) {
        cerr << strerror(errno);
        return 1;
    }
    VM vm(memory, file_size / sizeof(*memory));
    for (int i = 0; i < 1; i++) {
        vm.jit_run();
    }
    if (munmap(memory, file_size) < 0 || close(fd)) {
        cerr << strerror(errno);
        return 1;
    }
}