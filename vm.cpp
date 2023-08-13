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

template<typename T>
concept NumberLike = requires(T a, T b) {
    a + b;
    a - b;
    a * b;
    a / b;
    a > b;
    a >= b;
    a < b;
    a <= b;
    a == b;
};

template<typename Callable>
concept IntFromInt = requires(Callable callable, int a) {
    {callable(a)} -> std::same_as<int>;
};


template<typename Callable, typename Ret>
concept FromInt = requires(Callable callable, int a) {
    {callable(a)} -> std::same_as<Ret>;
};

class BaseExecutor {
    /*
     * provides the skeleton of a VM for Piglet bytecode
     * has the reference implementation of all instructions that
     * uses common funcs not implemented in this class
     */
public:
    template<NumberLike BaseType, IntFromInt T1, IntFromInt T2, IntFromInt T3, IntFromInt T4, FromInt<BaseType> T5, typename T6, typename T7,
            typename T8, typename T9, typename T10>
    static int switch_instruction(T1 jump_if_true_instruction, T2 jump_instruction, T3 jump_if_false_instruction, T4 process_cafe,
                           T5 get_argument, T6 load_from_memory, T7 store_to_memory, T8 stack_pop,
                           T9 stack_push, T10 print, const int* instructions, int num) {
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
                    NumberLike auto arg = get_argument(ip);
                    ip++;
                    NumberLike auto arg2 = stack_pop();
                    stack_push(arg2 + load_from_memory(arg));
                    break;
                }
                case OP_LOADI: {
                    NumberLike auto arg = get_argument(ip);
                    ip++;
                    stack_push(load_from_memory(arg));
                    break;
                }
                case OP_PUSHI: {
                    NumberLike auto arg = get_argument(ip);
                    ip++;
                    stack_push(arg);
                    break;
                }
                case OP_DISCARD: {
                    stack_pop();
                    break;
                }
                case OP_STOREI: {
                    auto addr = get_argument(ip);
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
                    BaseType val = stack_pop();
                    auto addr = stack_pop();
                    store_to_memory(addr, val);
                    break;
                }
                case OP_ADDI: {
                    auto arg = get_argument(ip);
                    ip++;
                    auto arg2 = stack_pop();
                    stack_push(arg2 + arg);
                    break;
                }
                case OP_DUP: {
                    auto var = stack_pop();
                    stack_push(var);
                    stack_push(var);
                    break;
                }
                case OP_SUB: {
                    auto arg = stack_pop();
                    auto arg2 = stack_pop();
                    stack_push(arg2 - arg);
                    break;
                }
                case OP_ADD: {
                    auto arg = stack_pop();
                    auto arg2 = stack_pop();
                    stack_push(arg + arg2);
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
                    auto arg = get_argument(ip);
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
                    ip = process_cafe(ip);
                    break;
                }
                default: {
                    throw runtime_error("not implemented instruction");
                }
            }
        }
        return ip;
    }
};

class jit_compiled_func : public jit_function
{
public:
    explicit jit_compiled_func(jit_context& context, int* instructions, bool is_void, int num_args, int* ip , int* memory, int* stack, int instr_num):
                     jit_function(context),
                     instructions(instructions), num_args(num_args), ip_ptr(ip), is_void(is_void)
    {
        this->memory_outer = memory;
        this->stack_outer = stack;
        this->instructions_number = instr_num;
        create();
        set_recompilable();
    }

    int jump_if_true_instruction(int ip) {
        auto arg = instructions[ip];
        ip++;
        if (labels.count(arg) == 0) {
            labels[arg] = jit_label_undefined;
        }
        insn_branch_if(stack_pop(), labels[arg]);
        return ip;
    }

    int jump_instruction(int ip) {
        auto arg = instructions[ip];
        ip++;
        if (labels.count(arg) == 0) {
            labels[arg] = jit_label_undefined;
        }
        insn_branch_if(new_constant(true), labels[arg]);
        return ip;
    }

