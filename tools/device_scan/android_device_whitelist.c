typedef struct 
{
	int	 index;
	char model_number[32];
	int  android_version;
	int	 sub_index;			//for maintance purpose only
	
	int  use_hardware_codec;
	int  use_camera;
	int  use_camera_preview_display;
	
	int  use_iomx_hardware_codec;
	int	 iomx_video_hw;
	int  iomx_audio_hw;
	
    int  java_video_hw_dec;
    int  java_audio_hw_dec;
    
}DeviceInfo;

#define HTC_Nexus_One				101
#define HTC_Sensation_4G			102
#define HTC_Incredible_S			103
#define HTC_Incredible_2			104
#define HTC_One_X					105
#define HTC_DROID_DNA_4G_LTE		106
#define LG_Optimus_2X				201
#define LG_NEXUS_5					202
#define Motorola_Atrix				301
#define Motorola_Xoom				302
#define Motorola_DroidX				303
#define Samsung_Infuse				401
#define Samsung_Nexus_S				402
#define Samsung_Galaxy_S_Captivate	403
#define Samsung_Galaxy_S_Epic		404
#define Samsung_Galaxy_S2			405
#define Samsung_Galaxy_S2_SkyRocket	406
#define Samsung_Galaxy_Nexus		407
#define Samsung_Galaxy_Tab			408
#define Samsung_Galaxy_Tab_7_dot_7	409
#define Samsung_Galaxy_S3			410
#define Samsung_Galaxy_Note			411
#define Samsung_Galaxy_Note_II_Intl	412
#define Samsung_Galaxy_S4_US		413
#define Samsung_Galaxy_S4_Int		414
#define Samsung_Marvell_powered_Galaxy_Tab_7 415
#define Samsung_Galaxy_S4_US2		416
#define Sony_Ericsson_XPERIA		501
#define Sony_Tablet_S				502
#define Marvell_DKB_board			601
#define Marvell_Berline_BG2			602
#define ASUS_China_Mobile_OPhone	701
#define ASUS_Transformer_TF101		702
#define ASUS_Transformer_Pad_TF300	703
#define Google_Nexus_7				704
#define Google_Nexus_10				705
#define Google_Nexus_4				706
#define ZTE_Blade_3G_Series_U880	801
#define HUAWEI_China_Mobile_OPhone	901
#define HUAWEI_Vision				902
#define VIZIO_tablet				1001
#define SHARP_AQUOS_IS12SH			1101
#define SHARP_GALAPAGOS_TABLET_A01SH 1102
#define Marvell_PXA988				1201
#define Marvell_PXA988_DKB			1202
#define Xiaomi_MI_2                 1301

