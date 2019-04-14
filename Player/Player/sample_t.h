///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sample_t class for SSE4.1 based stereo sample processing (double, double).
// based on Ralph Borsons (revivalizer) blog about simd for synths:
//		http://revivalizer.dk/blog/2013/07/26/art-of-softsynth-development-simd-parallelization/
//		http://revivalizer.dk/blog/2013/07/28/art-of-softsynth-development-using-sse-in-c-plus-plus-without-the-hassle/
//		https://github.com/revivalizer
// extended and fitted to my needs for 64klang
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SAMPLE_T_
#define _SAMPLE_T_

#include <intrin.h>
#include <smmintrin.h>

#define S_SKIP_UNUSED

_MM_ALIGN16 union sample_t
{	
	char	c[16];
	short	s[8];
	int		i[4];
	__int64	l[2];
	double  d[2];
	__m128d pd;
	__m128	ps;
	__m128i	pi;

	// init (must call this)
	static void init();

	// construction
	inline sample_t() {}
	inline explicit sample_t(const double x)			: pd(_mm_set1_pd(x)) {}
	inline explicit sample_t(double x1, double x2)		: pd(_mm_set_pd(x2, x1)) {}
	inline sample_t(double* ptr)						: pd(_mm_load_pd(ptr)) {}
	inline sample_t(sample_t* ptr)						: pd(_mm_load_pd(ptr->d)) {}
	inline sample_t(const __m128d& in)					: pd(in) {}
	inline sample_t(const __m128& in)					: ps(in) {}
	inline sample_t(const __m128i& in)					: pi(in) {}
	inline sample_t(const sample_t& x)					: pd(x.pd) {}
	inline explicit sample_t(int x)						{ i[0] = x; i[1] = x; i[2] = i[3] = 0; }

	// load/store/zero
	inline static sample_t zero()						{ return sample_t(_mm_setzero_pd()); }
	inline static sample_t load(const double* ptr)		{ return sample_t(_mm_load_pd(ptr)); }
	inline static sample_t loadu(const double* ptr)		{ return sample_t(_mm_loadu_pd(ptr)); }
	inline void store(double* ptr)						{ _mm_store_pd(ptr, pd); }
	inline void storeu(double* ptr)						{ _mm_storeu_pd(ptr, pd); }

	// assignment
	inline sample_t& operator=(const sample_t& x)		{ pd = x.pd; return *this; }

	// arithmetic	 
	inline sample_t& operator+=(const sample_t& x);
	inline sample_t& operator-=(const sample_t& x);
	inline sample_t& operator*=(const sample_t& x);
	inline sample_t& operator/=(const sample_t& x);
	inline sample_t& operator&=(const sample_t& x);
	inline sample_t& operator|=(const sample_t& x);
	inline sample_t& operator^=(const sample_t& x);

	// comparison
	inline sample_t operator==(const sample_t& x);
	inline sample_t operator!=(const sample_t& x);
	inline sample_t operator<(const sample_t& x);
	inline sample_t operator<=(const sample_t& x);
	inline sample_t operator>(const sample_t& x);
	inline sample_t operator>=(const sample_t& x);
};

// constants
enum
{
	// integers manually inserted to SC[]
	S_EXP2_OFFSET = 0,
	S_RAND_SEED,
	S_ALLBITS,

	// double lists loop inserted to SC[]
	S_RAND_MUL,
	S_RAND_NORM,
	S_SIGN_MASK,
	S_M1_0,
	S_0_125,
	S_0_5,
	S_0_75,
	S_1_0,
	S_2_0,
	S_4_0,
	S_8_0,
	S_32_0,
	S_128_0,
	S_32768_0,
	S_6_0,
	S_10_0,
	S_60_0,
	S_440_0,	
	S_0_225,
	S_0_2,
	S_0_1,
	S_0_01,
	S_0_001,
	S_1_O_12,
		
	S_SAMPLERATE,

	CHANNELROOT_EFC,
	CHANNELROOT_EFT,
	ADSR_SCALE,
	ADSR_LOG2,
	ADSR_ZERO,
	NOISEGEN_B0,
	NOISEGEN_W0,
	NOISEGEN_B1,
	NOISEGEN_W1,
	NOISEGEN_B2,
	NOISEGEN_W2,
	NOISEGEN_W3,

	ENVFOLLOWER_SCALE,

	DELAY_APC,

	DCFILTER_C,

	REVERB_RSF,
	REVERB_RSO,
	REVERB_ALLP0,
	REVERB_ALLP1,
	REVERB_ALLP2,
	REVERB_ALLP3,
	REVERB_COMB0,
	REVERB_COMB1,
	REVERB_COMB2,
	REVERB_COMB3,
	REVERB_COMB4,
	REVERB_COMB5,
	REVERB_COMB6,
	REVERB_COMB7,
	REVERB_MAXCOMBSIZE,

