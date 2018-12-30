 /*
Minetest
Copyright (C) 2010-2014 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "test.h"

#include <cmath>
#include "util/numeric.h"
#include "exceptions.h"
#include "noise.h"

class TestRandom : public TestBase {
public:
	TestRandom() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestRandom"; }

	void runTests(IGameDef *gamedef);

	void testPseudoRandom();
	void testPseudoRandomRange();
	void testPcgRandom();
	void testPcgRandomRange();
	void testPcgRandomBytes();
	void testPcgRandomNormalDist();

	static const int expected_pseudorandom_results[256];
	static const u32 expected_pcgrandom_results[256];
	static const u8 expected_pcgrandom_bytes_result[24];
	static const u8 expected_pcgrandom_bytes_result2[24];
};

static TestRandom g_test_instance;

void TestRandom::runTests(IGameDef *gamedef)
{
	TEST(testPseudoRandom);
	TEST(testPseudoRandomRange);
	TEST(testPcgRandom);
	TEST(testPcgRandomRange);
	TEST(testPcgRandomBytes);
	TEST(testPcgRandomNormalDist);
}

////////////////////////////////////////////////////////////////////////////////

void TestRandom::testPseudoRandom()
{
	PseudoRandom pr(814538);

	for (u32 i = 0; i != 256; i++)
		UASSERTEQ(int, pr.next(), expected_pseudorandom_results[i]);
}


void TestRandom::testPseudoRandomRange()
{
	PseudoRandom pr((int)time(NULL));

	EXCEPTION_CHECK(PrngException, pr.range(2000, 6000));
	EXCEPTION_CHECK(PrngException, pr.range(5, 1));

	for (u32 i = 0; i != 32768; i++) {
		int min = (pr.next() % 3000) - 500;
		int max = (pr.next() % 3000) - 500;
		if (min > max)
			SWAP(int, min, max);

		int randval = pr.range(min, max);
		UASSERT(randval >= min);
		UASSERT(randval <= max);
	}
}


void TestRandom::testPcgRandom()
{
	PcgRandom pr(814538, 998877);

	for (u32 i = 0; i != 256; i++)
		UASSERTEQ(u32, pr.next(), expected_pcgrandom_results[i]);
}


void TestRandom::testPcgRandomRange()
{
	PcgRandom pr((int)time(NULL));

	EXCEPTION_CHECK(PrngException, pr.range(5, 1));

	// Regression test for bug 3027
	pr.range(pr.RANDOM_MIN, pr.RANDOM_MAX);

	for (u32 i = 0; i != 32768; i++) {
		int min = (pr.next() % 3000) - 500;
		int max = (pr.next() % 3000) - 500;
		if (min > max)
			SWAP(int, min, max);

		int randval = pr.range(min, max);
		UASSERT(randval >= min);
		UASSERT(randval <= max);
	}
}


void TestRandom::testPcgRandomBytes()
{
	char buf[32];
	PcgRandom r(1538, 877);

	memset(buf, 0, sizeof(buf));
	r.bytes(buf + 5, 23);
	UASSERT(memcmp(buf + 5, expected_pcgrandom_bytes_result,
		sizeof(expected_pcgrandom_bytes_result)) == 0);

	memset(buf, 0, sizeof(buf));
	r.bytes(buf, 17);
	UASSERT(memcmp(buf, expected_pcgrandom_bytes_result2,
		sizeof(expected_pcgrandom_bytes_result2)) == 0);
}


void TestRandom::testPcgRandomNormalDist()
{
	static const int max = 120;
	static const int min = -120;
	static const int num_trials = 20;
	static const u32 num_samples = 61000;
	s32 bins[max - min + 1];
	memset(bins, 0, sizeof(bins));

	PcgRandom r(486179 + (int)time(NULL));

	for (u32 i = 0; i != num_samples; i++) {
		s32 randval = r.randNormalDist(min, max, num_trials);
		UASSERT(randval <= max);
		UASSERT(randval >= min);
		bins[randval - min]++;
	}

	// Note that here we divide variance by the number of trials;
	// this is because variance is a biased estimator.
	int range      = (max - min + 1);
	float mean     = (max + min) / 2;
	float variance = ((range * range - 1) / 12) / num_trials;
	float stddev   = std::sqrt(variance);

	static const float prediction_intervals[] = {
		0.68269f, // 1.0
		0.86639f, // 1.5
		0.95450f, // 2.0
		0.98758f, // 2.5
		0.99730f, // 3.0
	};

	//// Simple normality test using the 68-95-99.7% rule
	for (u32 i = 0; i != ARRLEN(prediction_intervals); i++) {
		float deviations = i / 2.f + 1.f;
		int lbound = myround(mean - deviations * stddev);
		int ubound = myround(mean + deviations * stddev);
		UASSERT(lbound >= min);
		UASSERT(ubound <= max);

		int accum = 0;
		for (int j = lbound; j != ubound; j++)
			accum += bins[j - min];

		float actual = (float)accum / num_samples;
		UASSERT(std::fabs(actual - prediction_intervals[i]) < 0.02f);
	}
}


const int TestRandom::expected_pseudorandom_results[256] = {
	0x02fa, 0x60d5, 0x6c10, 0x606b, 0x098b, 0x5f1e, 0x4f56, 0x3fbd, 0x77af,
	0x4fe9, 0x419a, 0x6fe1, 0x177b, 0x6858, 0x36f8, 0x6d83, 0x14fc, 0x2d62,
	0x1077, 0x23e2, 0x041b, 0x7a7e, 0x5b52, 0x215d, 0x682b, 0x4716, 0x47e3,
	0x08c0, 0x1952, 0x56ae, 0x146d, 0x4b4f, 0x239f, 0x3fd0, 0x6794, 0x7796,
	0x7be2, 0x75b7, 0x5691, 0x28ee, 0x2656, 0x40c0, 0x133c, 0x63cd, 0x2aeb,
	0x518f, 0x7dbc, 0x6ad8, 0x736e, 0x5b05, 0x160b, 0x589f, 0x6f64, 0x5edc,
	0x092c, 0x0a39, 0x199e, 0x1927, 0x562b, 0x2689, 0x3ba3, 0x366f, 0x46da,
	0x4e49, 0x0abb, 0x40a1, 0x3846, 0x40db, 0x7adb, 0x6ec1, 0x6efa, 0x01cc,
	0x6335, 0x4352, 0x72fb, 0x4b2d, 0x509a, 0x257e, 0x2f7d, 0x5891, 0x2195,
	0x6107, 0x5269, 0x56e3, 0x4849, 0x38f7, 0x2791, 0x04f2, 0x4e05, 0x78ff,
	0x6bae, 0x50b3, 0x74ad, 0x31af, 0x531e, 0x7d56, 0x11c9, 0x0b5e, 0x405e,
	0x1e15, 0x7f6a, 0x5bd3, 0x6649, 0x71b4, 0x3ec2, 0x6ab4, 0x520e, 0x6ad6,
	0x287e, 0x10b8, 0x18f2, 0x7107, 0x46ea, 0x1d85, 0x25cc, 0x2689, 0x35c1,
	0x3065, 0x6237, 0x3edd, 0x23d9, 0x6fb5, 0x37a1, 0x3211, 0x526a, 0x4b09,
	0x23f1, 0x58cc, 0x2e42, 0x341f, 0x5e16, 0x3d1a, 0x5e8c, 0x7a82, 0x4635,
	0x2bf8, 0x6577, 0x3603, 0x1daf, 0x539f, 0x2e91, 0x6bd8, 0x42d3, 0x7a93,
	0x26e3, 0x5a91, 0x6c67, 0x1b66, 0x3ac7, 0x18bf, 0x20d8, 0x7153, 0x558d,
	0x7262, 0x653d, 0x417d, 0x3ed3, 0x3117, 0x600d, 0x6d04, 0x719c, 0x3afd,
	0x6ba5, 0x17c5, 0x4935, 0x346c, 0x5479, 0x6ff6, 0x1fcc, 0x1054, 0x3f14,
	0x6266, 0x3acc, 0x3b77, 0x71d8, 0x478b, 0x20fa, 0x4e46, 0x7e77, 0x5554,
	0x3652, 0x719c, 0x072b, 0x61ad, 0x399f, 0x621d, 0x1bba, 0x41d0, 0x7fdc,
	0x3e6c, 0x6a2a, 0x5253, 0x094e, 0x0c10, 0x3f43, 0x73eb, 0x4c5f, 0x1f23,
	0x12c9, 0x0902, 0x5238, 0x50c0, 0x1b77, 0x3ffd, 0x0124, 0x302a, 0x26b9,
	0x3648, 0x30a6, 0x1abc, 0x3031, 0x4029, 0x6358, 0x6696, 0x74e8, 0x6142,
	0x4284, 0x0c00, 0x7e50, 0x41e3, 0x3782, 0x79a5, 0x60fe, 0x2d15, 0x3ed2,
	0x7f70, 0x2b27, 0x6366, 0x5100, 0x7c44, 0x3ee0, 0x4e76, 0x7d34, 0x3a60,
	0x140e, 0x613d, 0x1193, 0x268d, 0x1e2f, 0x3123, 0x6d61, 0x4e0b, 0x51ce,
	0x13bf, 0x58d4, 0x4f43, 0x05c6, 0x4d6a, 0x7eb5, 0x2921, 0x2c36, 0x1c89,
	0x63b9, 0x1555, 0x1f41, 0x2d9f,
};

const u32 TestRandom::expected_pcgrandom_results[256] = {
	0x48c593f8, 0x054f59f5, 0x0d062dc1, 0x23852a23, 0x7fbbc97b, 0x1f9f141e,
	0x364e6ed8, 0x995bba58, 0xc9307dc0, 0x73fb34c4, 0xcd8de88d, 0x52e8ce08,
	0x1c4a78e4, 0x25c0882e, 0x8a82e2e0, 0xe3bc3311, 0xb8068d42, 0x73186110,
	0x19988df4, 0x69bd970b, 0x7214728c, 0x0aee320c, 0x2a5a536c, 0xaf48d715,
	0x00bce504, 0xd2b8f548, 0x520df366, 0x96d8fff5, 0xa1bb510b, 0x63477049,
	0xb85990b7, 0x7e090689, 0x275fb468, 0x50206257, 0x8bab4f8a, 0x0d6823db,
	0x63faeaac, 0x2d92deeb, 0x2ba78024, 0x0d30f631, 0x338923a0, 0xd07248d8,
	0xa5db62d3, 0xddba8af6, 0x0ad454e9, 0x6f0fd13a, 0xbbfde2bf, 0x91188009,
	0x966b394d, 0xbb9d2012, 0x7e6926cb, 0x95183860, 0x5ff4c59b, 0x035f628a,
	0xb67085ef, 0x33867e23, 0x68d1b887, 0x2e3298d7, 0x84fd0650, 0x8bc91141,
	0x6fcb0452, 0x2836fee9, 0x2e83c0a3, 0xf1bafdc5, 0x9ff77777, 0xfdfbba87,
	0x527aebeb, 0x423e5248, 0xd1756490, 0xe41148fa, 0x3361f7b4, 0xa2824f23,
	0xf4e08072, 0xc50442be, 0x35adcc21, 0x36be153c, 0xc7709012, 0xf0eeb9f2,
	0x3d73114e, 0x1c1574ee, 0x92095b9c, 0x1503d01c, 0xd6ce0677, 0x026a8ec1,
	0x76d0084d, 0x86c23633, 0x36f75ce6, 0x08fa7bbe, 0x35f6ff2a, 0x31cc9525,
	0x2c1a35e6, 0x8effcd62, 0xc782fa07, 0x8a86e248, 0x8fdb7a9b, 0x77246626,
	0x5767723f, 0x3a78b699, 0xe548ce1c, 0x5820f37d, 0x148ed9b8, 0xf6796254,
	0x32232c20, 0x392bf3a2, 0xe9af6625, 0xd40b0d88, 0x636cfa23, 0x6a5de514,
	0xc4a69183, 0xc785c853, 0xab0de901, 0x16ae7e44, 0x376f13b5, 0x070f7f31,
	0x34cbc93b, 0xe6184345, 0x1b7f911f, 0x631fbe4b, 0x86d6e023, 0xc689b518,
	0x88ef4f7c, 0xddf06b45, 0xc97f18d4, 0x2aaee94b, 0x45694723, 0x6db111d2,
	0x91974fce, 0xe33e29e2, 0xc5e99494, 0x8017e02b, 0x3ebd8143, 0x471ffb80,
	0xc0d7ca1b, 0x4954c860, 0x48935d6a, 0xf2d27999, 0xb93d608d, 0x40696e90,
	0x60b18162, 0x1a156998, 0x09b8bbab, 0xc80a79b6, 0x8adbcfbc, 0xc375248c,
	0xa584e2ea, 0x5b46fe11, 0x58e84680, 0x8a8bc456, 0xd668b94f, 0x8b9035be,
	0x278509d4, 0x6663a140, 0x81a9817a, 0xd4f9d3cf, 0x6dc5f607, 0x6ae04450,
	0x694f22a4, 0x1d061788, 0x2e39ad8b, 0x748f4db2, 0xee569b52, 0xd157166d,
	0xdabc161e, 0xc8d50176, 0x7e3110e5, 0x9f7d033b, 0x128df67f, 0xb0078583,
	0xa3a75d26, 0xc1ad8011, 0x07dd89ec, 0xef04f456, 0x91bf866c, 0x6aac5306,
	0xdd5a1573, 0xf73ff97a, 0x4e1186ad, 0xb9680680, 0xc8894515, 0xdc95a08e,
	0xc894fd8e, 0xf84ade15, 0xd787f8c1, 0x40dcecca, 0x1b24743e, 0x1ce6ab23,
	0x72321653, 0xb80fbaf7, 0x1bcf099b, 0x1ff26805, 0x78f66c8e, 0xf93bf51a,
	0xfb0c06fe, 0xe50d48cf, 0x310947e0, 0x1b78804a, 0xe73e2c14, 0x8deb8381,
	0xe576122a, 0xe5a8df39, 0x42397c5e, 0xf5503f3c, 0xbe3dbf8d, 0x1b360e5c,
	0x9254caaf, 0x7a9f6744, 0x6d4144fa, 0xd77c65fe, 0x44ca7b12, 0xf58a4c00,
	0x159500d0, 0x92769857, 0x7134fdd4, 0xa3fea693, 0xbd044831, 0xeded39a1,
	0xe4570204, 0xaea37f2f, 0x9a302971, 0x620f8402, 0x1d2f3e5e, 0xf9c2f49c,
	0x738e813a, 0xb3c92251, 0x7ecba63b, 0xbe7eebc7, 0xf800267c, 0x3fdeb760,
	0xf12d5e7d, 0x5a18dce1, 0xb35a539c, 0xe565f057, 0x2babf38c, 0xae5800ad,
	0x421004dd, 0x6715acb6, 0xff529b64, 0xd520d207, 0x7cb193e7, 0xe9b18e4c,
	0xfd2a8a59, 0x47826ae3, 0x56ba43f8, 0x453b3d99, 0x8ae1675f, 0xf66f5c34,
	0x057a6ac1, 0x010769e4, 0xa8324158, 0x410379a5, 0x5dfc8c97, 0x72848afe,
	0x59f169e5, 0xe32acb78, 0x5dfaa9c4, 0x51bb956a,
};

const u8 TestRandom::expected_pcgrandom_bytes_result[24] = {
	0xf3, 0x79, 0x8f, 0x31, 0xac, 0xd9, 0x34, 0xf8, 0x3c, 0x6e, 0x82, 0x37,
	0x6b, 0x4b, 0x77, 0xe3, 0xbd, 0x0a, 0xee, 0x22, 0x79, 0x6e, 0x40, 0x00,
};

const u8 TestRandom::expected_pcgrandom_bytes_result2[24] = {
	0x47, 0x9e, 0x08, 0x3e, 0xd4, 0x21, 0x2d, 0xf6, 0xb4, 0xb1, 0x9d, 0x7a,
	0x60, 0x02, 0x5a, 0xb2, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
