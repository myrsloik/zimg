#ifdef ZIMG_X86

#include <cmath>
#include "common/cpuinfo.h"
#include "common/pixel.h"
#include "common/x86/cpuinfo_x86.h"
#include "graph/image_filter.h"
#include "depth/depth.h"
#include "depth/dither.h"

#include "gtest/gtest.h"
#include "graph/filter_validator.h"

namespace {

void test_case(const zimg::PixelFormat &pixel_in, const zimg::PixelFormat &pixel_out, const char * const expected_sha1[3], double expected_snr)
{
	const unsigned w = 640;
	const unsigned h = 480;
	const zimg::depth::DitherType dither = zimg::depth::DitherType::RANDOM;

	if (!zimg::query_x86_capabilities().avx2) {
		SUCCEED() << "avx2 not available, skipping";
		return;
	}

	auto filter_c = zimg::depth::create_dither(dither, w, h, pixel_in, pixel_out, zimg::CPUClass::NONE);
	auto filter_avx2 = zimg::depth::create_dither(dither, w, h, pixel_in, pixel_out, zimg::CPUClass::X86_AVX2);

	FilterValidator validator{ filter_avx2.get(), w, h, pixel_in };
	validator.set_sha1(expected_sha1)
	         .set_ref_filter(filter_c.get(), expected_snr)
	         .validate();
}

} // namespace


TEST(DitherAVX2Test, test_ordered_dither_b2b)
{
	zimg::PixelFormat pixel_in{ zimg::PixelType::BYTE, 8, true, false };
	zimg::PixelFormat pixel_out{ zimg::PixelType::BYTE, 1, true, false };

	const char *expected_sha1[3] = {
		"cd90fbe6881101c82387fbe4e630b7da215ab190"
	};

	test_case(pixel_in, pixel_out, expected_sha1, INFINITY);
}

TEST(DitherAVX2Test, test_ordered_dither_b2w)
{

	zimg::PixelFormat pixel_in{ zimg::PixelType::BYTE, 8, true, false };
	zimg::PixelFormat pixel_out{ zimg::PixelType::WORD, 9, true, false };

	const char *expected_sha1[3] = {
		"551e95e2ab3a6cb21f5e8e532a70257b5a7ce5d7"
	};

	test_case(pixel_in, pixel_out, expected_sha1, INFINITY);
}

TEST(DitherAVX2Test, test_ordered_dither_w2b)
{
	zimg::PixelFormat pixel_in = zimg::PixelType::WORD;
	zimg::PixelFormat pixel_out = zimg::PixelType::BYTE;

	const char *expected_sha1[3] = {
		"6da260ba1fec3eda99d83739ad3ae9eaeba15df2"
	};

	test_case(pixel_in, pixel_out, expected_sha1, INFINITY);
}

TEST(DitherAVX2Test, test_ordered_dither_w2w)
{
	zimg::PixelFormat pixel_in{ zimg::PixelType::WORD, 16, false, false };
	zimg::PixelFormat pixel_out{ zimg::PixelType::WORD, 10, false, false };

	const char *expected_sha1[3] = {
		"094b16505a3030cda9283f1d2b0ebf6ed84d6535"
	};

	test_case(pixel_in, pixel_out, expected_sha1, INFINITY);
}

TEST(DitherAVX2Test, test_ordered_dither_h2b)
{
	zimg::PixelFormat pixel_in = zimg::PixelType::HALF;
	zimg::PixelFormat pixel_out = zimg::PixelType::BYTE;

	const char *expected_sha1[3] = {
		"30ef22fa882d6f4bc0dbb7224a1dffdedf108734"
	};

	test_case(pixel_in, pixel_out, expected_sha1, INFINITY);
}

TEST(DitherAVX2Test, test_ordered_dither_h2w)
{
	zimg::PixelFormat pixel_in = zimg::PixelType::HALF;
	zimg::PixelFormat pixel_out = zimg::PixelType::WORD;

	const char *expected_sha1[3] = {
		"661fad247819b453a77fffb32aa4da95c0e7024e"
	};

	test_case(pixel_in, pixel_out, expected_sha1, INFINITY);
}

TEST(DitherAVX2Test, test_ordered_dither_f2b)
{
	zimg::PixelFormat pixel_in = zimg::PixelType::FLOAT;
	zimg::PixelFormat pixel_out = zimg::PixelType::BYTE;

	const char *expected_sha1[3] = {
		"6bf40dea8fb17f035be0e4fa14a303b3cde45dc5"
	};

	test_case(pixel_in, pixel_out, expected_sha1, INFINITY);
}

TEST(DitherAVX2Test, test_ordered_dither_f2w)
{
	zimg::PixelFormat pixel_in = zimg::PixelType::FLOAT;
	zimg::PixelFormat pixel_out = zimg::PixelType::WORD;

	const char *expected_sha1[3] = {
		"b902d8029f198214e39ca10582dbfe0714172a1d"
	};

	// The use of FMA changes the rounding of the result at 16-bits.
	test_case(pixel_in, pixel_out, expected_sha1, 120.0);
}

#endif // ZIMG_X86
