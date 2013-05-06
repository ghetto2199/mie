#include <cybozu/test.hpp>
#include <mie/fp.hpp>
#include <mie/gmp_util.hpp>
#include <time.h>
#include <mie/mont_fp.hpp>

typedef mie::FpT<mie::Gmp> Zn;

template<class T>
mpz_class toGmp(const T& x)
{
	std::string str = x.toStr();
	mpz_class t;
	mie::Gmp::fromStr(t, str);
	return t;
}

template<class T>
T fromGmp(const mpz_class& x)
{
	std::string str;
	mie::Gmp::toStr(str, x);
	T t;
	t.fromStr(str);
	return t;
}

template<size_t N>
struct Test {
	typedef mie::MontFpT<N> Fp;
	mpz_class m;
	void run(const char *p)
	{
		Fp::setModulo(p);
		m = p;
		cstr();
		fromStr();
		stream();
		conv();
		compare();
		modulo();
		ope();
		power();
		neg_power();
		power_Zn();
		setRaw();
		set64bit();
		getRaw();
//		bench();
	}
	void cstr()
	{
		const struct {
			const char *str;
			int val;
		} tbl[] = {
			{ "0", 0 },
			{ "1", 1 },
			{ "123", 123 },
			{ "0x123", 0x123 },
			{ "0b10101", 21 },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			// string cstr
			Fp x(tbl[i].str);
			CYBOZU_TEST_EQUAL(x, tbl[i].val);

			// int cstr
			Fp y(tbl[i].val);
			CYBOZU_TEST_EQUAL(y, x);

			// copy cstr
			Fp z(x);
			CYBOZU_TEST_EQUAL(z, x);

			// assign int
			Fp w;
			w = tbl[i].val;
			CYBOZU_TEST_EQUAL(w, x);

			// assign self
			Fp u;
			u = w;
			CYBOZU_TEST_EQUAL(u, x);

			// conv
			std::ostringstream os;
			os << tbl[i].val;

			std::string str;
			x.toStr(str);
			CYBOZU_TEST_EQUAL(str, os.str());
		}
		CYBOZU_TEST_EXCEPTION_MESSAGE(Fp("-123"), cybozu::Exception, "fromStr");
	}

	void fromStr()
	{
		const struct {
			const char *in;
			int out;
			int base;
		} tbl[] = {
			{ "100", 100, 0 }, // set base = 10 if base = 0
			{ "100", 4, 2 },
			{ "100", 256, 16 },
			{ "0b100", 4, 0 },
			{ "0b100", 4, 2 },
			{ "0b100", 4, 16 }, // ignore base
			{ "0x100", 256, 0 },
			{ "0x100", 256, 2 }, // ignore base
			{ "0x100", 256, 16 },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			Fp x;
			x.fromStr(tbl[i].in, tbl[i].base);
			CYBOZU_TEST_EQUAL(x, tbl[i].out);
		}
	}

	void stream()
	{
		const struct {
			const char *in;
			int out10;
			int out16;
		} tbl[] = {
			{ "100", 100, 256 }, // set base = 10 if base = 0
			{ "0b100", 4, 4 },
			{ "0x100", 256, 256 }, // ignore base
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			{
				std::istringstream is(tbl[i].in);
				Fp x;
				is >> x;
				CYBOZU_TEST_EQUAL(x, tbl[i].out10);
			}
			{
				std::istringstream is(tbl[i].in);
				Fp x;
				is >> std::hex >> x;
				CYBOZU_TEST_EQUAL(x, tbl[i].out16);
			}
		}
	}

	void conv()
	{
		const char *bin = "0b100100011010001010110011110001001000000010010001101000101011001111000100100000001001000110100010101100111100010010000";
		const char *hex = "0x123456789012345678901234567890";
		const char *dec = "94522879687365475552814062743484560";
		Fp b(bin);
		Fp h(hex);
		Fp d(dec);
		CYBOZU_TEST_EQUAL(b, h);
		CYBOZU_TEST_EQUAL(b, d);

		std::string str;
		b.toStr(str, 2);
		CYBOZU_TEST_EQUAL(str, bin);
		b.toStr(str);
		CYBOZU_TEST_EQUAL(str, dec);
		b.toStr(str, 16);
		CYBOZU_TEST_EQUAL(str, hex);
	}

