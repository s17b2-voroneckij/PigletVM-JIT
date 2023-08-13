#include <cstdio>

class BaseWorker {
public:
    template <typename FirstFuncType>
    static void do_work(FirstFuncType something_small) {
        something_small(5);
    }
};

class MainWorker {
public:
    void callBaseWorker() {
        auto lambda1 = [this] (int a) {f(a);};
        BaseWorker::do_work(lambda1);
    }

    int f(int a) {
        printf("%d\n", a + 1);
        return a + 1;
    }
};

int main() {
    MainWorker worker;
    worker.callBaseWorker();
    printf("Hello");
}