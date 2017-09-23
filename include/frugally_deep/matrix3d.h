// Copyright 2016, Tobias Hermann.
// https://github.com/Dobiasd/frugally-deep
// Distributed under the MIT License.
// (See accompanying LICENSE file or at
//  https://opensource.org/licenses/MIT)

#pragma once

#include "frugally_deep/typedefs.h"

#include "frugally_deep/matrix2d.h"
#include "frugally_deep/matrix3d_pos.h"
#include "frugally_deep/size3d.h"

#include <fplus/fplus.hpp>

#include <cassert>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace fd
{

class matrix3d
{
public:
    matrix3d(const size3d& shape, const float_vec& values) :
        size_(shape),
        values_(values)
    {
        assert(shape.volume() == values.size());
    }
    explicit matrix3d(const size3d& shape) :
        size_(shape),
        values_(shape.volume(), 0.0f)
    {
    }
    float_t get(const matrix3d_pos& pos) const
    {
        return values_[idx(pos)];
    }
    float_t get(std::size_t z, std::size_t y, std::size_t x) const
    {
        return get(matrix3d_pos(z, y, x));
    }
    void set(const matrix3d_pos& pos, float_t value)
    {
        values_[idx(pos)] = value;
    }
    void set(std::size_t z, std::size_t y, std::size_t x, float_t value)
    {
        set(matrix3d_pos(z, y, x), value);
    }
    const size3d& size() const
    {
        return size_;
    }
    const float_vec& as_vector() const
    {
        return values_;
    }
    void overwrite_values(const float_vec_const_it vs_begin,
        const float_vec_const_it vs_end)
    {
        assert(static_cast<std::size_t>(std::distance(vs_begin, vs_end)) ==
            size().volume());
        std::copy(vs_begin, vs_end, std::begin(values_));
    }

private:
    std::size_t idx(const matrix3d_pos& pos) const
    {
        return
            pos.z_ * size().height_ * size().width_ +
            pos.y_ * size().width_ +
            pos.x_;
    };
    size3d size_;
    float_vec values_;
};

typedef std::vector<matrix3d> matrix3ds;

inline std::string show_matrix3d(const matrix3d& m)
{
    std::string str;
    str += "[";
    for (std::size_t z = 0; z < m.size().depth_; ++z)
    {
        str += "[";
        for (std::size_t y = 0; y < m.size().height_; ++y)
        {
            for (std::size_t x = 0; x < m.size().width_; ++x)
            {
                str += fplus::show_float_fill_left<float_t>(' ', 8, 4,
                    m.get(z, y, x)) + ",";
            }
            str += "]\n";
        }
        str += "]\n";
    }
    str += "]";
    return str;
}

template <typename F>
matrix3d transform_matrix3d(F f, const matrix3d& m)
{
    return matrix3d(m.size(), fplus::transform(f, m.as_vector()));
}

inline matrix3d reshape_matrix3d(const matrix3d& in_vol, const size3d& out_size)
{
    return matrix3d(out_size, in_vol.as_vector());
}

inline matrix2d depth_slice(std::size_t z, const matrix3d& m)
{
    matrix2d result(shape2(m.size().height_, m.size().width_));
    for (std::size_t y = 0; y < m.size().height_; ++y)
    {
        for (std::size_t x = 0; x < m.size().width_; ++x)
        {
            result.set(y, x, m.get(z, y, x));
        }
    }
    return result;
}

inline matrix3d matrix3d_from_depth_slices(const std::vector<matrix2d>& ms)
{
    assert(!ms.empty());
    assert(
        fplus::all_the_same(
            fplus::transform([](const matrix2d& m) -> shape2
            {
                return m.size();
            },
            ms)));
    std::size_t height = ms.front().size().height_;
    std::size_t width = ms.front().size().width_;
    matrix3d m(size3d(ms.size(), height, width));
    for (std::size_t z = 0; z < m.size().depth_; ++z)
    {
        for (std::size_t y = 0; y < m.size().height_; ++y)
        {
            for (std::size_t x = 0; x < m.size().width_; ++x)
            {
                m.set(z, y, x, ms[z].get(y, x));
            }
        }
    }
    return m;
}

inline std::vector<matrix2d> matrix3d_to_depth_slices(const matrix3d& m)
{
    std::vector<matrix2d> ms(
        m.size().depth_,
        matrix2d(shape2(m.size().height_, m.size().width_)));

    for (std::size_t z = 0; z < m.size().depth_; ++z)
    {
        for (std::size_t y = 0; y < m.size().height_; ++y)
        {
            for (std::size_t x = 0; x < m.size().width_; ++x)
            {
                ms[z].set(y, x, m.get(z, y, x));
            }
        }
    }
    return ms;
}

inline matrix3d sparse_matrix3d(std::size_t step, const matrix3d& in)
{
    return matrix3d_from_depth_slices(
        fplus::transform(
            fplus::bind_1st_of_2(sparse_matrix2d, step),
            matrix3d_to_depth_slices(in)));
}

inline matrix3d matrix2d_to_matrix3d(const matrix2d& m)
{
    matrix3d result(size3d(1, m.size().height_, m.size().width_));
    for (std::size_t y = 0; y < m.size().height_; ++y)
    {
        for (std::size_t x = 0; x < m.size().width_; ++x)
        {
            result.set(0, y, x, m.get(y, x));
        }
    }
    return result;
}

inline std::pair<matrix3d_pos, matrix3d_pos> matrix3d_min_max_pos(
    const matrix3d& vol)
{
    matrix3d_pos result_min(0, 0, 0);
    matrix3d_pos result_max(0, 0, 0);
    float_t value_max = std::numeric_limits<float_t>::lowest();
    float_t value_min = std::numeric_limits<float_t>::max();
    for (std::size_t z = 0; z < vol.size().depth_; ++z)
    {
        for (std::size_t y = 0; y < vol.size().height_; ++y)
        {
            for (std::size_t x = 0; x < vol.size().width_; ++x)
            {
                auto current_value = vol.get(z, y, x);
                if (current_value > value_max)
                {
                    result_max = matrix3d_pos(z, y, x);
                    value_max = current_value;
                }
                if (current_value < value_min)
                {
                    result_min = matrix3d_pos(z, y, x);
                    value_min = current_value;
                }
            }
        }
    }
    return std::make_pair(result_min, result_max);
}

inline matrix3d_pos matrix3d_max_pos(const matrix3d& vol)
{
    return matrix3d_min_max_pos(vol).second;
}

inline matrix3d_pos matrix3d_min_pos(const matrix3d& vol)
{
    return matrix3d_min_max_pos(vol).second;
}

inline std::pair<float_t, float_t> matrix3d_min_max_value(const matrix3d& vol)
{
    auto min_max_positions = matrix3d_min_max_pos(vol);
    return std::make_pair(
        vol.get(min_max_positions.first), vol.get(min_max_positions.second));
}

inline float_t matrix3d_max_value(const matrix3d& m)
{
    return m.get(matrix3d_max_pos(m));
}

inline float_t matrix3d_min_value(const matrix3d& m)
{
    return m.get(matrix3d_min_pos(m));
}

inline matrix3d add_matrix3ds(const matrix3d& m1, const matrix3d& m2)
{
    assert(m1.size() == m2.size());
    return matrix3d(m1.size(), fplus::zip_with(std::plus<float_t>(),
        m1.as_vector(), m2.as_vector()));
}

inline matrix3d concatenate_matrix3ds(const matrix3ds& ts)
{
    const auto matrix3d_size_without_depth = [](const matrix3d& t) -> fd::shape2
    {
        return t.size().without_depth();
    };

    fd::assertion(fplus::all_the_same_on(matrix3d_size_without_depth, ts),
        "all tensors must have the same width and height");

    fd::assertion(!ts.empty(), "no tensors to concatenate");

    const auto matrix3d_size_depth = [](const matrix3d& t) -> std::size_t
    {
        return t.size().depth_;
    };
    const std::size_t depth_sum = fplus::sum(fplus::transform(
        matrix3d_size_depth, ts));

    const auto as_vector = [](const matrix3d& t) -> fd::float_vec
    {
        return t.as_vector();
    };
    return matrix3d(
        size3d(depth_sum, ts.front().size().height_, ts.front().size().width_),
        fplus::transform_and_concat(as_vector, ts));
}

inline matrix3d add_to_matrix3d_elems(const matrix3d& m, float_t x)
{
    return matrix3d(m.size(), fplus::transform([x](float_t e) -> float_t
    {
        return x + e;
    }, m.as_vector()));
}

inline matrix3d multiply_matrix3d_elems(const matrix3d& m, float_t x)
{
    return matrix3d(m.size(), fplus::transform([x](float_t e) -> float_t
    {
        return x * e;
    }, m.as_vector()));
}

inline matrix3d sum_matrix3ds(const std::vector<matrix3d>& ms)
{
    assert(!ms.empty());
    return fplus::fold_left_1(add_matrix3ds, ms);
}

inline matrix3d multiply_matrix3ds_elementwise(
    const matrix3d& m1, const matrix3d& m2)
{
    assert(m1.size() == m2.size());
    return matrix3d(m1.size(), fplus::zip_with(std::multiplies<float_t>(),
        m1.as_vector(), m2.as_vector()));
}

inline matrix3d multiply_matrix3d(const matrix3d& m, float_t factor)
{
    auto multiply_value_by_factor = [factor](const float_t x) -> float_t
    {
        return factor * x;
    };
    return transform_matrix3d(multiply_value_by_factor, m);
}

inline matrix3d divide_matrix3d(const matrix3d& m, float_t divisor)
{
    return multiply_matrix3d(m, 1 / divisor);
}

inline matrix3d mean_matrix3d(const std::vector<matrix3d>& ms)
{
    return divide_matrix3d(sum_matrix3ds(ms), static_cast<float_t>(ms.size()));
}

inline matrix3d sub_matrix3d(const matrix3d& m1, const matrix3d& m2)
{
    return add_matrix3ds(m1, multiply_matrix3d(m2, -1));
}

inline matrix3d abs_matrix3d_values(const matrix3d& m)
{
    return transform_matrix3d(fplus::abs<float_t>, m);
}

inline matrix3d abs_diff_matrix3ds(const matrix3d& m1, const matrix3d& m2)
{
    return abs_matrix3d_values(sub_matrix3d(m1, m2));
}

inline float_t matrix3d_sum_all_values(const matrix3d& m)
{
    return fplus::sum(m.as_vector());
}

inline float_t matrix3d_mean_value(const matrix3d& m)
{
    return
        matrix3d_sum_all_values(m) /
        static_cast<float_t>(m.size().volume());
}

inline matrix3d operator + (const matrix3d& lhs, const matrix3d& rhs)
{
    return add_matrix3ds(lhs, rhs);
}

inline matrix3d operator - (const matrix3d& lhs, const matrix3d& rhs)
{
    return sub_matrix3d(lhs, rhs);
}

inline matrix3d operator * (const matrix3d& m, float_t factor)
{
    return multiply_matrix3d(m, factor);
}

inline matrix3d operator / (const matrix3d& m, float_t divisor)
{
    return divide_matrix3d(m, divisor);
}

inline bool operator == (const matrix3d& a, const matrix3d& b)
{
    return a.size() == b.size() && a.as_vector() == b.as_vector();
}

inline bool operator != (const matrix3d& a, const matrix3d& b)
{
    return !(a == b);
}

inline matrix3d& operator += (matrix3d& lhs, const matrix3d& rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

inline matrix3d transpose_matrix3d(const matrix3d& m)
{
    return
        matrix3d_from_depth_slices(
            fplus::transform(
                transpose_matrix2d,
                matrix3d_to_depth_slices(m)));
}

inline matrix3d flip_matrix3d_horizontally(const matrix3d& m)
{
    return
        matrix3d_from_depth_slices(
            fplus::transform(
                flip_matrix2d_horizontally,
                matrix3d_to_depth_slices(m)));
}

inline matrix3d rotate_matrix3d_ccw(int step_cnt_90_deg, const matrix3d& m)
{
    return
        matrix3d_from_depth_slices(
            fplus::transform(
                fplus::bind_1st_of_2(rotate_matrix2d_ccw, step_cnt_90_deg),
                matrix3d_to_depth_slices(m)));
}

inline matrix3d flatten_matrix3d(const matrix3d& vol)
{
    float_vec values;
    values.reserve(vol.size().volume());
    for (std::size_t x = 0; x < vol.size().width_; ++x)
    {
        for (std::size_t y = 0; y < vol.size().height_; ++y)
        {
            for (std::size_t z = 0; z < vol.size().depth_; ++z)
            {
                values.push_back(vol.get(z, y, x));
            }
        }
    }
    return matrix3d(size3d(1, 1, values.size()), values);
}

} // namespace fd