	void compare()
	{
		const struct {
			int lhs;
			int rhs;
			int cmp;
		} tbl[] = {
			{ 0, 0, 0 },
			{ 1, 0, 1 },
			{ 0, 1, -1 },
			{ -1, 0, 1 }, // m-1, 0
			{ 0, -1, -1 }, // 0, m-1
			{ 123, 456, -1 },
			{ 456, 123, 1 },
			{ 5, 5, 0 },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const Fp x(tbl[i].lhs);
			const Fp y(tbl[i].rhs);
			const int cmp = tbl[i].cmp;
			if (cmp == 0) {
				CYBOZU_TEST_EQUAL(x, y);
			} else {
				CYBOZU_TEST_ASSERT(x != y);
			}
		}
	}

	void modulo()
	{
		std::ostringstream ms;
		ms << m;

		std::string str;
		Fp::getModulo(str);
		CYBOZU_TEST_EQUAL(str, ms.str());
	}

	void ope()
	{
		const struct {
			mpz_class x;
			mpz_class y;
			mpz_class add; // x + y
			mpz_class sub; // x - y
			mpz_class mul; // x * y
		} tbl[] = {
			{ 0, 1, 1, m - 1, 0 },
			{ 9, 5, 14, 4, 45 },
			{ 10, 13, 23, m - 3, 130 },
			{ 2000, m - 1000, 1000, 3000, m - 2000000 },
			{ m - 12345, m - 9999, m - (12345 + 9999), m - 12345 + 9999, 12345 * 9999 },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const Fp x(fromGmp<Fp>(tbl[i].x));
			const Fp y(fromGmp<Fp>(tbl[i].y));
			Fp z;
			Fp::add(z, x, y);
			CYBOZU_TEST_EQUAL(toGmp(z), tbl[i].add);
			Fp::sub(z, x, y);
			CYBOZU_TEST_EQUAL(toGmp(z), tbl[i].sub);
			Fp::mul(z, x, y);
			CYBOZU_TEST_EQUAL(toGmp(z), tbl[i].mul);

			Fp r;
			Fp::inv(r, y);
			Fp::mul(z, z, r);
			CYBOZU_TEST_EQUAL(toGmp(z), tbl[i].x);

			z = x + y;
			CYBOZU_TEST_EQUAL(toGmp(z), tbl[i].add);
			z = x - y;
			CYBOZU_TEST_EQUAL(toGmp(z), tbl[i].sub);
			z = x * y;
			CYBOZU_TEST_EQUAL(toGmp(z), tbl[i].mul);

			z = x / y;
			z *= y;
			CYBOZU_TEST_EQUAL(toGmp(z), tbl[i].x);
		}
	}

	void power()
	{
		Fp x, y, z;
		x = 12345;
		z = 1;
		for (int i = 0; i < 100; i++) {
			Fp::power(y, x, i);
			CYBOZU_TEST_EQUAL(y, z);
			z *= x;
		}
	}

	void neg_power()
	{
		Fp x, y, z;
		x = 12345;
		z = 1;
		Fp rx = 1 / x;
		for (int i = 0; i < 100; i++) {
			Fp::power(y, x, -i);
			CYBOZU_TEST_EQUAL(y, z);
			z *= rx;
		}
	}

	void power_Zn()
	{
		Fp x, y, z;
		x = 12345;
		z = 1;
		for (int i = 0; i < 100; i++) {
			Fp::power(y, x, Zn(i));
			CYBOZU_TEST_EQUAL(y, z);
			z *= x;
		}
	}

	void setRaw()
	{
		// QQQ
#if 0
		char b1[] = { 0x56, 0x34, 0x12 };
		Fp x;
		x.setRaw(b1, 3);
		CYBOZU_TEST_EQUAL(x, 0x123456);
		int b2[] = { 0x12, 0x34 };
		x.setRaw(b2, 2);
		CYBOZU_TEST_EQUAL(x, Fp("0x3400000012"));
#endif
	}

