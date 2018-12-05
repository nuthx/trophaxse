#include <stdio.h>
#include <string.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>
#include <vitasdk.h>
#include <taihen.h>
#include <sys/syslimits.h>
#include <appmgr_user.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "debugScreen.h"
#include "np.h"
#include "rtc.h"

#define printf psvDebugScreenPrintf
#define F psvDebugScreenFont
#define SCREEN_COL (SCREEN_WIDTH / F.size_w)
#define SCREEN_ROW (SCREEN_HEIGHT / F.size_h)
#define CENTERX(buf) ((SCREEN_COL - strlen(buf)) / 2)
#define CENTERY (SCREEN_ROW / 2)
#define WINDOW_HEIGHT (SCREEN_ROW - 4)
#define DISPLAY_COLOR_FORMAT SCE_GXM_COLOR_FORMAT_A8B8G8R8

int oWindow = 0;
SceCtrlData pad;

/*
 * FUCK YOU SONY
 * 
 * MAKING ME FUCKIN INIT libGxm
 */ 

#define DISPLAY_WIDTH			960
#define DISPLAY_HEIGHT			544
#define DISPLAY_STRIDE_IN_PIXELS	1024
#define DISPLAY_BUFFER_COUNT		2
#define DISPLAY_MAX_PENDING_SWAPS	1
#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))

typedef struct{
	void*data;
	SceGxmSyncObject*sync;
	SceGxmColorSurface surf;
	SceUID uid;
}displayBuffer;

void gxm_vsync_cb(const void *callback_data){
	sceDisplaySetFrameBuf(&(SceDisplayFrameBuf){sizeof(SceDisplayFrameBuf),
		*((void **)callback_data),DISPLAY_STRIDE_IN_PIXELS, 0,
		DISPLAY_WIDTH,DISPLAY_HEIGHT}, SCE_DISPLAY_SETBUF_NEXTFRAME);
}

displayBuffer dbuf[DISPLAY_BUFFER_COUNT];

void *dram_alloc(unsigned int size, SceUID *uid){
	void *mem;
	*uid = sceKernelAllocMemBlock("gpu_mem", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, ALIGN(size,256*1024), NULL);
	sceKernelGetMemBlockBase(*uid, &mem);
	sceGxmMapMemory(mem, ALIGN(size,256*1024), SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);
	return mem;
}

void gxm_init(){
        int ret = 0;
	ret = sceGxmInitialize(&(SceGxmInitializeParams){0,DISPLAY_MAX_PENDING_SWAPS,gxm_vsync_cb,sizeof(void *),SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE});
        
        if(ret < 0){
        printf("sceGxmInitialize() failed. ret = 0x%x\n", ret);
        while(1){};
        }
        
        unsigned int i;
	for (i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
		dbuf[i].data = dram_alloc(4*DISPLAY_STRIDE_IN_PIXELS*DISPLAY_HEIGHT, &dbuf[i].uid);
		sceGxmColorSurfaceInit(&dbuf[i].surf,SCE_GXM_COLOR_FORMAT_A8B8G8R8,SCE_GXM_COLOR_SURFACE_LINEAR,SCE_GXM_COLOR_SURFACE_SCALE_NONE,SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,DISPLAY_WIDTH,DISPLAY_HEIGHT,DISPLAY_STRIDE_IN_PIXELS,dbuf[i].data);
		sceGxmSyncObjectCreate(&dbuf[i].sync);
	}
}


/*
 * FUCK YOU SONY
 * 
 * MAKING ME FUCKIN INIT libGxm
 */ 



char titleid[12];
char g_currentMount[16];
char g_currentPath[PATH_MAX];

typedef struct paths {
char path[1024];
} paths;


void updateCommonDiag()
{
        int ret = 0;
        SceCommonDialogUpdateParam	updateParam;
        ScePVoid color = (ScePVoid)0xFFFFFF;

        memset(&updateParam, 0, sizeof(updateParam));
        updateParam.renderTarget.colorFormat    = DISPLAY_COLOR_FORMAT;
        updateParam.renderTarget.surfaceType    = SCE_GXM_COLOR_SURFACE_LINEAR;
        updateParam.renderTarget.width          = DISPLAY_WIDTH;
        updateParam.renderTarget.height         = DISPLAY_HEIGHT;
        updateParam.renderTarget.strideInPixels = DISPLAY_STRIDE_IN_PIXELS;

        updateParam.renderTarget.colorSurfaceData = color;
        ret = sceCommonDialogUpdate(&updateParam);
        if(ret < 0){
            printf("sceCommonDialogUpdate() failed. ret = 0x%x\n", ret);
            while(1){};
        }
}
int isSceCommonDiagRunning()
{

        SceCommonDialogStatus cdStatus;

        cdStatus = sceNpTrophySetupDialogGetStatus();
        

        
        if (cdStatus == SCE_COMMON_DIALOG_STATUS_RUNNING)
        {
            return 1;
        }
        else if (cdStatus == SCE_COMMON_DIALOG_STATUS_FINISHED)
        {
            return 0;
        }
        else
        {
            return -1;
        }
}
    

