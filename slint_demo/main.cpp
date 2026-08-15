#include "demo.h"
#include <slint.h>

int main() {
    auto window = slint_demo::DemoWindow::create();
    window->run();
    return 0;
}
