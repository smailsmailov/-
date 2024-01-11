#include "ThirdLab/ThirdLab.h"
#include <iostream>

int main(int argc, char *argv[]){
    ThirdLab a = ThirdLab(argv[0]);
#if defined(WIN32)
    if (argc > 1) {
        if (strcmp(argv[1], "COPY1") == 0) {
            a.startProgram(1);
            return 0;
        }
        if (strcmp(argv[1], "COPY2") == 0) {
            a.startProgram(2);
            return 0;
        }
    }
#endif
    a.startProgram();
    return 0;
}