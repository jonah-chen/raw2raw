#include "raw2raw.h"
namespace r2r {
Timer::Timer(bool _start) { if (_start) start(); }
void Timer::start() { start_ = std::chrono::high_resolution_clock::now(); }
float Timer::stop()
{
    #pragma omp barrier
    auto end = std::chrono::high_resolution_clock::now();
    float t = std::chrono::duration<float>(end - start_).count();
    start();
    return t * 1000;
}
}
