//define structs:

//sceNpOptParam
typedef struct SceNpOptParam
{
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
typedef struct SceNpCommunicationPassphrase
{
    SceUChar8 data[128];
} SceNpCommunicationPassphrase;

//SceNpCommunicationSignature
typedef struct SceNpCommunicationSignature
{
    SceUChar8 data[160];
} SceNpCommunicationSignature;

//SceNpCommunicationConfig
typedef struct SceNpCommunicationConfig
{
    const SceNpCommunicationId *commId;
    const SceNpCommunicationPassphrase *commPassphrase;
    const SceNpCommunicationSignature *commSignature;
} SceNpCommunicationConfig;

//Define SceNpInit
int sceNpInit(const SceNpCommunicationConfig *commConf, SceNpOptParam *opt);

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
int sceNpTrophyCreateContext(SceNpTrophyContext *context, const SceNpCommunicationId *commId, const SceNpCommunicationSignature *commSign, SceUInt64 options);
//Define sceNpTrophyCreateHandle
int sceNpTrophyCreateHandle(SceNpTrophyHandle *handle);
//Define sceNpTrophyUnlockTrophy
int sceNpTrophyUnlockTrophy(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyId trophyId, SceNpTrophyId *platinumId);
//Define sceNpTrophyDestroyContext
int sceNpTrophyDestroyContext(SceNpTrophyContext context);
//Define sceNpTrophyDestroyHandle
int sceNpTrophyDestroyHandle(SceNpTrophyHandle handle);
//Define sceNpTrophyTerm
int sceNpTrophyTerm(void);

//NPTrophySetupDialog

//structs:
typedef struct SceNpTrophySetupDialogParam
{
    SceUInt32 sdkVersion;
    SceCommonDialogParam commonParam;
    SceNpTrophyContext context;
    SceUInt32 options;
    SceUInt8 reserved[128];
} SceNpTrophySetupDialogParam;

typedef struct SceNpTrophySetupDialogResult
{
    SceInt32 result;
    SceUInt8 reserved[128];
} SceNpTrophySetupDialogResult;

//SceNpTrophyDetails
typedef struct SceNpTrophyDetails
{
    SceSize size;
    SceNpTrophyId trophyId;
    SceNpTrophyGrade trophyGrade;
    SceNpTrophyGroupId groupId;
    SceBool hidden;
    SceChar8 name[128];
    SceChar8 description[1024];
} SceNpTrophyDetails;

//SceNpTrophyData
typedef struct SceNpTrophyData
{
    SceSize size;
    SceNpTrophyId trophyId;
    SceBool unlocked;
    SceUInt8 reserved[4];
    SceRtcTick timestamp;
} SceNpTrophyData;

//SceNpTrophyGameDetails
typedef struct SceNpTrophyGameDetails
{
    SceSize size;
    SceUInt32 numGroups;
    SceUInt32 numTrophies;
    SceUInt32 numPlatinum;
    SceUInt32 numGold;
    SceUInt32 numSilver;
    SceUInt32 numBronze;
    SceChar8 title[128];
    SceChar8 description[1024];
} SceNpTrophyGameDetails;

//SceNpTrophyGameData
typedef struct SceNpTrophyGameData
{
    SceSize size;
    SceUInt32 unlockedTrophies;
    SceUInt32 unlockedPlatinum;
    SceUInt32 unlockedGold;
    SceUInt32 unlockedSilver;
    SceUInt32 unlockedBronze;
    SceUInt32 progressPercentage;
} SceNpTrophyGameData;

//functions

//Define sceNpTrophySetupDialogInit
SceInt32 sceNpTrophySetupDialogInit(SceNpTrophySetupDialogParam *param);
//Define sceNpTrophySetupDialogTerm
SceInt32 sceNpTrophySetupDialogTerm(void);
//Define sceNpTrophySetupDialogGetStatus
SceCommonDialogStatus sceNpTrophySetupDialogGetStatus(void);
//Define sceNpTrophySetupDialogGetResult
SceInt32 sceNpTrophySetupDialogGetResult(SceNpTrophySetupDialogResult *result);

//Define sceNpTrophyGetTrophyInfo
int sceNpTrophyGetTrophyInfo(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyId trophyId, SceNpTrophyDetails *details, SceNpTrophyData *data);
//Define sceNpTrophyGetGameInfo
int sceNpTrophyGetGameInfo(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyGameDetails *details, SceNpTrophyGameData *data);
