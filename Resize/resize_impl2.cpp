#include <algorithm>
#include <cstdint>
#include "Common/except.h"
#include "Common/linebuffer.h"
#include "Common/pixel.h"
#include "Common/zfilter.h"
#include "filter.h"
#include "resize_impl2.h"

namespace zimg {;
namespace resize {;

namespace {;

int32_t unpack_pixel_u16(uint16_t x)
{
	return (int32_t)x + INT16_MIN;
}

uint16_t pack_pixel_u16(int32_t x)
{
	x = ((x + (1 << 13)) >> 14) - INT16_MIN;
	x = std::max(std::min(x, (int32_t)UINT16_MAX), (int32_t)0);

	return (uint16_t)x;
}

void resize_line_h_u16_c(const FilterContext &filter, const uint16_t *src, uint16_t *dst, unsigned left, unsigned right)
{
	for (unsigned j = left; j < right; ++j) {
		unsigned left = filter.left[j];
		int32_t accum = 0;

		for (unsigned k = 0; k < filter.filter_width; ++k) {
			int32_t coeff = filter.data_i16[j * filter.stride_i16 + k];
			int32_t x = unpack_pixel_u16(src[left + k]);

			accum += coeff * x;
		}

		dst[j] = pack_pixel_u16(accum);
	}
}

void resize_line_h_f32_c(const FilterContext &filter, const float *src, float *dst, unsigned left, unsigned right)
{
	for (unsigned j = left; j < right; ++j) {
		unsigned left = filter.left[j];
		float accum = 0;

		for (unsigned k = 0; k < filter.filter_width; ++k) {
			float coeff = filter.data[j * filter.stride + k];
			float x = src[left + k];

			accum += coeff * x;
		}

		dst[j] = accum;
	}
}

void resize_line_v_u16_c(const FilterContext &filter, const LineBuffer<uint16_t> &src, LineBuffer<uint16_t> &dst, unsigned i, unsigned left, unsigned right)
{
	const int16_t *filter_coeffs = &filter.data_i16[i * filter.stride_i16];
	unsigned top = filter.left[i];

	for (unsigned j = left; j < right; ++j) {
		int32_t accum = 0;

		for (unsigned k = 0; k < filter.filter_width; ++k) {
			int32_t coeff = filter_coeffs[k];
			int32_t x = unpack_pixel_u16(src[top + k][j]);

			accum += coeff * x;
		}

		dst[i][j] = pack_pixel_u16(accum);
	}
}

void resize_line_v_f32_c(const FilterContext &filter, const LineBuffer<float> &src, LineBuffer<float> &dst, unsigned i, unsigned left, unsigned right)
{
	const float *filter_coeffs = &filter.data[i * filter.stride];
	unsigned top = filter.left[i];

	for (unsigned j = left; j < right; ++j) {
		float accum = 0;

		for (unsigned k = 0; k < filter.filter_width; ++k) {
			float coeff = filter_coeffs[k];
			float x = src[top + k][j];

			accum += coeff * x;
		}

		dst[i][j] = accum;
	}
}


class ResizeImplH_C : public ZimgFilter {
	FilterContext m_filter;
	PixelType m_type;
	bool m_is_sorted;
public:
	ResizeImplH_C(const FilterContext &filter, PixelType type) :
		m_filter(filter),
		m_type{ type },
		m_is_sorted{ std::is_sorted(m_filter.left.begin(), m_filter.left.end()) }
	{
		if (m_type != PixelType::WORD && m_type != PixelType::FLOAT)
			throw ZimgUnsupportedError{ "pixel type not supported" };
	}

	zimg_filter_flags get_flags() const override
	{
		zimg_filter_flags flags{};

		flags.same_row = true;
		flags.entire_row = !m_is_sorted;

		return flags;
	}

	pair_unsigned get_required_col_range(unsigned left, unsigned right) const override
	{
		if (m_is_sorted) {
			unsigned col_left = m_filter.left[left];
			unsigned col_right = m_filter.left[right - 1] + m_filter.filter_width;

			return{ col_left, col_right };
		} else {
			return{ 0, m_filter.input_width };
		}
	}

