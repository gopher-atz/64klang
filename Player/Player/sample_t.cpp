#include "sample_t.h"
#include "SynthNode.h"

// the actual sample_t array
_MM_ALIGN16 sample_t SC[S_MAX_CONSTANTS];

// the raw data for the array

// integer constants
_MM_ALIGN16 
int		_EXP2_OFFSET[4]		= { 1023, 1023,    0,    0 };
int		_RAND_SEED[4]		= {    1,    0,31337,    0 };
//S_ALLBITS manually set below

// TODO: experiment with float and see if precision errors occur (can save about 100 byte when using floats)
// need to take car of _RAND_MUL there, it needs to be adjusted, if at all possible (might need to manually assign it

// constants source for conversion in sample_t array
double ConstantList[S_MAX_CONSTANTS] =
{
 0, // 3 dummies for 3 integers above to keep indices from enum in sync with this array
 0,
 0,

 7.908508792981e-320, 																		//	 double	_RAND_MUL			=
 1.0/(65536.0*32768.0),																		//	 double	_RAND_NORM			=
-0.,																						//	 double	_SIGN_MASK			=
-1.0,																						//	 double	_M1_0				=
 0.125,																						//	 double	_0_125				=
 0.5,																						//	 double	_0_5				=
 0.75,																						//	 double _0_75
 1.0,																						//	 double	_1_0				=
 2.0,																						//	 double	_2_0				=
 4.0,																						//	 double	_4_0				=
 8.0,																						//   double _8_0
 32.0,																						//   double _32_0
 128.0,																						//	 double	_128_0				=
 32768.0,																					//	 double _32768_0			=
 6.0,																						//	 double	_6_0				=
 10.0,																						//	 double	_10_0				=
 60.0,																						//	 double	_60_0				=
 440.0,																						//	 double	_440_0				=
 0.22499990463256836,																		//	 double	_0_225				=
 0.19999992847442627,																		//	 double	_0_2				=
 0.10000002384185791,																		//	 double	_0_1				=
 0.0099999979138374329,																		//	 double	_0_01				=
 0.0010000001639127731,																		//	 double	_0_001				=
 0.083333313465118408,																		//	 double	_1_O_12				= 
 (double)SYNTH_SAMPLERATE,																	//	 double _SAMPLERATE
																							
 0.99994754791259766,																		//	 double _CHANNELROOT_EFC	=
 1.0000003385357559e-005,																	//	 double _CHANNELROOT_EFT	=
 418381.75000000000,																		//	 double _ADSR_SCALE			=
 -10.0,																						//	 double _ADSR_LOG2
 0.0000152587890625,																		//	 double _ADSR_ZERO
 0.99765014648437500,																		//	 double _NOISEGEN_B0		=
 0.099045991897583008,																		//	 double _NOISEGEN_W0		=
 0.96299982070922852,																		//	 double _NOISEGEN_B1		=
 0.29651640355587006,																		//	 double _NOISEGEN_W1		=
 0.57000017166137695,																		//	 double _NOISEGEN_B2		=
 1.0526914596557617,																		//	 double _NOISEGEN_W2		=
 0.18479990959167480,																		//	 double _NOISEGEN_W3		=

 5644.7968750000000,																		//	 double _ENVFOLLOWER_SCALE	=

 0.61799955368041992,																		//	 double _DELAY_APC		=
  
 0.99432373046875,																			//	 double _DCFILTER_C		=

 0.28125,																					//	 double _REVERB_RSF	=
 0.703125,																					//	 double _REVERB_RSO	=
 
 225.0,																						//	 double _REVERB_ALLP0	=
 341.0,
 441.0,
 556.0,																						
  
 1116.0,																					//	 double _REVERB_COMB0	=
 1188.0,
 1277.0,
 1356.0,
 1422.0,
 1491.0,
 1557.0,
 1617.0,
  
 1640.0,																					//	 double _REVERB_MAXCOMBSIZE

 0.027235999703407288,																		//	 double _BOWED_VELOFFSET
 0.33249998092651367,																		//	 double _BOWED_FILTERB0
 0.64999961853027344,																		//	 double _BOWED_FILTERA1
 
 // constants for exp, log and sin implementations (zeroed out the least significant 2 bytes, original values in comment)
 1.4426950408815173, //1.4426950408889634073599,											//	 double	_LOG2E				=
 3.3219280948687810, //3.3219280948873623478703194294894,									//   double _LOG210				=
-4.6051701859687455 , //- 4.6051701859880913680359829093687,								//	 double _LN0_01				=
 0.00015465324084118492, //0.000154653240842602623787395880898,								//	 double	_EXP2_0				=
 0.0013395291543787380, //0.00133952915439234389712105060319,								//	 double	_EXP2_1				=
 0.0096180399117429261, //0.0096180399118156827664944870552,								//	 double	_EXP2_2				=
 0.055503406540083233, //0.055503406540531310853149866446,									//	 double	_EXP2_3				=
 0.24022651101404335, //0.240226511015459465468737123346,									//	 double	_EXP2_4				=
 0.69314720007241704, //0.69314720007380208630542805293,									//	 double	_EXP2_5				=
 0.99999999997089617, //0.99999999997182023878745628977,									//	 double	_EXP2_6				=
 0.41098153827988426, //0.410981538282433293325329456838,									//	 double	_LOG2_0				=
 0.40215548317064531, //0.402155483172044562892705980539,									//	 double	_LOG2_1				=
 0.57755014627036871, //0.57755014627178237959721643293,									//	 double	_LOG2_2				=
 0.96178780600166647, //0.96178780600659929206930296869,									//	 double	_LOG2_3				=
 2.8853901278343983, //2.88539012786343587248965772685,										//	 double	_LOG2_4				=
#ifndef S_SKIP_UNUSED
 0.00282363896258175373077393,																//	 double	_ATAN_0				=
-0.0159569028764963150024414,																//	 double	_ATAN_1				=
 0.0425049886107444763183594,																//	 double	_ATAN_2				=
-0.0748900920152664184570312,																//	 double	_ATAN_3				=
 0.106347933411598205566406,																//	 double	_ATAN_4				=
-0.142027363181114196777344,																//	 double	_ATAN_5				=
 0.199926957488059997558594,																//	 double	_ATAN_6				=
-0.333331018686294555664062,																//	 double	_ATAN_7				=
#endif
 3.1415926535846666, //3.1415926535897932384626433832795,									//	 double	_PI					=
 6.2831853071693331, //2.0*3.1415926535897932384626433832795,								//	 double	_2PI				=
 1.5707963267923333, //1.57079632679489661923,												//	 double	_PI2				=
 1.2732395447237650, //4.0 / 3.1415926535897932384626433832795,								//	 double	_B					=
-0.40528473456652137, //- 4.0 / (3.1415926535897932384626433832795*3.1415926535897932384626433832795),					//	 double	_C					=
 };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void sample_t::init()
{
	SC[0] = sample_t(*((const __m128i*)(_EXP2_OFFSET)));
	SC[1] = sample_t(*((const __m128i*)(_RAND_SEED)));
	SC[2] = SC[0] == SC[0];

	for (int i = 3; i < S_MAX_CONSTANTS; i++)
		SC[i] = sample_t((double)ConstantList[i]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sample_t __fastcall s_rand()
{	
	SC[S_RAND_SEED].pi = _mm_mul_epi32(SC[S_RAND_SEED].pi, SC[S_RAND_MUL].pi);
	return sample_t(_mm_mul_pd(_mm_cvtepi32_pd(_mm_shuffle_epi32(SC[S_RAND_SEED].pi, _MM_SHUFFLE(1,3,0,2))), SC[S_RAND_NORM].pd));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sample_t __fastcall s_sin(const sample_t& x)
{
	// keep in range 0..2pi
	sample_t mp = s_mod(x, SC[S_2PI]);
	sample_t p = s_ifthen(mp > SC[S_PI], mp-SC[S_2PI], mp);
	// input range now -pi .. pi (as needed)
	sample_t y = SC[S_B]*p + SC[S_C]*p*s_abs(p);
	return SC[S_0_225]*(y*s_abs(y) - y) + y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ECOEFFICIENTS 6
sample_t __fastcall s_exp2(const sample_t& x)
{    
	__m128i k = s_toInt(x);
	sample_t p = x - s_toSample(k);

#if ECOEFFICIENTS == 6
	sample_t r(SC[S_EXP2_0]);
	r = s_mad(r, p, SC[S_EXP2_1]);
	r = s_mad(r, p, SC[S_EXP2_2]);
	r = s_mad(r, p, SC[S_EXP2_3]);
	r = s_mad(r, p, SC[S_EXP2_4]);
	r = s_mad(r, p, SC[S_EXP2_5]);
	r = s_mad(r, p, SC[S_EXP2_6]);
#endif
#if ECOEFFICIENTS == 9
	sample_t r(1.02072375599725694063203809188e-7);
	r = s_mad(r, p, sample_t(1.32573274434801314145133004073e-6));
	r = s_mad(r, p, sample_t(0.0000152526647170731944840736190013));
	r = s_mad(r, p, sample_t(0.000154034441925859828261898614555));
	r = s_mad(r, p, sample_t(0.00133335582175770747495287552557));
	r = s_mad(r, p, sample_t(0.0096181291794939392517233403183));
	r = s_mad(r, p, sample_t(0.055504108664525029438908798685));
	r = s_mad(r, p, sample_t(0.240226506957026959772247598695));
	r = s_mad(r, p, sample_t(0.6931471805599487321347668143));
	r = s_mad(r, p, sample_t(1.00000000000000942892870993489));
#endif
#if ECOEFFICIENTS == 11
	sample_t r(4.45623165388261696886670014471e-10);
	r = s_mad(r, p, sample_t(7.0733589360775271430968224806e-9));
	r = s_mad(r, p, sample_t(1.01780540270960163558119510246e-7));
	r = s_mad(r, p, sample_t(1.3215437348041505269462510712e-6));
	r = s_mad(r, p, sample_t(0.000015252733849766201174247690629));
	r = s_mad(r, p, sample_t(0.000154035304541242555115696403795));
	r = s_mad(r, p, sample_t(0.00133335581463968601407096905671));
	r = s_mad(r, p, sample_t(0.0096181291075949686712855561931));
	r = s_mad(r, p, sample_t(0.055504108664821672870565883052));
	r = s_mad(r, p, sample_t(0.240226506959101382690753994082));
	r = s_mad(r, p, sample_t(0.69314718055994530864272481773));
	r = s_mad(r, p, sample_t(0.9999999999999999978508676375));
#endif


	/* 2^k; */
	k = _mm_add_epi32(k, SC[S_EXP2_OFFSET].pi);
	k = _mm_slli_epi32(k, 20);
	k = _mm_shuffle_epi32(k, _MM_SHUFFLE(1,3,0,2));

	/* a * 2^k. */
	return r * s_asSample(k);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LCOEFFICIENTS 4
sample_t __fastcall s_log2(const sample_t& x)
{
	// rescale
	sample_t minexp(_mm_set_epi32(0, (2 - (-1021)), 0, (2 - (-1021))));
	sample_t ilogb_x(_mm_shuffle_epi32(_mm_sub_epi64(_mm_srli_epi64(x.pi, 52), minexp.pi), _MM_SHUFFLE(1,3,2,0)));
	// mask out exponent with minimum exponent
	sample_t o(_mm_slli_epi64(minexp.pi, 52));
	sample_t a(_mm_srli_epi64(SC[S_ALLBITS].pi, 12));
	sample_t p = (x & a) | o;
	// pole
	sample_t y = (p - SC[S_1_0]) / (p + SC[S_1_0]);
	sample_t y2 = y*y;

#if LCOEFFICIENTS == 4
	sample_t r(SC[S_LOG2_0]);
	r = s_mad(r, y2, SC[S_LOG2_1]);
	r = s_mad(r, y2, SC[S_LOG2_2]);
	r = s_mad(r, y2, SC[S_LOG2_3]);
	r = s_mad(r, y2, SC[S_LOG2_4]);
#endif
#if LCOEFFICIENTS == 7
	sample_t r = sample_t(0.293251364683280430617251942017);
	r = mad(r, y2, sample_t(0.201364223624519571276587631354));
	r = mad(r, y2, sample_t(0.264443947645547871780098560836));
	r = mad(r, y2, sample_t(0.320475051320227723946459855458));
	r = mad(r, y2, sample_t(0.412202612052105347480086431555));
	r = mad(r, y2, sample_t(0.57707794741938820005328259256));
	r = mad(r, y2, sample_t(0.96179669445173881282808321929));
	r = mad(r, y2, sample_t(2.88539008177676567117601117274));
#endif
#if LCOEFFICIENTS == 9
	sample_t r = sample_t(0.259935726478127940817401224248);
	r = mad(r, y2, sample_t(0.140676370079882918464564658472));
	r = mad(r, y2, sample_t(0.196513478841924000569879320851));
	r = mad(r, y2, sample_t(0.221596471338300882039273355617));
	r = mad(r, y2, sample_t(0.262327298560598641020007602127));
	r = mad(r, y2, sample_t(0.320598261015170101859472461613));
	r = mad(r, y2, sample_t(0.412198595799726905825871956187));
	r = mad(r, y2, sample_t(0.57707801621733949207376840932));
	r = mad(r, y2, sample_t(0.96179669392666302667713134701));
	r = mad(r, y2, sample_t(2.88539008177792581277410991327));
#endif

	r *= y;
	// undo rescaling
	r += s_toSample(ilogb_x.pi);

	return r;
}

#ifndef S_SKIP_UNUSED

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ACOEFFICIENTS 6
sample_t s_atan(const sample_t& x)
{   
	sample_t q1 = x;
	sample_t s = s_abs(x);
	
	sample_t q0 = s > SC[S_1_0];
	s = s_ifthen(q0, SC[S_1_0] / s, s);    
	sample_t t = s * s;
	
#if ACOEFFICIENTS == 6
	sample_t u(SC[S_ATAN_0]);
	u = s_mad(u, t, SC[S_ATAN_1]);
	u = s_mad(u, t, SC[S_ATAN_2]);
	u = s_mad(u, t, SC[S_ATAN_3]);
	u = s_mad(u, t, SC[S_ATAN_4]);
	u = s_mad(u, t, SC[S_ATAN_5]);
	u = s_mad(u, t, SC[S_ATAN_6]);
	u = s_mad(u, t, SC[S_ATAN_7]);
#else
	sample_t u(-1.88796008463073496563746e-05);
	u = s_mad(u, t, sample_t(0.000209850076645816976906797));
	u = s_mad(u, t, sample_t(-0.00110611831486672482563471));
	u = s_mad(u, t, sample_t(0.00370026744188713119232403));
	u = s_mad(u, t, sample_t(-0.00889896195887655491740809));
	u = s_mad(u, t, sample_t(0.016599329773529201970117));
	u = s_mad(u, t, sample_t(-0.0254517624932312641616861));
	u = s_mad(u, t, sample_t(0.0337852580001353069993897));
	u = s_mad(u, t, sample_t(-0.0407629191276836500001934));
	u = s_mad(u, t, sample_t(0.0466667150077840625632675));
	u = s_mad(u, t, sample_t(-0.0523674852303482457616113));
	u = s_mad(u, t, sample_t(0.0587666392926673580854313));
	u = s_mad(u, t, sample_t(-0.0666573579361080525984562));
	u = s_mad(u, t, sample_t(0.0769219538311769618355029));
	u = s_mad(u, t, sample_t(-0.090908995008245008229153));
	u = s_mad(u, t, sample_t(0.111111105648261418443745));
	u = s_mad(u, t, sample_t(-0.14285714266771329383765));
	u = s_mad(u, t, sample_t(0.199999999996591265594148));
	u = s_mad(u, t, sample_t(-0.333333333333311110369124));
#endif    
	t = s + s * (t * u);    
	t = s_ifthen(q0, SC[S_PI2] - t, t);
	return s_cpsign(t, q1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sample_t s_tan(const sample_t& x)
{
	return s_sin(x)/s_cos(x);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sample_t s_cosh(const sample_t& x)
{
	return (s_exp(x) + s_exp(s_neg(x))) * SC[S_0_5];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sample_t s_sinh(const sample_t& x)
{
	return (s_exp(x) - s_exp(s_neg(x))) * SC[S_0_5];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sample_t s_tanh(const sample_t& x)
{
	return s_sinh(x) / s_cosh(x);
}

#endif