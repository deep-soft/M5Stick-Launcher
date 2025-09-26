#ifndef __SD_FUNCTIONS_H
#define __SD_FUNCTIONS_H

#include <FS.h>
#include <SD_MMC.h>
#include <SD.h>
#include <SPI.h>
#include <FFat.h>
#include <CustomUpdate.h>

extern SPIClass sdcardSPI;
#ifndef PART_04MB
bool eraseFAT();
#endif
bool setupSdCard();

void closeSdCard();

bool ToggleSDCard();

bool deleteFromSd(String path);

bool renameFile(String path, String filename);

bool copyFile(String path);

bool pasteFile(String path);

bool createFolder(String path);

void readFs(String folder, String result[][3]);

void sortList(String fileList[][3], int fileListCount);

String loopSD(bool filePicker = false);

void performUpdate(Stream &updateSource, size_t updateSize, int command);

void updateFromSD(String path);

bool performFATUpdate(Stream &updateSource, size_t updateSize,  const char *label = "vfs");
#endif