	void set64bit()
	{
		const struct {
			const char *p;
			uint64_t i;
		} tbl[] = {
			{ "0x1234567812345678", uint64_t(0x1234567812345678ull) },
			{ "0xaaaaaaaaaaaaaaaa", uint64_t(0xaaaaaaaaaaaaaaaaull) },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			Fp x(tbl[i].p);
			Fp y(tbl[i].i);
			CYBOZU_TEST_EQUAL(x, y);
		}
	}

	void getRaw()
	{
		const struct {
			const char *s;
			uint32_t v[4];
			size_t vn;
		} tbl[] = {
			{ "0", { 0, 0, 0, 0 }, 1 },
			{ "1234", { 1234, 0, 0, 0 }, 1 },
			{ "0xaabbccdd12345678", { 0x12345678, 0xaabbccdd, 0, 0 }, 2 },
			{ "0x11112222333344445555666677778888", { 0x77778888, 0x55556666, 0x33334444, 0x11112222 }, 4 },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			mpz_class x(tbl[i].s);
			const size_t bufN = 8;
			uint32_t buf[bufN];
			size_t n = mie::Gmp::getRaw(buf, bufN, x);
			CYBOZU_TEST_EQUAL(n, tbl[i].vn);
			for (size_t j = 0; j < n; j++) {
				CYBOZU_TEST_EQUAL(buf[j], tbl[i].v[j]);
			}
		}
	}
	void bench()
	{
		const int C = 5000000;
		Fp x("12345678901234567900342423332197");
		double base = 0;
		{
			clock_t begin = clock();
			for (int i = 0; i < C; i++) {
				x += x;
			}
			clock_t end = clock();
			base = (end - begin) / double(CLOCKS_PER_SEC) / C * 1e9;
			printf("add %7.2fnsec %s\n", base, x.toStr().c_str());
		}
		{
			clock_t begin = clock();
			for (int i = 0; i < C; i++) {
				x += x;
			}
			clock_t end = clock();
			double t = (end - begin) / double(CLOCKS_PER_SEC) / C * 1e9;
			printf("add %7.2fnsec(x%5.2f) %s\n", t, t / base, x.toStr().c_str());
		}
		{
			Fp y("0x7ffffffffffffffffffffffe26f2fc170f69466a74defd8d");
			clock_t begin = clock();
			for (int i = 0; i < C; i++) {
				x -= y;
			}
			clock_t end = clock();
			double t = (end - begin) / double(CLOCKS_PER_SEC) / C * 1e9;
			printf("sub %7.2fnsec(x%5.2f) %s\n", t, t / base, x.toStr().c_str());
		}
		{
			clock_t begin = clock();
			for (int i = 0; i < C; i++) {
				x *= x;
			}
			clock_t end = clock();
			double t = (end - begin) / double(CLOCKS_PER_SEC) / C * 1e9;
			printf("mul %7.2fnsec(x%5.2f) %s\n", t, t / base, x.toStr().c_str());
		}
		{
			Fp y("0xfffffffffffffffffffffe26f2fc170f69466a74defd8d");
			clock_t begin = clock();
			for (int i = 0; i < C; i++) {
				x /= y;
			}
			clock_t end = clock();
			double t = (end - begin) / double(CLOCKS_PER_SEC) / C * 1e9;
			printf("div %7.2fnsec(x%5.2f) %s\n", t, t / base, x.toStr().c_str());
		}
	}
};

CYBOZU_TEST_AUTO(all)
{
	Test<3> test3;
	const char *tbl3[] = {
		"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFAC73",
		"0x100000000000000000001B8FA16DFAB9ACA16B6B3",
		"0x10000000000000000000000000000000000000007",
		"1461501637330902918203683518218126812711137002561",
		"0xfffffffffffffffffffffffffffffffffffffffeffffee37",
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl3); i++) {
		test3.run(tbl3[i]);
	}
}