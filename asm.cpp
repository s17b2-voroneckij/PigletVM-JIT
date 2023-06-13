#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>

using namespace std;

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "usage: ./asm program.c.pvm program.pvm";
        return 1;
    }
    unordered_map<string, int> opcodes = {
            {"PUSHI", 0},
            {"LOADI", 1},
            {"LOADADDI", 2},
            {"STOREI", 3},
            {"LOAD", 4},
            {"STORE", 5},
            {"DUP", 6},
            {"DISCARD", 7},
            {"ADD", 8},
            {"ADDI", 9},
            {"SUB", 10},
            {"DIV", 11},
            {"MUL", 12},
            {"JUMP", 13},
            {"JUMP_IF_TRUE", 14},
            {"JUMP_IF_FALSE", 15},
            {"EQUAL", 16},
            {"LESS", 17},
            {"LESS_OR_EQUAL", 18},
            {"GREATER", 19},
            {"GREATER_OR_EQUAL", 20},
            {"GREATER_OR_EQUALI", 21},
            {"PRES", 22},
            {"DONE", 23},
            {"PRINT", 24},
            {"ABORT", 25}
    };
    unordered_set<string> simple_arg_opcodes = {
            "PUSHI", "LOADI", "LOADADDI", "STOREI", "ADDI", "GREATER_OR_EQUALI",

    };
    unordered_set<string> jump_opcodes = {
            "JUMP", "JUMP_IF_TRUE", "JUMP_IF_FALSE"
    };
    ifstream in(argv[1]);
    vector<int> result;
    unordered_map<string, size_t> labels;
    // vector with locations of label addresses to fill
    vector<pair<string, size_t>> labels_to_fill;
    string buf;
    while (in >> buf) {
        if (*buf.rbegin() == ':') {
            // this is a label
            // it should point to the next insruction that we will push

            // pop :
            buf.pop_back();
            labels[buf] = result.size();
            // put information about the presence of a label
            result.push_back(0xcafe);
            result.push_back(0xbabe);
        } else {
            // otherwise this is an instruction
            // if it has an argument, we need to read it
            result.push_back(opcodes.at(buf));
            if (simple_arg_opcodes.count(buf)) {
                int arg;
                in >> arg;
                result.push_back(arg);
            } else if (jump_opcodes.count(buf)) {
                string label;
                in >> label;
                result.push_back(0);
                labels_to_fill.emplace_back(label, result.size() - 1);
            }
        }
    }
    // now let`s fill label addresses
    for (auto& p : labels_to_fill) {
        result[p.second] = labels[p.first];
    }

    // now let`s write
    int fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0666);
    auto size = result.size() * sizeof(result[0]);
    if (fd < 0 || ftruncate(fd, size)) {
        cerr << strerror(errno);
        return 1;
    }
    void* memory = mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (memory == MAP_FAILED) {
        cerr << strerror(errno);
        return 1;
    }
    memcpy(memory, &result[0], size);
    munmap(memory, size);
    close(fd);
}
