#include <iostream>
#include <cstring>
#include <cstdlib>

class N {
private:
    char annotation[100];
    int value;

public:
    N(int v) {
        value = v;
    }
    void setAnnotation(char *str) {
        memcpy(this->annotation, str, strlen(str));
    }

    virtual int execute() {
        return this->value;
    }
};

int main(int argc, char **argv) {
    if (argc < 2) {
        exit(1);
    }

    N *obj1 = new N(5);
    N *obj2 = new N(6);
    obj1->setAnnotation(argv[1]);
    return obj2->execute();
}