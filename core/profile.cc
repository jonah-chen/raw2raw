/**
 * Raw2Raw
 * core/profile.cc
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the implementation of a very basic RAII timer classes that uses OpenMP to measure time.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#include "raw2raw.h"
#include <omp.h>
namespace r2r {
Timer::Timer(bool _start) { if (_start) start(); }
void Timer::start() { start_ = omp_get_wtime(); }
double Timer::stop()
{
    #pragma omp barrier
    double t = omp_get_wtime() - start_;
    start();
    return t * 1000;
}
} // namespace r2r