    int jump_if_false_instruction(int ip) {
        auto arg = instructions[ip];
        ip++;
        if (labels.count(arg) == 0) {
            labels[arg] = jit_label_undefined;
        }
        insn_branch_if_not(stack_pop(), labels[arg]);
        return ip;
    }

    int process_cafe(int ip) {
        // skipping 0xbabe
        ip++;
        // ip is now pointing to the instruction after the label
        if (labels.count(ip) == 0) {
            labels[ip] = jit_label_undefined;
        }
        insn_label(labels[ip]);
        return ip;
    }

    void build() override {
        cerr << "function being built" << endl;
        memory = new_value(jit_type_create_pointer(jit_type_int, 1));
        memory = this->new_constant(memory_outer);
        stack = new_value(jit_type_create_pointer(jit_type_int, 1));
        stack = this->new_constant(stack_outer);
        stack_size = new_value(jit_type_int);
        stack_size = new_constant(0);
        BaseExecutor::switch_instruction<jit_value>([this] (int a) { return jump_if_true_instruction(a);}, [this] (int a) { return jump_instruction(a);},
                                              [this] (int a) { return jump_if_false_instruction(a);}, [this] (int a) { return process_cafe(a);},
                                              [this] (int a) { return get_argument(a);}, [this] (const jit_value& a) { return load_from_memory(a);},
                                              [this] (const jit_value& a, const jit_value& b) { store_to_memory(a, b);}, [this] () { return stack_pop();},
                                              [this] (const jit_value& a) { return stack_push(a);}, [this] (const jit_value& a) { return print(a);},
                                              instructions, instructions_number);
        insn_return();
    }

protected:
    jit_value load_from_memory(const jit_value& index) {
        return insn_load_elem(memory, index, jit_type_int);
    }

    jit_value get_argument(int ip) {
        return new_constant(instructions[ip]);
    }

    void store_to_memory(const jit_value& index, const jit_value& element) {
        insn_store_elem(memory, index, element);
    }

    jit_value stack_pop() {
        stack_size = stack_size - new_constant(1);
        return insn_load_elem(stack, stack_size, jit_type_int);
    }

    void stack_push(const jit_value& element) {
        insn_store_elem(stack, stack_size, element);
        stack_size = stack_size + new_constant(1);
    }

    void print(const jit_value& element) {
        auto tmp = element.raw();
        this->insn_call_native("print_int", (void *)(&print_int), signature_helper(jit_type_void, jit_type_int, end_params),
                               &tmp, 1, 0);
    }

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
    bool is_void;
    int* memory_outer;
    int* stack_outer;
    int instructions_number;
    unordered_map<int, jit_label> labels;
    jit_value memory, stack, stack_size;
};

class VM {
public:
    // program is the pointer to instructions; size is the number of instructions
    explicit VM(int *program, int size) {
        instructions = program;
        instructions_number = size;
    }

    void run() {
        BaseExecutor::switch_instruction<int>([this] (int a) { return jump_if_true_instruction(a);}, [this] (int a) { return jump_instruction(a);},
                                              [this] (int a) { return jump_if_false_instruction(a);}, [] (int a) { return process_cafe(a);},
                                              [this] (int a) { return get_argument(a);}, [this] (int a) { return load_from_memory(a);},
                                              [this] (int a, int b) { store_to_memory(a, b);}, [this] () { return stack_pop();},
                                              [this] (int a) { return stack_push(a);}, [] (int a) { return print(a);},
                                              instructions, instructions_number);
    }

    static int process_cafe(int ip) {
        // skip 0xbabe
        ip++;
        return ip;
    }

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

    int get_argument(int ip) {
        return instructions[ip];
    }

    void jit_run() {
        int ip = 0;
        jit_context context;
        jit_compiled_func new_func(context, instructions, true, 0, &ip, memory, stack, instructions_number);
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
    friend class jit_compiled_func;

    void store_to_memory(int addr, int value) {
        memory[addr] = value;
    }

    void stack_push(int element) {
        stack[stack_size] = element;
        stack_size++;
    }

    int stack_pop() {
        return stack[--stack_size];
    }

    int load_from_memory(int index) {
        return memory[index];
    }

    static void print(int element) {
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