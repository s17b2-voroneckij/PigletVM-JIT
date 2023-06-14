#include <vector>
#include <iostream>
#include <sys/types.h>
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
        cout << "build() called\n";
        auto memory = new_value(jit_type_create_pointer(jit_type_int, 1));
        memory = this->new_constant(memory_outer);
        auto stack = new_value(jit_type_create_pointer(jit_type_int, 1));
        stack = this->new_constant(stack_outer);
        jit_value stack_size_ptr = new_value(jit_type_create_pointer(jit_type_int, 1));
        stack_size_ptr = this->new_constant(stack_size_ptr_outer);
        int& ip = *ip_ptr;
        while (true) {
            ip++;
            switch (instructions[ip - 1]) {
                case OP_PUSHI: {
                    auto arg = instructions[ip];
                    ip++;
                    push_on_stack(stack, stack_size_ptr, arg);
                    break;
                }
                case OP_STORE: {
                    auto val = pop_from_stack(stack, stack_size_ptr);
                    auto index = pop_from_stack(stack, stack_size_ptr);
                    insn_store_elem(memory, index, val);
                    break;
                }
                case OP_LOAD: {
                    auto ind = pop_from_stack(stack, stack_size_ptr);
                    push_on_stack(stack, stack_size_ptr, insn_load_elem(memory, ind, jit_type_int));
                    break;
                }
                case OP_PRINT: {
                    auto tmp = pop_from_stack(stack, stack_size_ptr).raw();
                    this->insn_call_native("print_int", (void *)(&print_int), signature_helper(jit_type_void, jit_type_int, end_params),
                                           &tmp, 1, 0);
                    break;
                }
                /*case OP_DONE: {
                    insn_return();
                    return;
                }*/
                case OP_ADD: {
                    auto arg1 = pop_from_stack(stack, stack_size_ptr);
                    auto arg2 = pop_from_stack(stack, stack_size_ptr);
                    push_on_stack(stack, stack_size_ptr, arg1 + arg2);
                    break;
                }
                case OP_SUB: {
                    auto arg1 = pop_from_stack(stack, stack_size_ptr);
                    auto arg2 = pop_from_stack(stack, stack_size_ptr);
                    push_on_stack(stack, stack_size_ptr, arg2 - arg1);
                    break;
                }
                case OP_MUL: {
                    auto arg1 = pop_from_stack(stack, stack_size_ptr);
                    auto arg2 = pop_from_stack(stack, stack_size_ptr);
                    push_on_stack(stack, stack_size_ptr, arg2 * arg1);
                    break;
                }
                case OP_DIV: {
                    auto arg1 = pop_from_stack(stack, stack_size_ptr);
                    auto arg2 = pop_from_stack(stack, stack_size_ptr);
                    // TODO: check that arg2 is not zero
                    push_on_stack(stack, stack_size_ptr, arg2 / arg1);
                    break;
                }
                case OP_EQUAL: {
                    auto arg1 = pop_from_stack(stack, stack_size_ptr);
                    auto arg2 = pop_from_stack(stack, stack_size_ptr);
                    push_on_stack(stack, stack_size_ptr, arg2 == arg1);
                    break;
                }
                case OP_GREATER: {
                    auto arg1 = pop_from_stack(stack, stack_size_ptr);
                    auto arg2 = pop_from_stack(stack, stack_size_ptr);
                    push_on_stack(stack, stack_size_ptr, arg2 > arg1);
                    break;
                }
                case OP_GREATER_OR_EQUAL: {
                    auto arg1 = pop_from_stack(stack, stack_size_ptr);
                    auto arg2 = pop_from_stack(stack, stack_size_ptr);
                    push_on_stack(stack, stack_size_ptr, arg2 >= arg1);
                    break;
                }
                case OP_LESS: {
                    auto arg1 = pop_from_stack(stack, stack_size_ptr);
                    auto arg2 = pop_from_stack(stack, stack_size_ptr);
                    push_on_stack(stack, stack_size_ptr, arg2 < arg1);
                    break;
                }
                case OP_LESS_OR_EQUAL: {
                    auto arg1 = pop_from_stack(stack, stack_size_ptr);
                    auto arg2 = pop_from_stack(stack, stack_size_ptr);
                    push_on_stack(stack, stack_size_ptr, arg2 <= arg1);
                    break;
                }
                case 0xcafe: {
                    ip++;
                }
                default: {
                    insn_return();
                    return;
                }
            }
        }
    }

    void push_on_stack(jit_value& stack, jit_value& stack_size_ptr, int arg) {
        this->insn_store_elem(stack, insn_load_elem(stack_size_ptr, new_constant(0), jit_type_int), new_constant(arg));
        insn_store_elem(stack_size_ptr, new_constant(0),
                        insn_load_elem(stack_size_ptr, new_constant(0), jit_type_int) + new_constant(1));
    }

    void push_on_stack(jit_value& stack, jit_value& stack_size_ptr, jit_value&& arg) {
        this->insn_store_elem(stack, insn_load_elem(stack_size_ptr, new_constant(0), jit_type_int), arg);
        insn_store_elem(stack_size_ptr, new_constant(0),
                        insn_load_elem(stack_size_ptr, new_constant(0), jit_type_int) + new_constant(1));
    }

    jit_value pop_from_stack(jit_value& stack, jit_value& stack_size_ptr) {
        insn_store_elem(stack_size_ptr, new_constant(0),
                        insn_load_elem(stack_size_ptr, new_constant(0), jit_type_int) - new_constant(1));
        return insn_load_elem(stack, insn_load_elem(stack_size_ptr, new_constant(0), jit_type_int), jit_type_int);
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

class VM {
public:
    // program is the pointer to instructions; size is the number of instructions
    explicit VM(int* program, int size) : instructions(program), context() {
        instrunctions_number = size;
    }

    void run() {
        size_t ip = 0;
        while (ip < instrunctions_number) {
            ip++;
            switch (instructions[ip - 1]) {
                case OP_JUMP: {
                    auto arg = instructions[ip];
                    ip++;
                    ip = arg;
                    break;
                }
                case OP_JUMP_IF_TRUE: {
                    auto arg = instructions[ip];
                    ip++;
                    if (stack_pop()) {
                        ip = arg;
                    }
                    break;
                }
                case OP_JUMP_IF_FALSE: {
                    auto arg = instructions[ip];
                    ip++;
                    if (!stack_pop()) {
                        ip = arg;
                    }
                    break;
                }
                case OP_LOADADDI: {
                    auto arg = instructions[ip];
                    ip++;
                    stack[stack_size - 1] += memory[arg];
                    break;
                }
                case OP_LOADI: {
                    auto arg = instructions[ip];
                    ip++;
                    stack[stack_size++] = memory[arg];
                    break;
                }
                case OP_PUSHI: {
                    auto arg = instructions[ip];
                    ip++;
                    stack[stack_size++] = arg;
                    break;
                }
                case OP_DISCARD: {
                    stack_size--;
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
                    stack[stack_size++] = memory[addr];
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
                    stack[stack_size - 1] += arg;
                    break;
                }
                case OP_DUP: {
                    stack[stack_size] = stack[stack_size - 1];
                    stack_size++;
                    break;
                }
                case OP_SUB: {
                    auto arg = stack_pop();
                    stack[stack_size - 1] -= arg;
                    break;
                }
                case OP_ADD: {
                    auto arg = stack_pop();
                    stack[stack_size - 1] += arg;
                    break;
                }
                case OP_DIV: {
                    auto arg = stack_pop();
                    if (arg == 0) {
                        cerr << "ZERO DIVISION\n";
                        return;
                    }
                    stack[stack_size - 1] /= arg;
                    break;
                }
                case OP_MUL: {
                    auto arg = stack_pop();
                    stack[stack_size - 1] *= arg;
                    break;
                }
                case OP_ABORT: {
                    cerr << "OP_ABORT called\n";
                    return;
                }
                case OP_DONE: {
                    cout << "program DONE\n";
                    return;
                }
                case OP_PRINT: {
                    cout << stack_pop() << "\n";
                    break;
                }
                case OP_POP_RES: {
                    stack_pop();
                    break;
                }
                case OP_GREATER_OR_EQUALI: {
                    auto arg = instructions[ip];
                    ip++;
                    stack[stack_size - 1] = stack[stack_size - 1] >= arg;
                    break;
                }
                case OP_GREATER_OR_EQUAL: {
                    auto arg = stack_pop();
                    stack[stack_size - 1] = stack[stack_size - 1] >= arg;
                    break;
                }
                case OP_GREATER: {
                    auto arg = stack_pop();
                    stack[stack_size - 1] = stack[stack_size - 1] > arg;
                    break;
                }
                case OP_LESS: {
                    auto arg = stack_pop();
                    stack[stack_size - 1] = stack[stack_size - 1] < arg;
                    break;
                }
                case OP_LESS_OR_EQUAL: {
                    auto arg = stack_pop();
                    stack[stack_size - 1] = stack[stack_size - 1] <= arg;
                    break;
                }
                case OP_EQUAL: {
                    auto arg = stack_pop();
                    stack[stack_size - 1] = stack[stack_size - 1] == arg;
                    break;
                }
                case 0xcafe: {
                    // skip 0xbabe
                    ip++;
                }
            }
        }
        cerr << "no more instructions";
    }

    void jit_run() {
        ip = 0;
        while (ip < instrunctions_number) {
            if (is_jump(instructions[ip])) {
                ip++;
                switch (instructions[ip - 1]) {
                    case OP_JUMP: {
                        auto arg = instructions[ip];
                        ip++;
                        ip = arg;
                        break;
                    }
                    case OP_JUMP_IF_TRUE: {
                        auto arg = instructions[ip];
                        ip++;
                        if (stack_pop()) {
                            ip = arg;
                        }
                        break;
                    }
                    case OP_JUMP_IF_FALSE: {
                        auto arg = instructions[ip];
                        ip++;
                        if (!stack_pop()) {
                            ip = arg;
                        }
                        break;
                    }
                }
            }
            else if (instructions[ip] == OP_DONE) {
                return;
            } else {
                if (jit_cache.count(ip) == 0) {
                    jit_cache.insert_or_assign(ip, jit_compiled_func(context, instructions, true, 0, &ip, memory, stack, &stack_size));
                }
                auto func = (void(*)())jit_cache.at(ip).closure();
                func();
                ip--;
            }
        }
    }

private:
    static bool is_jump(int instr) {
        return instr == OP_JUMP || instr == OP_JUMP_IF_FALSE || instr == OP_JUMP_IF_TRUE;
    }

    friend class jit_compiled_func;

    void store_to_memory(int addr, int value) {
        memory[addr] = value;
    }

    int stack_pop() {
        return stack[--stack_size];
    }

    int* instructions;
    jit_context context;
    unordered_map<int, jit_compiled_func> jit_cache;
    int stack[STACK_SIZE];
    int memory[MEMORY_SIZE];
    int stack_size = 0;
    int ip = 0;
    int instrunctions_number = 0;
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