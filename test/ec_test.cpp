#define PUT(x) std::cout << #x "=" << (x) << std::endl
#define CYBOZU_TEST_DISABLE_AUTO_RUN
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>
#include <mie/gmp_util.hpp>
#include <mie/fp.hpp>
#include <mie/ec.hpp>
#include <mie/ecparam.hpp>
#include <time.h>

#if defined(_WIN64) || defined(__x86_64__)
//	#define USE_MONT_FP
#endif
#ifdef USE_MONT_FP
#include <mie/mont_fp.hpp>
typedef mie::MontFpT<3> Fp_3;
typedef mie::MontFpT<4> Fp_4;
typedef mie::MontFpT<6> Fp_6;
typedef mie::MontFpT<9> Fp_9;
#else
typedef mie::FpT<mie::Gmp> Fp_3;
typedef mie::FpT<mie::Gmp> Fp_4;
typedef mie::FpT<mie::Gmp> Fp_6;
typedef mie::FpT<mie::Gmp> Fp_9;
#endif

struct tagZn;
typedef mie::FpT<mie::Gmp, tagZn> Zn;

template<class Fp>
struct Test {
	typedef mie::EcT<Fp> Ec;
	const mie::EcParam& para;
	Test(const mie::EcParam& para)
		: para(para)
	{
		Fp::setModulo(para.p);
		Zn::setModulo(para.n);
		Ec::setParam(para.a, para.b);
//		CYBOZU_TEST_EQUAL(para.bitLen, Fp(-1).getBitLen());
	}
	void cstr() const
	{
		Ec O;
		CYBOZU_TEST_ASSERT(O.isZero());
		Ec P;
		Ec::neg(P, O);
		CYBOZU_TEST_EQUAL(P, O);
	}
	void ope() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Zn n = 0;
		CYBOZU_TEST_ASSERT(Ec::isValid(x, y));
		Ec P(x, y), Q, R, O;
		{
			Ec::neg(Q, P);
			CYBOZU_TEST_EQUAL(Q.x, P.x);
			CYBOZU_TEST_EQUAL(Q.y, -P.y);

			R = P + Q;
			CYBOZU_TEST_ASSERT(R.isZero());

			R = P + O;
			CYBOZU_TEST_EQUAL(R, P);
			R = O + P;
			CYBOZU_TEST_EQUAL(R, P);
		}

		{
			Ec::dbl(R, P);
			Ec R2 = P + P;
			CYBOZU_TEST_EQUAL(R, R2);
			{
				Ec P2 = P;
				Ec::dbl(P2, P2);
				CYBOZU_TEST_EQUAL(P2, R2);
			}
			Ec R3L = R2 + P;
			Ec R3R = P + R2;
			CYBOZU_TEST_EQUAL(R3L, R3R);
			{
				Ec RR = R2;
				RR = RR + P;
				CYBOZU_TEST_EQUAL(RR, R3L);
				RR = R2;
				RR = P + RR;
				CYBOZU_TEST_EQUAL(RR, R3L);
				RR = P;
				RR = RR + RR;
				CYBOZU_TEST_EQUAL(RR, R2);
			}
			Ec::power(R, P, 2);
			CYBOZU_TEST_EQUAL(R, R2);
			Ec R4L = R3L + R2;
			Ec R4R = R2 + R3L;
			CYBOZU_TEST_EQUAL(R4L, R4R);
			Ec::power(R, P, 5);
			CYBOZU_TEST_EQUAL(R, R4L);
		}
		{
			R = P;
			for (int i = 0; i < 10; i++) {
				R += P;
			}
			Ec R2;
			Ec::power(R2, P, 11);
			CYBOZU_TEST_EQUAL(R, R2);
		}
		Ec::power(R, P, n - 1);
		CYBOZU_TEST_EQUAL(R, -P);
		R += P; // Ec::power(R, P, n);
		CYBOZU_TEST_ASSERT(R.isZero());
	}

	void power() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y);
		Ec Q;
		Ec R;
		for (int i = 0; i < 100; i++) {
			Ec::power(Q, P, i);
			CYBOZU_TEST_EQUAL(Q, R);
			R += P;
		}
	}

	void neg_power() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y);
		Ec Q;
		Ec R;
		for (int i = 0; i < 100; i++) {
			Ec::power(Q, P, -i);
			CYBOZU_TEST_EQUAL(Q, R);
			R -= P;
		}
	}

	void power_fp() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y);
		Ec Q;
		Ec R;
		for (int i = 0; i < 100; i++) {
			Ec::power(Q, P, Zn(i));
			CYBOZU_TEST_EQUAL(Q, R);
			R += P;
		}
	}
	void binaryExpression() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y);
		Ec Q;
		{
			cybozu::BitVector bv;
			P.appendToBitVec(bv);
			Q.fromBitVec(bv);
			CYBOZU_TEST_EQUAL(P, Q);
		}
		{
			P = -P;
			cybozu::BitVector bv;
			P.appendToBitVec(bv);
			Q.fromBitVec(bv);
			CYBOZU_TEST_EQUAL(P, Q);
		}
		P.clear();
		{
			cybozu::BitVector bv;
			P.appendToBitVec(bv);
			Q.fromBitVec(bv);
			CYBOZU_TEST_EQUAL(P, Q);
		}
	}

	template<class F>
	void test(F f, const char *msg) const
	{
		const int N = 300000;
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y);
		Ec Q = P + P + P;
		clock_t begin = clock();
		for (int i = 0; i < N; i++) {
			f(Q, P, Q);
		}
		clock_t end = clock();
		printf("%s %.2fusec\n", msg, (end - begin) / double(CLOCKS_PER_SEC) / N * 1e6);
	}
	/*
		add 8.71usec -> 6.94
		sub 6.80usec -> 4.84
		dbl 9.59usec -> 7.75
		pos 2730usec -> 2153
	*/
	void bench() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y);
		Ec Q = P + P + P;
		CYBOZU_BENCH("add", Ec::add, Q, P, Q);
		CYBOZU_BENCH("sub", Ec::sub, Q, P, Q);
		CYBOZU_BENCH("dbl", Ec::dbl, P, P);
		Zn z("-3");
		CYBOZU_BENCH("pow", Ec::power, P, P, z);
	}