	BOWED_VELOFFSET,
	BOWED_FILTERB0,
	BOWED_FILTERA1,

	// constants for exp, log and sin implementations
	S_LOG2E,	
	S_LOG210,
	S_LN0_01,
	S_EXP2_0,
	S_EXP2_1,
	S_EXP2_2,
	S_EXP2_3,
	S_EXP2_4,
	S_EXP2_5,
	S_EXP2_6,
	S_LOG2_0,
	S_LOG2_1,
	S_LOG2_2,
	S_LOG2_3,
	S_LOG2_4,
#ifndef S_SKIP_UNUSED
	S_ATAN_0,
	S_ATAN_1,
	S_ATAN_2,
	S_ATAN_3,
	S_ATAN_4,
	S_ATAN_5,
	S_ATAN_6,
	S_ATAN_7,
#endif
	S_PI,
	S_2PI,
	S_PI2,
	S_B,
	S_C,

	S_MAX_CONSTANTS
};
extern sample_t SC[S_MAX_CONSTANTS];

// operator overloads
inline sample_t		operator+(const sample_t& a, const sample_t& b)						{ return sample_t(_mm_add_pd(a.pd, b.pd)); }
inline sample_t		operator-(const sample_t& a, const sample_t& b)						{ return sample_t(_mm_sub_pd(a.pd, b.pd)); }
inline sample_t		operator*(const sample_t& a, const sample_t& b)						{ return sample_t(_mm_mul_pd(a.pd, b.pd)); }
inline sample_t		operator/(const sample_t& a, const sample_t& b)						{ return sample_t(_mm_div_pd(a.pd, b.pd)); }
inline sample_t		operator&(const sample_t& a, const sample_t& b)						{ return sample_t(_mm_and_pd(a.pd, b.pd)); }
inline sample_t		operator|(const sample_t& a, const sample_t& b)						{ return sample_t(_mm_or_pd(a.pd, b.pd)); }
inline sample_t		operator^(const sample_t& a, const sample_t& b)						{ return sample_t(_mm_xor_pd(a.pd, b.pd)); }
inline sample_t		operator!(const sample_t& a)										{ return sample_t(_mm_xor_pd(a.pd, SC[S_ALLBITS].pd)); }
inline sample_t		operator-(const sample_t& a)										{ return sample_t(_mm_xor_pd(a.pd, SC[S_SIGN_MASK].pd)); }

// arithmetic
inline sample_t&	sample_t::operator+=(const sample_t& x)								{ return *this=*this+x; }
inline sample_t&	sample_t::operator-=(const sample_t& x)								{ return *this=*this-x; }
inline sample_t&	sample_t::operator*=(const sample_t& x)								{ return *this=*this*x; }
inline sample_t&	sample_t::operator/=(const sample_t& x)								{ return *this=*this/x; }
inline sample_t&	sample_t::operator&=(const sample_t& x)								{ return *this=*this&x; }
inline sample_t&	sample_t::operator|=(const sample_t& x)								{ return *this=*this|x; }
inline sample_t&	sample_t::operator^=(const sample_t& x)								{ return *this=*this^x; }

// comparison
inline sample_t		sample_t::operator==(const sample_t& x)								{ return sample_t(_mm_cmpeq_pd (pd, x.pd)); }
inline sample_t		sample_t::operator!=(const sample_t& x)								{ return sample_t(_mm_cmpneq_pd(pd, x.pd)); }
inline sample_t		sample_t::operator< (const sample_t& x)								{ return sample_t(_mm_cmplt_pd (pd, x.pd)); }
inline sample_t		sample_t::operator<=(const sample_t& x)								{ return sample_t(_mm_cmple_pd (pd, x.pd)); }
inline sample_t		sample_t::operator> (const sample_t& x)								{ return sample_t(_mm_cmpgt_pd (pd, x.pd)); }
inline sample_t		sample_t::operator>=(const sample_t& x)								{ return sample_t(_mm_cmpge_pd (pd, x.pd)); }
// bitwise casts
inline __m128i		s_asInt(const sample_t& x)											{ return x.pi; }
inline sample_t		s_asSample(__m128i x)												{ return sample_t(_mm_castsi128_pd(x)); }  
// conversion
inline __m128i		s_toInt(const sample_t& x)											{ return _mm_cvtpd_epi32(x.pd); }
inline sample_t		s_toSample(__m128i x)												{ return sample_t(_mm_cvtepi32_pd(x)); }
// sample specific operations
inline sample_t		s_dupleft(const sample_t& x)										{ return sample_t(_mm_movelh_ps(x.ps, x.ps)); }
inline sample_t		s_dupright(const sample_t& x)										{ return sample_t(_mm_movehl_ps(x.ps, x.ps)); }

