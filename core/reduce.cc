#include "raw2raw.h"
#include <algorithm>
#include <numeric>
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

io_t *p_summation(const Task &task, int bits)
{
    interm_t limit = (1u<<bits) - 1;
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel for schedule(static)
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
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < task.wh; i++) {
        io_t m = 0;
        for (int j = 0; j < task.n_images; j++) {
            m = std::max(m, task.data[j * task.wh + i]);
        }
        ans[i] = m;
    }
    return ans;
}

io_t *p_minimum(const Task &task)
{
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel for schedule(static)
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
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < task.wh; i++) {
        io_t m = 0, M = 0;
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
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < task.wh; i++) {
        interm_t s = 0;
        for (int j = 0; j < task.n_images; j++) {
            auto delta = task.data[j * task.wh + i] - mean[i];
            s += delta * delta;
        }
        ans[i] = s / (task.n_images - 1);
    }
    delete[] mean;
    return ans;
}

io_t *p_standard_deviation(const Task &task)
{
    io_t *var = p_variance(task);
    io_t *ans = new io_t[task.wh];
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < task.wh; i++) {
        ans[i] = std::sqrt(var[i]);
    }
    delete[] var;
    return ans;
}

}
