#include <stdlib.h>
#include <stdio.h>
#include <fenv.h>
#include <iostream>

int main(int argc, char ** args) {
    if(argc == 1) {
        printf("Please pass a number as a string as a parameter\n");
        return EXIT_SUCCESS;
    }
    int modes[4] = {FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO, FE_TONEAREST};
    for(int i = 0; i < 4; i++) {
       fesetround(modes[i]);
       double result = strtod(args[1], NULL);
       std::cout << std::hexfloat << result << " (" << std::defaultfloat << result << ")" << std::endl;
    }
    return EXIT_SUCCESS;
}
