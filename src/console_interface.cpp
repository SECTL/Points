module;

#include <iostream>

import storage.data;
export module console_interface;
namespace console_i{
    int console_main(){
        std::cout << "欢迎使用积分管理系统！" << std::endl;
        return 0;
    }

}