int WriteFile(char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_RDWR | SCE_O_CREAT, 0777);
	if (fd < 0)
		return fd;

	int written = sceIoWrite(fd, buf, size);

	sceIoClose(fd);
	return written;
}

int ReadFile(char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file,SCE_O_RDONLY, 0777);
	if (fd < 0)
		return fd;

	int read = sceIoRead(fd, buf, size);

	sceIoClose(fd);
	return read;
}



int getFileSize(const char *file) {
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;
	int fileSize = sceIoLseek(fd, 0, SCE_SEEK_END);
	sceIoClose(fd);
	return fileSize;
}




int knomn_pfs_ids[] = {
  0x12F,
  0x3E8,
};

int pfsMount(const char *path) {
    	int ret;
	char klicensee[0x10];
	SceAppMgrMountIdArgs args;

	memset(klicensee, 0, sizeof(klicensee));

	args.process_titleid = titleid;
	args.path = path;
	args.desired_mount_point = NULL;
	args.klicensee = klicensee;
	args.mount_point = g_currentMount;
	
	for(int i=0;i<sizeof(knomn_pfs_ids) / sizeof(int); i++){

		args.id = knomn_pfs_ids[i];

		ret = sceAppMgrUserMountById(&args);

		if(ret >= 0){

			return ret;

		}

	}

	return ret;
}



void sceNpTrophySetupDialogParamInit(SceNpTrophySetupDialogParam* param)
{
	sceClibMemset( param, 0x0, sizeof(SceNpTrophySetupDialogParam) );
	_sceCommonDialogSetMagicNumber( &param->commonParam );
	param->sdkVersion = 0x03550011;
	param->context = -1;
	param->options = 0;
}


int setSecureTick(unsigned long long int psTime)
{

	
	
	SceRtcTick utcTime = {0};
	SceRtcTick localTime = {0};
	
	utcTime.tick = psTime;
	sceRtcConvertUtcToLocalTime(&utcTime,&localTime);
	psTime = localTime.tick;
	
	printf("setSecureTick: %llx\n",psTime);
	
	printf("Creating splits..\n");
	char hexint[15];
	memset(hexint,0,15);

	sprintf(hexint,"%llx",psTime);

	char ts1[7] = {hexint[0],hexint[1],hexint[2],hexint[3],hexint[4],hexint[5],0x00};
	unsigned long long int split1 = (unsigned int)strtoul(ts1, NULL, 16);
	printf("Split1: %llx\n",split1);


	char ts2[10] = {hexint[6],hexint[7],hexint[8],hexint[9],hexint[10],hexint[11],hexint[12],hexint[13],hexint[14],0x00};
	unsigned long long int split2 = (unsigned int)strtoul(ts2, NULL, 16);
	printf("Split2: %llx\n",split2);

	return SetTrophyTimes(split2,split1);
}



