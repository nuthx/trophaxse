//define structs:

//sceNpOptParam
typedef struct SceNpOptParam {
	SceSize optParamSize;
} SceNpOptParam;

//SceNpCommunicationId
/*typedef struct SceNpCommunicationId {
	char data[9];
	char term;
	SceUChar8 num;
	char dummy;
} SceNpCommunicationId;

allready in vitasdk?*/ 


//SceNpCommunicationPassphrase
#define SCE_NP_COMMUNICATION_PASSPHRASE_SIZE		(128)
typedef struct SceNpCommunicationPassphrase {
	SceUChar8 data[SCE_NP_COMMUNICATION_PASSPHRASE_SIZE];
} SceNpCommunicationPassphrase;

//SceNpCommunicationSignature
#define SCE_NP_COMMUNICATION_SIGNATURE_SIZE			(160)
typedef struct SceNpCommunicationSignature {
	SceUChar8 data[SCE_NP_COMMUNICATION_SIGNATURE_SIZE];
} SceNpCommunicationSignature;

//SceNpCommunicationConfig
typedef struct SceNpCommunicationConfig {
	const SceNpCommunicationId *commId;
	const SceNpCommunicationPassphrase *commPassphrase;
	const SceNpCommunicationSignature *commSignature;
} SceNpCommunicationConfig;



//Define SceNpInit
int sceNpInit(const SceNpCommunicationConfig *commConf,SceNpOptParam *opt);

//NP TROPHY:

//structs:
typedef SceInt32 SceNpTrophyHandle;
typedef SceInt32 SceNpTrophyContext;
typedef SceInt32 SceNpTrophyId;
typedef SceInt32 SceNpTrophyGroupId;
typedef SceInt32 SceNpTrophyGrade;

//functions


//Define sceNpTrophyInit
int sceNpTrophyInit(void *opt);
//Define sceNpTrophyCreateContext
int sceNpTrophyCreateContext(SceNpTrophyContext *context,const SceNpCommunicationId *commId,const SceNpCommunicationSignature *commSign,SceUInt64 options);
//Define sceNpTrophyCreateHandle
int sceNpTrophyCreateHandle(SceNpTrophyHandle *handle);
//Define sceNpTrophyUnlockTrophy
int sceNpTrophyUnlockTrophy(SceNpTrophyContext context,SceNpTrophyHandle handle,SceNpTrophyId trophyId,SceNpTrophyId *platinumId);
//Define sceNpTrophyDestroyContext
int sceNpTrophyDestroyContext(SceNpTrophyContext context);
//Define sceNpTrophyDestroyHandle
int sceNpTrophyDestroyHandle(SceNpTrophyHandle handle);
//Define sceNpTrophyTerm
int sceNpTrophyTerm(void);


//NPTrophySetupDialog

//structs:
typedef struct SceNpTrophySetupDialogParam {
	SceUInt32 sdkVersion;
	SceCommonDialogParam commonParam;
	SceNpTrophyContext context;
	SceUInt32 options;
	SceUInt8 reserved[128];
} SceNpTrophySetupDialogParam;

typedef struct SceNpTrophySetupDialogResult {
	SceInt32 result;
	SceUInt8 reserved[128];
} SceNpTrophySetupDialogResult;

//functions

//Define sceNpTrophySetupDialogInit
SceInt32 sceNpTrophySetupDialogInit(SceNpTrophySetupDialogParam* param);
//Define sceNpTrophySetupDialogTerm
SceInt32 sceNpTrophySetupDialogTerm(void);
//Define sceNpTrophySetupDialogGetStatus
SceCommonDialogStatus sceNpTrophySetupDialogGetStatus(void);
//Define sceNpTrophySetupDialogGetResult
SceInt32 sceNpTrophySetupDialogGetResult(SceNpTrophySetupDialogResult* result);


