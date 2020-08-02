extern "C"{
    void app_main(void);
}

#include "R502Interface.hpp"

void app_main()
{
    R502Interface R502;
    R502.init();
    R502.test1();
}
