/**
 * Raw2Raw
 * core/reduce.cc
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the implementation of the pixel-wise reduction algorithms. These algorithms are used to reduce the
 * number of images in a stack of images to a single image. The algorithms are implemented using OpenMP to parallelize
 * the computation.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */
#include "raw2raw.h"
#include <algorithm>
#include <cmath>

namespace r2r {
io_t *p_mean(const Task &task)
{
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel default(none) shared(task, ans)
    {
        interm_t s0, s1, s2, s3;

        #pragma omp for schedule(static)
        for (int i = 0; i < task.wh; i+=4) {
            s0 = s1 = s2 = s3 = 0;
            for (int j = 0; j < task.n_images; j++) {
                s0 += task.data[j * task.wh + i + 0];
                s1 += task.data[j * task.wh + i + 1];
                s2 += task.data[j * task.wh + i + 2];
                s3 += task.data[j * task.wh + i + 3];
            }
            ans[i + 0] = s0 / task.n_images;
            ans[i + 1] = s1 / task.n_images;
            ans[i + 2] = s2 / task.n_images;
            ans[i + 3] = s3 / task.n_images;
        }
    } 
    return ans;
}

io_t *p_median(const Task &task)
{
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel default(none) shared(task, ans)
    {
        io_t *buf = new io_t[task.n_images];
        #pragma omp for schedule(static)
        for (int i = 0; i < task.wh; i++) {
            for (int j = 0; j < task.n_images; j++) {
                buf[j] = task.data[j * task.wh + i];
            }
            std::nth_element(buf, buf + task.n_images / 2, buf + task.n_images);
            ans[i] = buf[task.n_images / 2];
        }
        delete[] buf;
    }
    return ans;
}

io_t *p_summation(const Task &task)
{
    interm_t limit = task.max_val;
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel for default(none) shared(task, ans, limit) schedule(static)
    for (int i = 0; i < task.wh; i++) {
        interm_t s = 0;
        for (int j = 0; j < task.n_images; j++) {
            s += task.data[j * task.wh + i];
        }
        ans[i] = std::min(s, limit);
    }
    return ans;
}

io_t *p_maximum(const Task &task)
{
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel for default(none) shared(task, ans) schedule(static)
    for (int i = 0; i < task.wh; i++) {
        io_t M = 0;
        for (int j = 0; j < task.n_images; j++) {
            M = std::max(M, task.data[j * task.wh + i]);
        }
        ans[i] = M;
    }
    return ans;
}

io_t *p_minimum(const Task &task)
{
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel for default(none) shared(task, ans) schedule(static)
    for (int i = 0; i < task.wh; i++) {
        io_t m = 0xffff;
        for (int j = 0; j < task.n_images; j++) {
            m = std::min(m, task.data[j * task.wh + i]);
        }
        ans[i] = m;
    }
    return ans;
}

io_t *p_range(const Task &task)
{
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel for default(none) shared(task, ans) schedule(static)
    for (int i = 0; i < task.wh; i++) {
        io_t m = 0xffff, M = 0;
        for (int j = 0; j < task.n_images; j++) {
            m = std::min(m, task.data[j * task.wh + i]);
            M = std::max(M, task.data[j * task.wh + i]);
        }
        ans[i] = M - m;
    }
    return ans;
}

io_t *p_variance(const Task &task)
{
    io_t *mean = p_mean(task);
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel for default(none) shared(task, mean, ans) schedule(static)
    for (int i = 0; i < task.wh; i++) {
        interm_t s = 0;
        for (int j = 0; j < task.n_images; j++) {
            auto delta = task.data[j * task.wh + i] - mean[i];
            s += delta * delta;
        }
        ans[i] = std::min<interm_t>(s / (task.n_images - 1), task.max_val);
    }
    delete[] mean;
    return ans;
}

io_t *p_standard_deviation(const Task &task)
{
    io_t *mean = p_mean(task);
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel for default(none) shared(task, mean, ans) schedule(static)
    for (int i = 0; i < task.wh; i++) {
        interm_t s = 0;
        for (int j = 0; j < task.n_images; j++) {
            auto delta = task.data[j * task.wh + i] - mean[i];
            s += delta * delta;
        }
        ans[i] = std::min<interm_t>((interm_t)std::sqrt(s / (task.n_images - 1)), task.max_val);
    }
    delete[] mean;
    return ans;
}


io_t *p_reduce(const Task &task, pReduction reduction)
{
    switch (reduction) {
        case pReduction::MEAN:
            return p_mean(task);
        case pReduction::MEDIAN:
            return p_median(task);
        case pReduction::SUMMATION:
            return p_summation(task);
        case pReduction::MAXIMUM:
            return p_maximum(task);
        case pReduction::MINIMUM:
            return p_minimum(task);
        case pReduction::RANGE:
            return p_range(task);
        case pReduction::VARIANCE:
            return p_variance(task);
        case pReduction::STANDARD_DEVIATION:
            return p_standard_deviation(task);
        default:
            return nullptr;
    }
}

} // namespace r2r
