#include "raw2raw.h"
#include <algorithm>

namespace r2r {
io_t *p_mean_remove_outlier(Task &task, int outliers)
{
    int outliers_per_side = outliers / 2;
    io_t *ans = new io_t[task.wh];
    int n = task.n_images;
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < task.wh; i++) {
        io_t *buf = new io_t[n];
        for (int j = 0; j < n; j++) {
            buf[j] = task.data[j * task.wh + i];
        }
        std::sort(buf, buf + n);
        interm_t s = 0;
        for (int j = outliers_per_side; j < n - outliers_per_side; j++) {
            s += buf[j];
        }
        ans[i] = s / (n - outliers);
        delete[] buf;
    }
    return ans;
}
}