	void process(void *, const zimg_image_buffer *src, const zimg_image_buffer *dst, void *, unsigned i, unsigned left, unsigned right) const override
	{
		if (m_type == PixelType::WORD) {
			LineBuffer<uint16_t> src_buf{ reinterpret_cast<uint16_t *>(src->data[0]), right, (unsigned)src->stride[0], src->mask[0] };
			LineBuffer<uint16_t> dst_buf{ reinterpret_cast<uint16_t *>(dst->data[0]), right, (unsigned)dst->stride[0], dst->mask[0] };

			resize_line_h_u16_c(m_filter, src_buf[i], dst_buf[i], left, right);
		} else {
			LineBuffer<float> src_buf{ reinterpret_cast<float *>(src->data[0]), right, (unsigned)src->stride[0], src->mask[0] };
			LineBuffer<float> dst_buf{ reinterpret_cast<float *>(dst->data[0]), right, (unsigned)dst->stride[0], dst->mask[0] };

			resize_line_h_f32_c(m_filter, src_buf[i], dst_buf[i], left, right);
		}
	}
};

class ResizeImplV_C : public ZimgFilter {
	FilterContext m_filter;
	PixelType m_type;
	bool m_is_sorted;
public:
	ResizeImplV_C(const FilterContext &filter, PixelType type) : 
		m_filter(filter),
		m_type{ type },
		m_is_sorted{ std::is_sorted(m_filter.left.begin(), m_filter.left.end()) }
	{
		if (m_type != PixelType::WORD && m_type != PixelType::FLOAT)
			throw ZimgUnsupportedError{ "pixel type not supported" };
	}

	zimg_filter_flags get_flags() const override
	{
		zimg_filter_flags flags{};

		flags.entire_row = !m_is_sorted;
		flags.entire_plane = !m_is_sorted;

		return flags;
	}

	pair_unsigned get_required_row_range(unsigned i) const override
	{
		if (m_is_sorted) {
			unsigned row = m_filter.left[i];

			return{ row, row + m_filter.filter_width };
		} else {
			return{ 0, m_filter.input_width };
		}
	}

	unsigned get_max_buffering() const override
	{
		return m_is_sorted ? m_filter.filter_width : -1;
	}

	void process(void *, const zimg_image_buffer *src, const zimg_image_buffer *dst, void *, unsigned i, unsigned left, unsigned right) const override
	{
		if (m_type == PixelType::WORD) {
			LineBuffer<uint16_t> src_buf{ reinterpret_cast<uint16_t *>(src->data[0]), right, (unsigned)src->stride[0], src->mask[0] };
			LineBuffer<uint16_t> dst_buf{ reinterpret_cast<uint16_t *>(dst->data[0]), right, (unsigned)dst->stride[0], dst->mask[0] };

			resize_line_v_u16_c(m_filter, src_buf, dst_buf, i, left, right);
		} else {
			LineBuffer<float> src_buf{ reinterpret_cast<float *>(src->data[0]), right, (unsigned)src->stride[0], src->mask[0] };
			LineBuffer<float> dst_buf{ reinterpret_cast<float *>(dst->data[0]), right, (unsigned)dst->stride[0], dst->mask[0] };

			resize_line_v_f32_c(m_filter, src_buf, dst_buf, i, left, right);
		}
	}
};

} // namespace


IZimgFilter *create_resize_impl2(const Filter &f, PixelType type, bool horizontal, unsigned src_dim, unsigned dst_dim, unsigned height,
                                 double shift, double subwidth, CPUClass cpu)
{
	FilterContext filter_ctx = compute_filter(f, src_dim, dst_dim, shift, subwidth);

	if (horizontal)
		return new ResizeImplH_C{ filter_ctx, type };
	else
		return new ResizeImplV_C{ filter_ctx, type };
}

} // namespace resize
} // namespace zimg