DeviceInfo my_devices[] =
{//		index						model_number				android_version sub codec/cam/preview iomx_hw/iomx_v/iomx_a jmc_v/jmc_a
	{ HTC_Nexus_One,				"Nexus One",				ANDROID_FROYO,  0,	1,1,1,	0,0,0, 0,0},	//1,1,0},	//on
	{ HTC_Nexus_One,				"Nexus One",				ANDROID_GINGER, 1,	1,1,1,	0,0,0, 0,0},	//on
	
	{ HTC_Sensation_4G,				"HTC Sensation 4G",			ANDROID_GINGER, 0,	1,0,0,	0,0,0, 0,0},
	{ HTC_Sensation_4G,				"HTC Sensation 4G",			ANDROID_ICE,    0,	1,0,0,  0,0,0, 0,0},
	{ HTC_Incredible_S,				"HTC Incredible S",			ANDROID_GINGER, 0,	0,0,0,	0,0,0, 0,0},	//?? not tested
	
	{ HTC_One_X,					"HTC One X",				ANDROID_ICE,	0,	1,0,0,	0,0,0, 0,0},	//on
	
	{ HTC_DROID_DNA_4G_LTE,			"HTC6435LVW",				ANDROID_JELLY,	0,	1,0,0,	0,0,0, 0,0},	//on
	
	{ LG_Optimus_2X,				"LG-P990",					ANDROID_FROYO,  0,	1,0,0,	0,0,0, 0,0},	//on
	
	{ Motorola_Atrix,				"MB860",					ANDROID_FROYO,  0,	1,0,0,	0,0,0, 0,0},	//1,1,0},	//on
	{ Motorola_Atrix,				"MB860",					ANDROID_GINGER, 1,	1,0,0,	0,0,0, 0,0},	//on
	//{ Motorola_Atrix,				"MB860",					ANDROID_ICE,    2,	1,0,0 },
	//{ Motorola_Atrix,				"MB860",					ANDROID_JELLY,  3,	1,0,0 },
	
	{ Motorola_Xoom,				"Xoom",						ANDROID_HONEY,  0,	0,0,0,	0,0,0, 0,0},	//Honeycomb not supported
	{ Motorola_Xoom,				"Xoom",						ANDROID_ICE,    0,	0,0,0,	0,0,0, 0,0},	//performance not good
	//{ Motorola_Xoom,				"Xoom",						ANDROID_JELLY,  0,	0,0,0 },	//?? not tested
	//{ Motorola_DroidX,			"DROIDX",					ANDROID_GINGER, 0,	0,0,0 },	//?? not tested
	
	{ Samsung_Infuse,				"SAMSUNG-SGH-I997",			ANDROID_FROYO,  0,	0,0,0,	0,0,0, 0,0},	//not a good device
	{ Samsung_Infuse,				"SAMSUNG-SGH-I997",			ANDROID_GINGER, 1,	1,0,0,	0,0,0, 0,0},	//it's better upgraded to Gingerbread
	
	{ Samsung_Nexus_S,				"Nexus S",					ANDROID_GINGER, 0,	0,1,0,	0,0,0, 0,0},	//stagefright OMX is too slow
	{ Samsung_Nexus_S,				"Nexus S",					ANDROID_ICE,    1,	0,1,0,	0,0,0, 0,0},	//artifact at beginning
	
	{ Samsung_Galaxy_S_Captivate,	"SAMSUNG-SGH-I897",			ANDROID_ECLAIR, 0,	0,0,0,	0,0,0, 0,0},	//too early
	{ Samsung_Galaxy_S_Captivate,	"SAMSUNG-SGH-I897",			ANDROID_FROYO,  1,	0,0,0,	0,0,0, 0,0},	//not good  --causing crash
	
	{ Samsung_Galaxy_S_Epic,		"SPH-D700",					ANDROID_GINGER, 0,	1,0,0,	0,0,0, 0,0},	//on --mp4v is off
	
	{ Samsung_Galaxy_S2,			"GT-I9100",					ANDROID_GINGER, 0,	1,0,0,	1,0,1, 0,0},	//on
	{ Samsung_Galaxy_S2,			"GT-I9100",					ANDROID_ICE,	1,	1,0,0,	1,0,1, 0,0},	//1,1,1},	//on
	{ Samsung_Galaxy_S2,			"GT-I9100",					ANDROID_JELLY,	0,	0,0,0,	1,1,1, 0,0},	//1,1,1},	//on
	
	{ Samsung_Galaxy_S2_SkyRocket,	"SAMSUNG-SGH-I727",			ANDROID_GINGER, 0,	1,0,0,	1,0,1, 0,0},	//1,0,1},	//on
	{ Samsung_Galaxy_S2_SkyRocket,	"SAMSUNG-SGH-I727",			ANDROID_ICE,	1,	1,0,0,	1,1,1, 0,0},	//1,1,1},	//on
	
	{ Samsung_Galaxy_Nexus,			"Galaxy Nexus",				ANDROID_ICE,    0,	1,0,0,	0,0,0, 0,0},	//on
	{ Samsung_Galaxy_Nexus,			"Galaxy Nexus",				ANDROID_JELLY,  1,	1,0,0,	0,0,0, 0,0},	//on
	{ Samsung_Galaxy_Nexus,			"Galaxy Nexus",				ANDROID_JELLY2, 1,	1,0,0,	0,0,0, 0,0},	//on
	
	{ Samsung_Galaxy_Tab,			"SCH-I800",					ANDROID_GINGER, 0,	1,0,0,	0,0,0, 0,0},	//1,0,1},	//on --mp4v is off
	
	{ Samsung_Galaxy_Tab_7_dot_7,	"GT-P6810",					ANDROID_GINGER, 0,	0,0,0,	0,0,0, 0,0},	//?? not tested
	{ Samsung_Marvell_powered_Galaxy_Tab_7,	"SM-T210R",			ANDROID_JELLY, 0,	0,0,0,	0,0,0, 1,0},	//
	{ Samsung_Marvell_powered_Galaxy_Tab_7,	"SM-T210R",			ANDROID_JELLY2, 0,	0,0,0,	0,0,0, 1,0},	//
	
	{ Samsung_Galaxy_S3,			"GT-I9300",					ANDROID_ICE,	0,	1,0,0,	1,1,1, 0,0},	//on
	{ Samsung_Galaxy_S3,			"GT-I9300",					ANDROID_JELLY,	1,	1,0,0,	1,1,1, 1,0},	//1,1,1},	//on
	
	{ Samsung_Galaxy_S4_US,			"SGH-M919",					ANDROID_JELLY2,	0,	0,0,0,	1,1,1, 1,0},	//1,1,1},	//on
	
    { Samsung_Galaxy_S4_US2,		"SCH-I545",					ANDROID_JELLY3,	0,	0,0,0,	0,0,0, 1,0},	//1,1,1},	//on

	{ Samsung_Galaxy_S4_Int,		"GT-I9500",					ANDROID_JELLY2,	0,	1,0,0,	1,1,1, 0,0},	//on

	{ Samsung_Galaxy_Note,			"SAMSUNG-SGH-I717",			ANDROID_ICE,	0,	1,0,0,	0,0,0, 0,0},	//on

	{ Samsung_Galaxy_Note_II_Intl,	"GT-N7100",					ANDROID_JELLY,	0,	1,0,0,	1,1,1, 0,0},	//on
	
	{ Sony_Ericsson_XPERIA,			"SO-01C",					ANDROID_GINGER, 0,	1,0,0,	0,0,0, 0,0},	//on
	
	{ Sony_Tablet_S,				"Sony Tablet S",			ANDROID_HONEY,  0,	0,0,0,	0,0,0, 0,0},	//Honeycomb not supported
	
	{ Marvell_DKB_board,			"dkb",						ANDROID_GINGER, 0,	1,0,0,	0,0,0, 0,0},	//on
	//{ Marvell_Berline_BG2,		"berlin_bg2",				ANDROID_ICE,    0,	0,0,0 },	//?? not tested
	
	{ ASUS_China_Mobile_OPhone,		"OPhone OS 2.0 (53857)",	ANDROID_ECLAIR, 0,	0,0,0,	0,0,0, 0,0},	//too early
	
	{ ASUS_Transformer_TF101,		"ASUS Transformer TF101",	ANDROID_HONEY,  0,	0,0,0,	0,0,0, 0,0},	//Honeycomb not supported
	{ ASUS_Transformer_TF101,		"Transformer TF101",		ANDROID_ICE,	1,	0,0,0,  0,0,0, 0,0},	//?? not tested
	
	{ ASUS_Transformer_Pad_TF300,"ASUS Transformer Pad TF300T",	ANDROID_ICE,	0,	1,0,0,	0,0,0, 0,0},	//on
	{ ASUS_Transformer_Pad_TF300,"ASUS Transformer Pad TF300T",	ANDROID_JELLY,  1,	1,0,0,	0,0,0, 0,0},	//on
	{ ASUS_Transformer_Pad_TF300,"ASUS Transformer Pad TF300T",	ANDROID_JELLY2, 1,	1,0,0,	0,0,0, 0,0},	//on
    
	{ Google_Nexus_7,               "Nexus 7",                  ANDROID_JELLY,  0,  1,0,0,  0,0,0, 0,0}, //on
	{ Google_Nexus_7,				"Nexus 7",					ANDROID_JELLY2, 1,	1,0,0,	0,0,0, 0,0},	//on
	
	{ Google_Nexus_10,				"Nexus 10",					ANDROID_JELLY,  0,	1,0,0,	0,0,0, 0,0},//
	{ Google_Nexus_10,				"Nexus 10",					ANDROID_JELLY2, 0,	1,0,0,	0,0,0, 0,0},//
	
	{ Google_Nexus_4,				"Nexus 4",					ANDROID_JELLY,  0,	1,0,0,	0,0,0, 0,0},//
	{ Google_Nexus_4,				"Nexus 4",					ANDROID_JELLY2, 0,	1,0,0,	0,0,0, 0,0},//
	{ Google_Nexus_4,				"Nexus 4",					ANDROID_KITKAT, 0,	1,0,0,	0,0,0, 0,0},//
	
	{ ZTE_Blade_3G_Series_U880,		"ZTE-T U880",				ANDROID_FROYO,  0,	0,0,0,	0,0,0, 0,0},	//using OMX
	
	{ HUAWEI_China_Mobile_OPhone,	"HUAWEI T8600",				ANDROID_FROYO,  0,	0,0,0,	0,0,0, 0,0},	//using OMX?
	
	{ HUAWEI_Vision,				"HUAWEI U8850",				ANDROID_GINGER, 0,	1,0,0,	0,0,0, 0,0},	//on
	
	{ VIZIO_tablet,					"VTAB1008",					ANDROID_GINGER, 0,	0,0,0,	0,0,0, 0,0},	//stagefright OMX is too slow
	{ VIZIO_tablet,					"VTAB1008",					ANDROID_HONEY,  1,	0,1,0,	0,0,0, 0,0},	//stagefright OMX is too slow
	
	{ SHARP_AQUOS_IS12SH,			"IS12SH",					ANDROID_GINGER, 0,	0,0,0,	0,0,0, 0,0},	//?? not tested
	
	{ Marvell_PXA988,               "pxa988ff_cmcc",            ANDROID_JELLY,  0,  0,0,0,  0,1,1, 0,0},

	{ Marvell_PXA988_DKB,           "pxa988dkb_def",            ANDROID_JELLY,  0,  1,0,0,  1,1,1, 0,0},  //DKB can be on without affecting release version
	
	{ SHARP_GALAPAGOS_TABLET_A01SH,	"A01SH",					ANDROID_HONEY,  0,	0,0,0,	0,0,0, 0,0},	//Honeycomb not supported

	{ Xiaomi_MI_2,                  "MI 2",                     ANDROID_JELLY,  0,	0,0,0,	0,0,0, 1,0},
	{ Xiaomi_MI_2,                  "MI 2",                     ANDROID_JELLY2, 0,	0,0,0,	0,0,0, 1,0},

	{ LG_NEXUS_5,					"Nexus 5",					ANDROID_LOLLIPOP, 0,	0,0,0,	0,0,0, 1,0}
};