int main() {
        gxm_init();
		psvDebugScreenInit();
start:
        psvDebugScreenClear();

                
        int size = 0;

        int dfd = sceIoDopen("ur0:user/00/trophy/data/");
        int res;
        do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));
        res = sceIoDread(dfd, &dir);
        if (res > 0 && SCE_S_ISDIR(dir.d_stat.st_mode) && strcmp(dir.d_name,"sce_trop") != 0) {
            size += 1;
        }} while (res > 0);
        sceIoDclose(dfd);


        paths *title_list = malloc(sizeof(paths)*size);
        
        int a = 0;
        dfd = sceIoDopen("ur0:/user/00/trophy/data/");
        do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));
        res = sceIoDread(dfd, &dir);
        if (res > 0 && SCE_S_ISDIR(dir.d_stat.st_mode) && strcmp(dir.d_name,"sce_trop") != 0) {
            char path[1024];
            sprintf(path,"ur0:/user/00/trophy/conf/%s/TROP.SFM",dir.d_name);
            int sfmsize = getFileSize(path);
            char *sfm = malloc(sfmsize);
            char titledest[256];
            
            ReadFile(path,sfm,sfmsize);
            
            int len = strstr(sfm,"</title-name>")  -  (strstr(sfm,"<title-name>")  + sizeof("<title-name>")) - 1;
            len += 2;
            memcpy(&titledest, strstr(sfm,"<title-name>") + sizeof("<title-name>") - 1, len);
            titledest[len] = 0;
            
            char name[1028];
            sprintf(name,"%.45s (%s)\n",titledest,dir.d_name);
            
            strcpy(title_list[a].path, name);
            a += 1;
        }} while (res > 0);
        sceIoDclose(dfd);

        int x = 1, y = 1, selection = 0, window = 0;
	
    do
    {
        char buf[256];
        strncpy(buf, "**** TropHax StandAlone Edition ****", sizeof(buf));
        printf("\e[%i;%iH%s", 1, CENTERX(buf), buf);
        strncpy(buf, "Choose a game", sizeof(buf));
        printf("\e[%i;%iH%s", 2, CENTERX(buf), buf);
        y = 3;
        int max = (size < WINDOW_HEIGHT) ? size : WINDOW_HEIGHT;
        for (int i=0; i < max; i++) {
            if (i+window == selection)
                psvDebugScreenSetFgColor(0xFF0000);
            printf("\e[%i;%iH%s", y, x, title_list[i+window].path);
            y += 1;
            psvDebugScreenSetFgColor(0xFFFFFF);
        }
        strncpy(buf, "U/D: Select Game  X: Confirm  O: Exit", sizeof(buf));
        printf("\e[%i;%iH%s", SCREEN_ROW, CENTERX(buf), buf);
        memset(&pad, 0, sizeof(pad));
        sceCtrlPeekBufferPositive(0, &pad, 1);
		
        if (pad.buttons == SCE_CTRL_UP)
            {
                if (selection <= size - WINDOW_HEIGHT){
                    window -= 1;
				}
                if (window < 0){
                    window = 0;
				}
                selection -= 1;
                if (selection < 0)
                    selection = 0;
                sceKernelDelayThread(150000);
            }
        if (pad.buttons == SCE_CTRL_DOWN)
            {
                if (selection >= WINDOW_HEIGHT - 1){
                    window += 1;
				}
                if (window > size - WINDOW_HEIGHT){
                    window = (size - WINDOW_HEIGHT < 0) ? 0 : size - WINDOW_HEIGHT;
				}
                selection += 1;
                if (selection + 1 > size - 1)
                    selection = size - 1;
                sceKernelDelayThread(150000);
            }
        if (pad.buttons == SCE_CTRL_CROSS)
        {
                psvDebugScreenClear();
                printf("Preforming TrophyPatcher Operations Please wait ...\n");
				
                int ret = sceAppMgrUmount("app0:");
                if(ret < 0){
                        printf("sceAppMgrUmount() failed. ret = 0x%x\n", ret);
                        while(1){};
                }

                char kplugin_path[0x200];
                char uplugin_path[0x200];
                sceAppMgrAppParamGetString(0, 12, titleid , 256);

                sprintf(kplugin_path, "ux0:app/%s/module/kernel.skprx", titleid);
                sprintf(uplugin_path, "ux0:app/%s/module/user.suprx", titleid);
				printf("kplugin_path: %s\n",kplugin_path);
				printf("uplugin_path: %s\n",uplugin_path);
                int kernel_modid, user_modid;

                kernel_modid = taiLoadStartKernelModule(kplugin_path, 0, NULL, 0);
				if(kernel_modid < 0){
                        printf("taiLoadStartKernelModule() failed. ret = 0x%x\n", kernel_modid);
                }	
                user_modid = sceKernelLoadStartModule(uplugin_path, 0, NULL, 0, NULL, NULL);
				if(user_modid < 0){
                        printf("sceKernelLoadStartModule() failed. ret = 0x%x\n", user_modid);
                }
				
				
				printf("Setting up memory...");

                char trophy_path[0x200];
                


                
                SceUChar8 commid[256];
                memset(commid,0,256);
                char name[1024];
                memset(name,0,256);
                char titleidOfGame[256];
                memset(titleidOfGame,0,256);
                char trptranspath[1024];
                memset(trptranspath,0,1024);
                char trptitlepath[1024];
                memset(trptitlepath,0,1024);
                SceUChar8 commsign[160];
                memset(commsign,0,160);
                
                char path[1024];
                
                SceUID fd;
                int len;
                int size;
                
                
                sprintf(name,"%s",title_list[selection].path);
                len = strstr(name,")")  -  (strstr(name,"(")  + sizeof("(")) - 1;
                len +=2;
                memcpy(&commid, strstr(name,"(") + sizeof("(") - 1, len);
                commid[len] = 0;
				printf(" OK\n");
                sprintf(trophy_path, "ur0:user/00/trophy/data/%s/", commid);
				
                ret = pfsMount(trophy_path);
                if(ret < 0){
                        printf("pfsMount() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
               
                sprintf(trptitlepath,"%s/TRPTITLE.DAT",g_currentMount);
                    
                fd = sceIoOpen(trptitlepath,SCE_O_RDONLY, 0777);
                sceIoLseek(fd,0x290,SCE_SEEK_SET);
                sceIoRead(fd,titleidOfGame,0x0A);
                sceIoClose(fd);
                
                sprintf(trptranspath,"%s/TRPTRANS.DAT",g_currentMount);
                    
                fd = sceIoOpen(trptranspath,SCE_O_RDONLY, 0777);
                sceIoLseek(fd,0x190,SCE_SEEK_SET);
                sceIoRead(fd,commsign,160);
                sceIoClose(fd);
                
                ret = sceAppMgrUmount(g_currentMount);
                if(ret < 0){
                        printf("sceAppMgrUmount() failed. ret = 0x%x\n", ret);
                        while(1){};
                }


				
				//Try find where trophy.trp is located.
				printf("Locating trophy.trp...\n");
				
				char location[0x1028];
				memset(location,0x0,0x1028);
				char checkPath[0x1028];
				
				memset(checkPath,0x00,0x1028);
				sprintf(checkPath,"ux0:/app/%s/sce_sys/trophy/%s/TROPHY.TRP",titleidOfGame,commid);
				
				if(getFileSize(checkPath) >=0)
				{
					sprintf(location,"ux0:/app");
					goto Found;
				}
				
				memset(checkPath,0x00,0x1028);
				sprintf(checkPath,"gro0:/app/%s/sce_sys/trophy/%s/TROPHY.TRP",titleidOfGame,commid);
				
				if(getFileSize(checkPath) >=0)
				{
					sprintf(location,"gro0:/app");
					goto Found;
				}
				memset(checkPath,0x00,0x1028);
				sprintf(checkPath,"ur0:/app/%s/sce_sys/trophy/%s/TROPHY.TRP",titleidOfGame,commid);
				
				if(getFileSize(checkPath) >=0)
				{
					sprintf(location,"ur0:/app");
					goto Found;
				}
				
				memset(checkPath,0x00,0x1028);
				sprintf(checkPath,"pd0:/app/%s/sce_sys/trophy/%s/TROPHY.TRP",titleidOfGame,commid);
				
				if(getFileSize(checkPath) >=0)
				{
					sprintf(location,"pd0:/app"); //Welcome Park
					goto Found;
				}
				
				printf("Cound not find %s (Possibly game not installed?)\n",titleidOfGame);
				while(1){};
				
Found:
				
				printf("Found! - %s is in %s/%s\n",commid,location,titleidOfGame);
				
				
				sprintf(path,"%s/%s",location,titleidOfGame);
                ret = sceAppMgrGameDataMount(path,0,0,g_currentMount); //GameDataMount mounts WITH patches, so no need to check for them
                if(ret < 0){
                        printf("sceAppMgrGameDataMount() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
				

                

                
                memset(path,0,1024);
                sprintf(path,"%s/sce_sys/trophy/%s/TROPHY.TRP",g_currentMount,commid);
                size = getFileSize(path);
                if (size >=0)
                {
                char *trpfile = malloc(size);
                memset(trpfile,0,size);
                ret = ReadFile(path,trpfile,size);
                if(ret < 0){
                        printf("ReadFile() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                ret = ReadFile(path,trpfile,size);
                if(ret < 0){
                        printf("ReadFile() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                
                memset(path,0,1024);
                sprintf(path,"ux0:app/%s/sce_sys",titleid);
                sceIoMkdir(path,0006);
                sprintf(path,"ux0:app/%s/sce_sys/trophy",titleid);
                sceIoMkdir(path,0006);
                sprintf(path,"ux0:app/%s/sce_sys/trophy/%s",titleid,commid);
                sceIoMkdir(path,0006);
                sprintf(path,"ux0:app/%s/sce_sys/trophy/%s/TROPHY.TRP",titleid,commid);
				
                ret = WriteFile(path,trpfile,size);
                if(ret < 0){
                        printf("WriteFile() failed. ret = 0x%x\n", ret);
                }
                }
                else
                {
                printf("getFileSize() failed. ret: 0x%x\n",size);
                if (ret == 0x80010002)
                {
                printf("Possibly the game is not installed?\n");
                }
                
                while(1){};
                
                }
                ret = sceAppMgrUmount(g_currentMount);
                if(ret < 0){
                        printf("sceAppMgrUmount() failed. ret = 0x%x\n", ret);
                        while(1){};
                }

                
                // remount app0
                // CelesteBlue TAKE NOTES.
                memset(path,0,1024);
                sprintf(path,"ux0:/app/%s",titleid);
                ret = pfsMount(path);
                if(ret < 0){
                        printf("pfsMount() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                
                
                //Setup trophys
                
                printf("Setting up Trophys! Please wait ...");
                
                ret = sceSysmoduleLoadModule(SCE_SYSMODULE_NP);
                    
                if (ret < 0) {
                        printf("sceSysmoduleLoadModule(SCE_SYSMODULE_NP) failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                
                ret = sceSysmoduleLoadModule(SCE_SYSMODULE_NP_TROPHY);
                if (ret < 0) {
                        printf("sceSysmoduleLoadModule(SCE_SYSMODULE_NP_TROPHY) failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                
                ret = sceNpTrophyInit(NULL);
                if (ret < 0) {
                        printf("sceNpTrophyInit() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                
                SceNpTrophyContext trophyContext = -1;
                
                SceNpCommunicationId npCommId = {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},0,0,0};
                memcpy(npCommId.data,commid,9);
                SceNpCommunicationSignature npCommSign;
                memcpy(npCommSign.data,commsign,160);
                SceNpTrophyHandle handle = -1;
                
                
                ret = sceNpTrophyCreateContext(&trophyContext,&npCommId,&npCommSign,0);
                if (ret < 0) {
                        printf("sceNpTrophyCreateContext() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                
                ret = sceNpTrophyCreateHandle(&handle);
                if (ret < 0) {
                        printf("sceNpTrophyCreateHandle() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                
                
                SceNpTrophySetupDialogParam setupParam;
                
                sceNpTrophySetupDialogParamInit(&setupParam);
                
                setupParam.context = trophyContext;
                
                ret = sceCommonDialogSetConfigParam(&(SceCommonDialogConfigParam){});
                if(ret < 0) {
                        printf("sceCommonDialogSetConfigParam() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                
                ret = sceNpTrophySetupDialogInit(&setupParam);
                if(ret < 0){
                        printf("sceNpTrophySetupDialogInit() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                

                while(isSceCommonDiagRunning())
                {
                    updateCommonDiag();
                }
                
                SceNpTrophySetupDialogResult setupResult;
                memset(&setupResult, 0, sizeof(setupResult));

                ret = sceNpTrophySetupDialogGetResult(&setupResult);
                if(ret < 0){
                        printf("sceNpTrophySetupDialogGetResult() failed. ret = 0x%x\n", ret);
                        while(1){};
                }
                
                ret = sceNpTrophySetupDialogTerm();
                if(ret < 0){
                        printf("sceNpTrophySetupDialogTerm() failed. ret = 0x%x\n", ret);
                        while(1){};
                }

                if (setupResult.result != SCE_COMMON_DIALOG_RESULT_OK){
                        printf("Failed! %lx",setupResult.result);
                        while(1){}
                }
                
                printf("OK!\n");
                
				printf("All prep done!\n");
				sceKernelDelayThread(500000);
				
				//All done!
TrophyMenu:
				//Draw new menu
				psvDebugScreenClear(); // Clear the screen!
				x = 1, y = 1, selection = 0, window = 0;
				
				size = 3;
				paths *option_list = malloc(sizeof(paths)*size);

				
				//set options
				strcpy(option_list[0].path, "Unlock a Trophy");
				strcpy(option_list[1].path, "Unlock All Trophys");
				strcpy(option_list[3].path, "Exit");
				
				while(1)
					{
					char buf[256];
					strncpy(buf, "**** TropHax StandAlone Edition ****", sizeof(buf));
					printf("\e[%i;%iH%s", 1, CENTERX(buf), buf);
					strncpy(buf, "Choose an Option:", sizeof(buf));
					printf("\e[%i;%iH%s", 2, CENTERX(buf), buf);
					y = 3;
					int max = (size < WINDOW_HEIGHT) ? size : WINDOW_HEIGHT;
					for (int i=0; i < max; i++) {
						if (i+window == selection)
							psvDebugScreenSetFgColor(0xFF0000);
						printf("\e[%i;%iH%s", y, x, option_list[i+window].path);
						y += 1;
						psvDebugScreenSetFgColor(0xFFFFFF);
					}
					strncpy(buf, "U/D: Select Option  X: Confirm  O: Return", sizeof(buf));
					printf("\e[%i;%iH%s", SCREEN_ROW, CENTERX(buf), buf);
					memset(&pad, 0, sizeof(pad));
					sceCtrlPeekBufferPositive(0, &pad, 1);

					if (pad.buttons == SCE_CTRL_UP)
					{
						if (selection <= size - WINDOW_HEIGHT){
							window -= 1;
						}
						if (window < 0){
							window = 0;
						}
						selection -= 1;
						if (selection < 0)
							selection = 0;
						sceKernelDelayThread(150000);
					}
				if (pad.buttons == SCE_CTRL_DOWN)
					{
						if (selection >= WINDOW_HEIGHT - 1){
							window += 1;
						}
						if (window > size - WINDOW_HEIGHT){
							window = (size - WINDOW_HEIGHT < 0) ? 0 : size - WINDOW_HEIGHT;
						}
						selection += 1;
						if (selection + 1 > size - 1)
							selection = size - 1;
						sceKernelDelayThread(150000);
					}
						
				if(pad.buttons == SCE_CTRL_CIRCLE)
				{
					psvDebugScreenClear();
					ret = sceNpTrophyDestroyContext(trophyContext);
					if(ret < 0){
							printf("sceNpTrophyDestroyContext() failed. ret = 0x%x\n", ret);
							while(1){};
					}
					
					ret = sceNpTrophyDestroyHandle(handle);
					if(ret < 0){
							printf("sceNpTrophyDestroyHandle() failed. ret = 0x%x\n", ret);
							while(1){};
					}
					
					ret = sceNpTrophyTerm();
					if(ret < 0){
							printf("sceNpTrophyTerm() failed. ret = 0x%x\n", ret);
							while(1){};
					}
					
					
					sceKernelStopUnloadModule(user_modid, 0, NULL, 0, NULL, NULL);
					taiStopUnloadKernelModule(kernel_modid, 0, NULL, 0, NULL, NULL); 
					sceKernelDelayThread(150000);
					goto start;
				}
				
				if (pad.buttons == SCE_CTRL_CROSS)
				{
				
					if(selection == 0)
					{
						//Show trophy selection menu
						x = 1, y = 1, selection = 0, window = 0;
						psvDebugScreenClear();
						printf("Obtaining trophy count..");
						SceNpTrophyGameDetails gameDetails = {0};
						gameDetails.size = sizeof(SceNpTrophyGameDetails);
						sceNpTrophyGetGameInfo(trophyContext,handle,&gameDetails,NULL);
						size = gameDetails.numTrophies;
						printf(" %i\n",size);
						
						paths *trophy_list = malloc(sizeof(paths)*size);
						SceNpTrophyDetails trophyDetails = {0};
						trophyDetails.size = sizeof(SceNpTrophyDetails);
						printf("Obtaining trophy names...");
						for (int i=0; i < size; i++)
						{
							sceNpTrophyGetTrophyInfo(trophyContext,handle,i,&trophyDetails,NULL);
							if(strcmp((char *)trophyDetails.name,"") !=0)
							{
								sprintf(trophy_list[i].path,"%.50s (%i)",trophyDetails.name,i);
							}
							else
							{
								sprintf(trophy_list[i].path,"Hidden Trophy (%i)",i);
							}
						}
						printf("OK!\n");
						sceKernelDelayThread(500000);
selectTrophyMenu:
						psvDebugScreenClear();
						while(1)
							{
								char buf[256];
								strncpy(buf, "**** TropHax StandAlone Edition ****", sizeof(buf));
								printf("\e[%i;%iH%s", 1, CENTERX(buf), buf);
								strncpy(buf, "Choose a trophy:", sizeof(buf));
								printf("\e[%i;%iH%s", 2, CENTERX(buf), buf);
								y = 3;
								int max = (size < WINDOW_HEIGHT) ? size : WINDOW_HEIGHT;
								for (int i=0; i < max; i++) {
									if (i+window == selection)
										psvDebugScreenSetFgColor(0xFF0000);
									printf("\e[%i;%iH%s", y, x, trophy_list[i+window].path);
									y += 1;
									psvDebugScreenSetFgColor(0xFFFFFF);
								}
								strncpy(buf, "U/D: Select Trophy  X: Unlock  O: Return", sizeof(buf));
								printf("\e[%i;%iH%s", SCREEN_ROW, CENTERX(buf), buf);
								memset(&pad, 0, sizeof(pad));
								sceCtrlPeekBufferPositive(0, &pad, 1);
								
								if (pad.buttons == SCE_CTRL_UP)
								{
									if (selection <= size - WINDOW_HEIGHT){
										window -= 1;
									}
									if (window < 0){
										window = 0;
									}
									selection -= 1;
									if (selection < 0)
										selection = 0;
									sceKernelDelayThread(150000);
								}
								if (pad.buttons == SCE_CTRL_DOWN)
								{
									if (selection >= WINDOW_HEIGHT - 1){
										window += 1;
									}
									if (window > size - WINDOW_HEIGHT){
										window = (size - WINDOW_HEIGHT < 0) ? 0 : size - WINDOW_HEIGHT;
									}
									selection += 1;
									if (selection + 1 > size - 1)
										selection = size - 1;
									sceKernelDelayThread(150000);
								}
								if (pad.buttons == SCE_CTRL_CROSS)
								{
												
									psvDebugScreenClear(); // Clear the screen!
									sceKernelDelayThread(200000);
									
									x = 1, y = 1;
									int selection2 = 0;
									int window2 = 0;
									
									int size2 = 2;
									paths *timeList = malloc(sizeof(paths)*size2);

									
									//set options
									strcpy(timeList[0].path, "Use Real SecureTick");
									strcpy(timeList[1].path, "Use Fake SecureTick");
									
									while(1)
										{
										char buf[256];
										strncpy(buf, "**** TropHax StandAlone Edition ****", sizeof(buf));
										printf("\e[%i;%iH%s", 1, CENTERX(buf), buf);
										strncpy(buf, "Choose an Option:", sizeof(buf));
										printf("\e[%i;%iH%s", 2, CENTERX(buf), buf);
										y = 3;
										int max = (size2 < WINDOW_HEIGHT) ? size2 : WINDOW_HEIGHT;
										for (int i=0; i < max; i++) {
											if (i+window2 == selection2)
												psvDebugScreenSetFgColor(0xFF0000);
											printf("\e[%i;%iH%s", y, x, timeList[i+window2].path);
											y += 1;
											psvDebugScreenSetFgColor(0xFFFFFF);
										}
										strncpy(buf, "U/D: Select Option  X: Confirm", sizeof(buf));
										printf("\e[%i;%iH%s", SCREEN_ROW, CENTERX(buf), buf);
										memset(&pad, 0, sizeof(pad));
										sceCtrlPeekBufferPositive(0, &pad, 1);

										if (pad.buttons == SCE_CTRL_UP)
										{
											if (selection2 <= size2 - WINDOW_HEIGHT){
												window2 -= 1;
											}
											if (window2 < 0){
												window2 = 0;
											}
											selection2 -= 1;
											if (selection2 < 0)
												selection2 = 0;
											sceKernelDelayThread(150000);
										}
									if (pad.buttons == SCE_CTRL_DOWN)
										{
											if (selection2 >= WINDOW_HEIGHT - 1){
												window2 += 1;
											}
											if (window2 > size2 - WINDOW_HEIGHT){
												window2 = (size2 - WINDOW_HEIGHT < 0) ? 0 : size2 - WINDOW_HEIGHT;
											}
											selection2 += 1;
											if (selection2 + 1 > size2 - 1)
												selection2 = size2 - 1;
											sceKernelDelayThread(150000);
										}
									if (pad.buttons == SCE_CTRL_CROSS)
									{
										if(selection2 == 0)
										{
											  psvDebugScreenClear();
											  printf("Unlocking trophy %i\n",selection);
											  SceNpTrophyId id = selection;
											  SceNpTrophyId platid;
											  
											  ret = sceNpTrophyUnlockTrophy(trophyContext,handle,id,&platid);
												if(ret < 0){
														if(ret == 0x8055160f)
														{
															printf("Trophy %li is allready unlocked.\n",id);
														}
														else
														{
															printf("sceNpTrophyUnlockTrophy() failed. ret = 0x%x\n", ret);
														}
												}
												else
												{
													printf("Successfully unlocked trophy %li\n",id);
													
													SceNpTrophyDetails trophyDetails = {0};
													trophyDetails.size = sizeof(SceNpTrophyDetails);
													sceNpTrophyGetTrophyInfo(trophyContext,handle,id,&trophyDetails,NULL);
													memset(trophy_list[id].path,0x00,sizeof(trophy_list[id].path));
													sprintf(trophy_list[id].path,"%.50s (%li)",trophyDetails.name,id);
													
												}
												sceKernelDelayThread(500000);
												goto selectTrophyMenu;
										}
									
										if(selection2 == 1)
										{
											psvDebugScreenClear();
											sceKernelDelayThread(150000);
											int selectedPartOfTime = 0;
											
											SceDateTime dateTime;
											memset(&dateTime,0x00,sizeof(SceDateTime));
											

											SceRtcTick fakeTime = {0};
											sceRtcGetCurrentTick(&fakeTime);
											
											sceRtcSetTick(&dateTime,&fakeTime);
											while(1)
											{

												
												char buf[256];
												strncpy(buf, "**** TropHax StandAlone Edition ****", sizeof(buf));
												printf("\e[%i;%iH%s\n", 1, CENTERX(buf), buf);
												printf("Set SecureTick: (DD/MM/YY H:M:S)\n");

												
												if(selectedPartOfTime == 0)
												{
													psvDebugScreenSetFgColor(0xFF0000);
												}
												printf("%02d",dateTime.day);
												psvDebugScreenSetFgColor(0xFFFFFF);
												printf("/");
												
												if(selectedPartOfTime == 1)
												{
													psvDebugScreenSetFgColor(0xFF0000);
												}
												printf("%02d",dateTime.month);
												psvDebugScreenSetFgColor(0xFFFFFF);
												printf("/");
												
												if(selectedPartOfTime == 2)
												{
													psvDebugScreenSetFgColor(0xFF0000);
												}
												printf("%02d",dateTime.year);
												psvDebugScreenSetFgColor(0xFFFFFF);
												printf(" ");
												
												if(selectedPartOfTime == 3)
												{
													psvDebugScreenSetFgColor(0xFF0000);
												}
												printf("%02d",dateTime.hour);
												psvDebugScreenSetFgColor(0xFFFFFF);
												printf(":");
												
												if(selectedPartOfTime == 4)
												{
													psvDebugScreenSetFgColor(0xFF0000);
												}
												printf("%02d",dateTime.minute);
												psvDebugScreenSetFgColor(0xFFFFFF);
												printf(":");
												
												if(selectedPartOfTime == 5)
												{
													psvDebugScreenSetFgColor(0xFF0000);
												}
												printf("%02d",dateTime.second);
												psvDebugScreenSetFgColor(0xFFFFFF);
												printf("\n\nResulting timestamp:\n");
												
												printf("%llx",fakeTime.tick);
												
												strncpy(buf, "U/D: Increment/Decrement L/R: Part of Date  X: Confirm", sizeof(buf));
												printf("\e[%i;%iH%s", SCREEN_ROW, CENTERX(buf), buf);
												
												memset(&pad, 0, sizeof(pad));
												sceCtrlPeekBufferPositive(0, &pad, 1);
												if(pad.buttons == SCE_CTRL_RIGHT)
												{
													selectedPartOfTime ++;
													
													if(selectedPartOfTime >= 5)
													{
														selectedPartOfTime = 5;
													}
													
													sceKernelDelayThread(150000);
												}
												if(pad.buttons == SCE_CTRL_LEFT)
												{
													selectedPartOfTime --;
													
													if(selectedPartOfTime <= 0)
													{
														selectedPartOfTime = 0;
													}
													
													sceKernelDelayThread(150000);
												}
												
												if(pad.buttons == SCE_CTRL_UP)
												{
													if(selectedPartOfTime == 0)
													{
														dateTime.day ++;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.day --;
														}
													}
													if(selectedPartOfTime == 1)
													{
														dateTime.month ++;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.month --;
														}
													}
													if(selectedPartOfTime == 2)
													{
														dateTime.year ++;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.year --;
														}
													}
													if(selectedPartOfTime == 3)
													{
														dateTime.hour ++;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.hour --;
														}
													}
													if(selectedPartOfTime == 4)
													{
														dateTime.minute ++;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.minute --;
														}
													}
													if(selectedPartOfTime == 5)
													{
														dateTime.second ++;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.second --;
														}
													}
													sceKernelDelayThread(150000);
												}
												
												if(pad.buttons == SCE_CTRL_DOWN)
												{
													if(selectedPartOfTime == 0)
													{
														dateTime.day --;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.day ++;
														}
													}
													if(selectedPartOfTime == 1)
													{
														dateTime.month --;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.month ++;
														}
													}
													if(selectedPartOfTime == 2)
													{
														dateTime.year --;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.year ++;
														}
													}
													if(selectedPartOfTime == 3)
													{
														dateTime.hour --;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.hour ++;
														}
													}
													if(selectedPartOfTime == 4)
													{
														dateTime.minute --;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.minute ++;
														}
														
													}
													if(selectedPartOfTime == 5)
													{
														dateTime.second --;
														
														ret = sceRtcGetTick(&dateTime, &fakeTime);
														if(ret < 0)
														{
															dateTime.second ++;
														}
													}
													sceKernelDelayThread(150000);
												}
												
												if(pad.buttons == SCE_CTRL_CROSS)
												{
													psvDebugScreenClear();

												  printf("Unlocking trophy %i\n",selection);
												  SceNpTrophyId id = selection;
												  SceNpTrophyId platid;
												  FakeTimes(1);
												  ret = setSecureTick(fakeTime.tick);
												  ret = sceNpTrophyUnlockTrophy(trophyContext,handle,id,&platid);
													if(ret < 0){
															if(ret == 0x8055160f)
															{
																printf("Trophy %li is allready unlocked.\n",id);
															}
															else
															{
																printf("sceNpTrophyUnlockTrophy() failed. ret = 0x%x\n", ret);
															}
													}
													else
													{
														printf("Successfully unlocked trophy %li\n",id);
														
														SceNpTrophyDetails trophyDetails = {0};
														trophyDetails.size = sizeof(SceNpTrophyDetails);
														sceNpTrophyGetTrophyInfo(trophyContext,handle,id,&trophyDetails,NULL);
														memset(trophy_list[id].path,0x00,sizeof(trophy_list[id].path));
														sprintf(trophy_list[id].path,"%.50s (%li)",trophyDetails.name,id);
														
													}
													
													FakeTimes(0);
												  
													sceKernelDelayThread(500000);
													goto selectTrophyMenu;
													}
											}
										}
									}
									}
							  }
								if (pad.buttons == SCE_CTRL_CIRCLE)
								{
								  sceKernelDelayThread(150000);
								  goto TrophyMenu;
							  }
								if(window != oWindow)
								{
										psvDebugScreenClear();
										oWindow = window;
								  }
							}
						
						
					}
					
					else if(selection == 1)
					{
						
						psvDebugScreenClear(); //clear screen
						SceNpTrophyId id = 0;
						SceNpTrophyId platid;
						
						while (1)
						{
							ret = sceNpTrophyUnlockTrophy(trophyContext,handle,id,&platid);
							if(ret < 0){
									if(ret == 0x8055160f)
									{
										printf("Trophy %li is allready unlocked.\n",id);
									}
									else if(ret == 0x8055160e)
									{
										printf("All trophys unlocked!\n");
										sceKernelDelayThread(500000);
										goto TrophyMenu;
									}
									else
									{
										printf("sceNpTrophyUnlockTrophy() failed. ret = 0x%x\n", ret);
									}
							}
							else
							{
								printf("Successfully unlocked trophy %li\n",id);
							}
							id ++;
						}
					}
					else if(selection == 2)
					{
						psvDebugScreenClear();
						ret = sceNpTrophyDestroyContext(trophyContext);
						if(ret < 0){
								printf("sceNpTrophyDestroyContext() failed. ret = 0x%x\n", ret);
								while(1){};
						}
						
						ret = sceNpTrophyDestroyHandle(handle);
						if(ret < 0){
								printf("sceNpTrophyDestroyHandle() failed. ret = 0x%x\n", ret);
								while(1){};
						}
						
						ret = sceNpTrophyTerm();
						if(ret < 0){
								printf("sceNpTrophyTerm() failed. ret = 0x%x\n", ret);
								while(1){};
						}
						
						
						sceKernelStopUnloadModule(user_modid, 0, NULL, 0, NULL, NULL);
						taiStopUnloadKernelModule(kernel_modid, 0, NULL, 0, NULL, NULL); 
						sceKernelDelayThread(150000);
						goto start;
					}
				}
					
			}
		

                

                
			}
		
		if(window != oWindow)
		{
			psvDebugScreenClear();
			oWindow = window;
		}
	
	}while (pad.buttons != SCE_CTRL_CIRCLE);
    



    sceKernelExitProcess(0);
    return 0;
}

