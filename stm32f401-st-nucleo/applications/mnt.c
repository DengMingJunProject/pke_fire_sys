/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-08-08     Yang        the first version
 */

#include <rtthread.h>
#include <dfs.h>
#include <dfs_fs.h>
#include <fal.h>

#ifdef RT_USING_DFS_ROMFS
#include <dfs_romfs.h>
#endif

#ifdef RT_USING_DFS_ROMFS
#define SD_ROOT     "/sdcard"
#else
#define SD_ROOT     "/"
#endif

void fal_mount(char * partition, char *director, char *type)
{
	if( strcmp(type,"lfs")==0 ){
		struct rt_device *flash_dev = fal_mtd_nor_device_create(partition);
		struct statfs fsbuf;
		int diskfree=0;
		
		if( *director != '/' ){
			log_i("%s directory format not correct\r\n",director);
			return;
		}
		
		if (flash_dev == NULL){
			
			log_e("Can't create a block device on '%s' partition.\r\n", partition);
		}
		if (dfs_mount(flash_dev->parent.name, director, "lfs", 0, 0) == 0){
			
			statfs(director,&fsbuf);
			diskfree = fsbuf.f_bfree*fsbuf.f_bsize;	
			log_i("%s mount lfs OK! free size %d\r\n",flash_dev->parent.name,diskfree);
		}
		else{
			dfs_mkfs("lfs", flash_dev->parent.name); 
			log_e("Failed to initialize %s!\n",director);
			log_d("You should create a filesystem on the block device first!\r\n");
		}
	}
	else if( strcmp(type,"elm")==0 ){
		struct rt_device *flash_dev = fal_blk_device_create(partition);
		struct statfs fsbuf;
		int diskfree=0;
		
		if( *director != '/' ){
			log_i("%s directory format not correct\r\n",director);
			return;
		}
		
		if (flash_dev == NULL){
			
			log_e("Can't create a block device on '%s' partition.\r\n", partition);
		}
		if (dfs_mount(flash_dev->parent.name, director, "elm", 0, 0) == 0){
			
			statfs(director,&fsbuf);
			diskfree = fsbuf.f_bfree*fsbuf.f_bsize;	
			log_i("%s mount elm OK! free size %d\r\n",flash_dev->parent.name,diskfree);
		}
		else{
			dfs_mkfs("elm", flash_dev->parent.name); 
			log_e("Failed to initialize %s!\n",director);
			log_d("You should create a filesystem on the block device first!\r\n");
		}
	}
}

int mnt_init(void)
{
#ifdef RT_USING_DFS_ROMFS
    /* initialize the device filesystem */
//    dfs_init();

//    dfs_romfs_init();

//    /* mount rom file system */
//	if (dfs_mount(RT_NULL, "/", "rom", 0, &(romfs_root)) == 0)
//    {
//        rt_kprintf("ROM file system initializated!\n");
//    }
#endif

#ifdef BSP_DRV_SDCARD
    /* initilize sd card */
     mci_hw_init("sd0");
#endif

//#ifdef RT_DFS_ELM_REENTRANT    
	
	if (dfs_mount(RT_NULL, "/", "rom", 0, &(romfs_root)) == 0){
		log_i("ROM System initialized!\n");
	}
	else{
		log_e("ROM System init failed!\n");
	}
	
	fal_mount("filesystem","/flash","lfs");

	/* mount flash0 partition 1 as flash0 directory */
//	uint8_t try_times = 3;
//    if (dfs_mount("norflash", "flash", "lfs", 0, 0) == 0)
//        rt_kprintf("File System initialized!\n");
//    else
//    {
//    	do{
//    		if (dfs_mount("norflash", "flash", "lfs", 0, 0) == 0)
//    		{	
//    			rt_kprintf("File System initialized!\n");
//				break;
//    		}
//		}while (--try_times);
//		
//		if (try_times == 0)
//		{
//			rt_kprintf("Format norflash0 as lfs file system... \n");
//			dfs_mkfs("lfs", "norflash");
//			if (dfs_mount("norflash", "flash", "lfs", 0, 0) == 0)
//        		rt_kprintf("File System initialized!\n");
//		}
//    }
//	/* mount flash1 partition 1 as flash1 directory */
//	try_times = 3;
//    if (dfs_mount("flash1", "flash1", "elm", 0, 0) == 0)
//        rt_kprintf("File System initialized!\n");
//    else
//	{
//    	do{
//    		if (dfs_mount("flash1", "flash1", "elm", 0, 0) == 0)
//    		{	
//    			rt_kprintf("File System initialized!\n");
//				break;
//    		}
//		}while (--try_times);

//		if (try_times == 0)
//		{
//			rt_kprintf("Format flash1 as elm file system... \n");
//			dfs_mkfs("elm", "flash1");
//			if (dfs_mount("flash1", "flash1", "elm", 0, 0) == 0)
//        		rt_kprintf("File System initialized!\n");
//		}
//    }
//#endif
    
    return 0;
}
INIT_ENV_EXPORT(mnt_init);