// other math functions
inline sample_t		s_min	(const sample_t& a, const sample_t& b)						{ return sample_t(_mm_min_pd(a.pd, b.pd)); }
inline sample_t		s_max	(const sample_t& a, const sample_t& b)						{ return sample_t(_mm_max_pd(a.pd, b.pd)); }
inline sample_t		s_sqrt	(const sample_t& x)											{ return sample_t(_mm_sqrt_pd(x.pd)); }
inline sample_t		s_mad	(const sample_t& x, const sample_t& y, const sample_t& o)	{ return sample_t(_mm_add_pd(_mm_mul_pd(x.pd, y.pd), o.pd)); }
inline sample_t		s_ifthen(const sample_t& c, const sample_t& x, const sample_t& y)	{ return sample_t(_mm_blendv_pd(y.pd, x.pd, c.pd)); }
inline sample_t		s_floor	(const sample_t& x)											{ return sample_t(_mm_round_pd(x.pd, _MM_FROUND_FLOOR)); }
inline sample_t		s_ceil  (const sample_t& x)											{ return sample_t(_mm_round_pd(x.pd, _MM_FROUND_CEIL)); }
inline sample_t		s_round (const sample_t& x)											{ return sample_t(_mm_round_pd(x.pd, _MM_FROUND_TO_NEAREST_INT)); }
inline sample_t		s_abs	(const sample_t& x)											{ return sample_t(_mm_andnot_pd(SC[S_SIGN_MASK].pd, x.pd)); }
inline sample_t		s_neg	(const sample_t& x)											{ return sample_t(_mm_xor_pd(x.pd, SC[S_SIGN_MASK].pd)); }
inline sample_t		s_cpsign(const sample_t& x, const sample_t& y)						{ return sample_t(_mm_or_pd(_mm_andnot_pd(SC[S_SIGN_MASK].pd, x.pd), _mm_and_pd(SC[S_SIGN_MASK].pd, y.pd))); }
inline sample_t		s_mod	(const sample_t& x, const sample_t& m)						{ return x-(s_floor(x/m)*m); }
inline sample_t		s_clamp (const sample_t& x, const sample_t& u, const sample_t& l)	{ return s_max(l, s_min(u, x)); }
inline sample_t		s_lerp(const sample_t& x, const sample_t& y, const sample_t& f)		{ return (y - x)*f + x; }
inline sample_t		s_shuffle(const sample_t& x, const sample_t& y)						{ return sample_t(_mm_shuffle_pd(x.pd, y.pd, 1)); }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern sample_t	__fastcall	s_rand();
extern sample_t	__fastcall	s_sin(const sample_t& x);
extern sample_t	__fastcall	s_exp2(const sample_t& x);
extern sample_t	__fastcall	s_log2(const sample_t& x);
inline sample_t		s_cos	(const sample_t& x)											{ return s_sin(x+SC[S_PI2]); }
inline sample_t		s_exp	(const sample_t& x)											{ return s_exp2(x*SC[S_LOG2E]); }
inline sample_t		s_exp10	(const sample_t& x)											{ return s_exp2(x*SC[S_LOG210]); }
inline sample_t		s_pow	(const sample_t& x, const sample_t& y)						{ return s_exp2(y*s_log2(x)); }
inline sample_t		s_cerp	(const sample_t& x, const sample_t& y, const sample_t& f)	{ return s_lerp(x, y, (SC[S_1_0]-s_cos(f*SC[S_PI]))*SC[S_0_5]); }
inline sample_t		s_equalp(const sample_t& x, const sample_t& y, const sample_t& f)	{ return x*s_sin(SC[S_PI2] + SC[S_PI2] * f) + y*s_sin(SC[S_PI2] * f); }
inline sample_t		s_db2lin(const sample_t& x)											{ return s_exp2(x / SC[S_6_0]); }
inline sample_t		s_lin2db(const sample_t& x)											{ return s_log2(s_abs(x) + SC[CHANNELROOT_EFT]) * SC[S_6_0]; }

#ifndef S_SKIP_UNUSED
extern sample_t		s_atan	(const sample_t& x);
extern sample_t		s_tan(const sample_t& x);
extern sample_t		s_cosh(const sample_t& x);
extern sample_t		s_sinh(const sample_t& x);
extern sample_t		s_tanh(const sample_t& x);
#endif

#endif