/*
Affine : sandy-bridge
add 3.17usec
sub 2.43usec
dbl 3.32usec
pow 905.00usec
Jacobi
add 2.34usec
sub 2.65usec
dbl 1.56usec
pow 499.00usec
*/
	void run() const
	{
		cstr();
		ope();
		power();
		neg_power();
		power_fp();
		binaryExpression();
#ifdef NDEBUG
		bench();
#endif
	}
private:
	Test(const Test&);
	void operator=(const Test&);
};

template<class Fp>
void test_sub(const mie::EcParam *para, size_t paraNum)
{
	for (size_t i = 0; i < paraNum; i++) {
		puts(para[i].name);
		Test<Fp>(para[i]).run();
	}
}

int g_partial = -1;

CYBOZU_TEST_AUTO(all)
{
#ifdef USE_MONT_FP
	puts("use MontFp");
#else
	puts("use GMP");
#endif
	if (g_partial & (1 << 3)) {
		const struct mie::EcParam para3[] = {
	//		mie::ecparam::p160_1,
			mie::ecparam::secp160k1,
			mie::ecparam::secp192k1,
			mie::ecparam::NIST_P192,
		};
		test_sub<Fp_3>(para3, CYBOZU_NUM_OF_ARRAY(para3));
	}

	if (g_partial & (1 << 4)) {
		const struct mie::EcParam para4[] = {
			mie::ecparam::secp224k1,
			mie::ecparam::secp256k1,
			mie::ecparam::NIST_P224,
			mie::ecparam::NIST_P256,
		};
		test_sub<Fp_4>(para4, CYBOZU_NUM_OF_ARRAY(para4));
	}

	if (g_partial & (1 << 6)) {
		const struct mie::EcParam para6[] = {
	//		mie::ecparam::secp384r1,
			mie::ecparam::NIST_P384,
		};
		test_sub<Fp_6>(para6, CYBOZU_NUM_OF_ARRAY(para6));
	}

	if (g_partial & (1 << 9)) {
		const struct mie::EcParam para9[] = {
	//		mie::ecparam::secp521r1,
			mie::ecparam::NIST_P521,
		};
		test_sub<Fp_9>(para9, CYBOZU_NUM_OF_ARRAY(para9));
	}
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		g_partial = -1;
	} else {
		g_partial = 0;
		for (int i = 1; i < argc; i++) {
			g_partial |= 1 << atoi(argv[i]);
		}
	}
	return cybozu::test::autoRun.run(argc, argv);
}
