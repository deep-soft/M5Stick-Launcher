#include "sd_functions.h"
#include "display.h"
#include "esp_log.h"
#include "mykeyboard.h"
#include <esp_flash.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <globals.h>
SPIClass sdcardSPI;
String fileToCopy;
// Protected global variables
String fileList[MAXFILES][3];

#ifndef PART_04MB
/***************************************************************************************
** Function name: eraseFAT
** Description:   erase FAT partition to micropython compatibilities
***************************************************************************************/
bool eraseFAT() {
    esp_err_t err;

    // Find FAT partition with its name
    const esp_partition_t *partition =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "vfs");
    if (!partition) {
        // log_e("Failed to find partition");
        return false;
    }

    // erase all FAT partition
    err = esp_flash_erase_region(NULL, partition->address, partition->size);
    if (err != ESP_OK) {
        // log_e("Failed to erase partition: %s", esp_err_to_name(err));
        return false;
    }

    // Find FAT partition with its name
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "sys");
    if (!partition) {
        // log_e("Failed to find partition");
        goto Exit;
    }

    // erase all FAT partition
    err = esp_flash_erase_region(NULL, partition->address, partition->size);
    if (err != ESP_OK) { return false; }
Exit:
    return true;
}
#endif
/***************************************************************************************
** Function name: setupSdCard
** Description:   Start SD Card
***************************************************************************************/
bool setupSdCard() {
#if !defined(SDM_SD) // fot Lilygo T-Display S3 with lilygo shield
    if (!SD_MMC.begin("/sdcard", true))
#elif (TFT_MOSI == SDCARD_MOSI)
    if (!SDM.begin(SDCARD_CS)) // https://github.com/Bodmer/TFT_eSPI/discussions/2420
#elif defined(HEADLESS)
    if (_sck == 0 && _miso == 0 && _mosi == 0 && _cs == 0) {
        Serial.println("SdCard pins not set");
        return false;
    }

    sdcardSPI.begin(_sck, _miso, _mosi, _cs); // start SPI communications
    delay(10);
    if (!SDM.begin(_cs, sdcardSPI))
#elif defined(DONT_USE_INPUT_TASK)
#if (TFT_MOSI != SDCARD_MOSI)
    sdcardSPI.begin(SDCARD_SCK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS); // start SPI communications
    if (!SDM.begin(SDCARD_CS, sdcardSPI))
#else
    if (!SDM.begin(SDCARD_CS))
#endif

#else
    sdcardSPI.begin(SDCARD_SCK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS); // start SPI communications
    delay(10);
    if (!SDM.begin(SDCARD_CS, sdcardSPI))
#endif
    {
        // sdcardSPI.end(); // Closes SPI connections and release pin header.
        Serial.println("Failed to mount SDCARD");
        sdcardMounted = false;
        return false;
    } else {
        Serial.println("SDCARD mounted successfully");
        sdcardMounted = true;
        return true;
    }
}

/***************************************************************************************
** Function name: closeSdCard
** Description:   Turn Off SDCard, set sdcardMounted state to false
***************************************************************************************/
void closeSdCard() {
    SDM.end();
    sdcardMounted = false;
}

/***************************************************************************************
** Function name: ToggleSDCard
** Description:   Turn Off or On the SDCard, return sdcardMounted state
***************************************************************************************/
bool ToggleSDCard() {
    if (sdcardMounted == true) {
        closeSdCard();
        sdcardMounted = false;
        return false;
    } else {
        sdcardMounted = setupSdCard();
        return sdcardMounted;
    }
}
/***************************************************************************************
** Function name: deleteFromSd
** Description:   delete file or folder
***************************************************************************************/
bool deleteFromSd(String path) {
    File dir = SDM.open(path);
    if (!dir.isDirectory()) { return SDM.remove(path.c_str()); }

    dir.rewindDirectory();
    bool success = true;

    File file = dir.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            success &= deleteFromSd(file.path());
        } else {
            success &= SDM.remove(file.path());
        }
        file = dir.openNextFile();
    }

    dir.close();
    // Apaga a própria pasta depois de apagar seu conteúdo
    success &= SDM.rmdir(path.c_str());
    return success;
}

/***************************************************************************************
** Function name: renameFile
** Description:   rename file or folder
***************************************************************************************/
bool renameFile(String path, String filename) {
    String newName = keyboard(filename, 76, "Type the new Name:");
    if (!setupSdCard()) {
        // Serial.println("Falha ao inicializar o cartão SD");
        return false;
    }

    // Rename the file of folder
    if (SDM.rename(path, path.substring(0, path.lastIndexOf('/')) + "/" + newName)) {
        // Serial.println("Renamed from " + filename + " to " + newName);
        return true;
    } else {
        // Serial.println("Fail on rename.");
        return false;
    }
}

/***************************************************************************************
** Function name: copyFile
** Description:   copy file address to memory
***************************************************************************************/
bool copyFile(String path) {
    if (!setupSdCard()) {
        // Serial.println("Fail to start SDCard");
        return false;
    }
    File file = SDM.open(path, FILE_READ);
    if (!file.isDirectory()) {
        fileToCopy = path;
        file.close();
        return true;
    } else {
        displayRedStripe("Cannot copy Folder");
        file.close();
        return false;
    }
}

/***************************************************************************************
** Function name: pasteFile
** Description:   paste file to new folder
***************************************************************************************/
bool pasteFile(String path) {
    // Tamanho do buffer para leitura/escrita
    const size_t bufferSize = 2048 * 2; // Ajuste conforme necessário para otimizar a performance
    uint8_t buffer[bufferSize];

    // Abrir o arquivo original
    File sourceFile = SDM.open(fileToCopy, FILE_READ);
    if (!sourceFile) {
        // Serial.println("Falha ao abrir o arquivo original para leitura");
        return false;
    }

    // Criar o arquivo de destino
    File destFile = SDM.open(path + "/" + fileToCopy.substring(fileToCopy.lastIndexOf('/') + 1), FILE_WRITE);
    if (!destFile) {
        // Serial.println("Falha ao criar o arquivo de destino");
        sourceFile.close();
        return false;
    }

    // Ler dados do arquivo original e escrever no arquivo de destino
    size_t bytesRead;
    int tot = sourceFile.size();
    int prog = 0;
    // tft->drawRect(5,tftHeight-12, (tftWidth-10), 9, FGCOLOR);
    while ((bytesRead = sourceFile.read(buffer, bufferSize)) > 0) {
        if (destFile.write(buffer, bytesRead) != bytesRead) {
            // Serial.println("Falha ao escrever no arquivo de destino");
            sourceFile.close();
            destFile.close();
            return false;
        } else {
            prog += bytesRead;
            float rad = 360 * prog / tot;
            tft->drawArc(tftWidth / 2, tftHeight / 2, tftHeight / 4, tftHeight / 5, 0, int(rad), ALCOLOR);
            // tft->fillRect(7,tftHeight-10, (tftWidth-14)*prog/tot, 5, FGCOLOR);
        }
    }

    // Fechar ambos os arquivos
    sourceFile.close();
    destFile.close();
    return true;
}

/***************************************************************************************
** Function name: createFolder
** Description:   create new folder
***************************************************************************************/
bool createFolder(String path) {
    String foldername = keyboard("", 76, "Folder Name: ");
    if (!setupSdCard()) {
        // Serial.println("Fail to start SDCard");
        return false;
    }
    if (!SDM.mkdir(path + foldername)) {
        displayRedStripe("Couldn't create folder");
        return false;
    }
    return true;
}

/***************************************************************************************
** Function name: sortList
** Description:   sort files for name
***************************************************************************************/
void sortList(String fileList[][3], int fileListCount) {
    bool swapped;
    String temp[3];
    String name1, name2;

    do {
        swapped = false;
        for (int i = 0; i < fileListCount - 1; i++) {
            name1 = fileList[i][0];
            name1.toUpperCase();
            name2 = fileList[i + 1][0];
            name2.toUpperCase();

            // Verificar se ambos são pastas ou arquivos
            bool isFolder1 = fileList[i][2] == "folder";
            bool isFolder2 = fileList[i + 1][2] == "folder";

            // Primeiro, ordenar pastas
            if (isFolder1 && !isFolder2) {
                continue; // Se o primeiro for uma pasta e o segundo não, não troque
            } else if (!isFolder1 && isFolder2) {
                // Se o primeiro for um arquivo e o segundo uma pasta, troque
                for (int j = 0; j < 3; j++) {
                    temp[j] = fileList[i][j];
                    fileList[i][j] = fileList[i + 1][j];
                    fileList[i + 1][j] = temp[j];
                }
                swapped = true;
            } else {
                // Ambos são pastas ou arquivos, então ordenar alfabeticamente
                if (name1.compareTo(name2) > 0) {
                    for (int j = 0; j < 2; j++) {
                        temp[j] = fileList[i][j];
                        fileList[i][j] = fileList[i + 1][j];
                        fileList[i + 1][j] = temp[j];
                    }
                    swapped = true;
                }
            }
        }
    } while (swapped);
}

/***************************************************************************************
** Function name: sortList
** Description:   sort files for name
***************************************************************************************/
void readFs(String folder, String result[][3]) {
    int allFilesCount = 0;
    while (allFilesCount < MAXFILES) {
        result[allFilesCount][0] = "";
        result[allFilesCount][1] = "";
        result[allFilesCount][2] = "";
        allFilesCount++;
    }
    allFilesCount = 0;

    if (!setupSdCard()) {
        // Serial.println("Falha ao iniciar o cartão SD");
        displayRedStripe("SD not found or not formatted in FAT32");
        delay(2500);
        return; // Retornar imediatamente em caso de falha
    }

    File root = SDM.open(folder);
    if (!root || !root.isDirectory()) {
        displayRedStripe("Fail open root");
        delay(2500);
        return; // Retornar imediatamente se não for possível abrir o diretório
    }

    File file2;
    file2 = root.openNextFile();
    while (file2 && allFilesCount < (MAXFILES - 1)) {
        String fileName = file2.name();
        if (!file2.isDirectory()) {
            String ext = fileName.substring(fileName.lastIndexOf(".") + 1);
            ext.toUpperCase();
            if (onlyBins) {
                if (ext.equals("BIN")) {
                    result[allFilesCount][0] = fileName.substring(fileName.lastIndexOf("/") + 1);
                    result[allFilesCount][1] = file2.path();
                    result[allFilesCount][2] = "file";
                    allFilesCount++;
                }
            } else {
                result[allFilesCount][0] = fileName.substring(fileName.lastIndexOf("/") + 1);
                result[allFilesCount][1] = file2.path();
                result[allFilesCount][2] = "file";
                allFilesCount++;
            }
        }

        file2 = root.openNextFile();
    }
    file2.close();
    root.close();

    root = SDM.open(folder);
    File file = root.openNextFile();
    while (file && allFilesCount < (MAXFILES - 1)) {
        String fileName = file.name();
        if (file.isDirectory()) {
            result[allFilesCount][0] = fileName.substring(fileName.lastIndexOf("/") + 1);
            result[allFilesCount][1] = file.path();
            result[allFilesCount][2] = "folder";
            allFilesCount++;
        }
        file = root.openNextFile();
    }
    file.close();
    root.close();

    // Ordenar os arquivos e pastas
    sortList(result, allFilesCount);
    // allFilesCount++;
    result[allFilesCount][0] = "> Back";
    folder = folder.substring(0, folder.lastIndexOf('/'));
    if (folder == "") folder = "/";
    result[allFilesCount][1] = folder;
    result[allFilesCount][2] = "operator";
}
/*********************************************************************
**  Function: loopSD
**  Where you choose what to do wuth your SD Files
**********************************************************************/
String loopSD(bool filePicker) {
    Opt_Coord coord;
    std::vector<MenuOptions> list;
    int max_idx = 0;
    int min_idx = 255;

    prog_handler = 0;
    String result = "";
    bool reload = false;
    bool redraw = true;
    int index = 0;
    int maxFiles = 0;
    String Folder = "/";
    String PreFolder = "/";
    tft->fillScreen(BGCOLOR);
    tft->drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, FGCOLOR);

    readFs(Folder, fileList);
    coord = listFiles(0, fileList, list);

    for (int i = 0; i < MAXFILES; i++)
        if (fileList[i][2] != "") maxFiles++;
        else break;
    LongPressTmp = millis();
    while (1) {
        if (returnToMenu) break; // stop this loop and retur to the previous loop

        if (redraw) {
            if (strcmp(PreFolder.c_str(), Folder.c_str()) != 0 || reload) {
                index = 0;
                readFs(Folder, fileList);
                PreFolder = Folder;
                maxFiles = 0;
                for (int i = 0; i < MAXFILES; i++)
                    if (fileList[i][2] != "") maxFiles++;
                    else break;
                reload = false;
                tft->fillRoundRect(6, 6, tftWidth - 12, tftHeight - 12, 5, BGCOLOR);
                tft->fillRoundRect(6, 6, tftWidth - 12, tftHeight - 12, 5, BGCOLOR);
            }
            coord = listFiles(index, fileList, list);

            // Serial.println("\nContent of list object:");
            max_idx = 0;
            min_idx = MAXFILES;
            int tmp = 0;
            for (auto item : list) {
                if (item.name != "") {
                    tmp = item.name.toInt();
                    // Serial.print(tmp); Serial.print(" ");
                    if (tmp > max_idx) max_idx = tmp;
                    if (tmp < min_idx) min_idx = tmp;
                }
            }
            // Serial.printf("\nmax_idx: %d min_idx: %d\n", max_idx, min_idx);

            redraw = false;
        }

        displayScrollingText(fileList[index][0], coord);

#ifdef HAS_TOUCH
        if (touchPoint.pressed) {
            for (auto item : list) {
                if (item.contain(touchPoint.x, touchPoint.y)) {
                    SelPress = false;
                    PrevPress = false;
                    NextPress = false;
                    UpPress = false;
                    DownPress = false;
                    EscPress = false;
                    if (item.name == "") {
                        if (item.text == "+") index = max_idx + 1;
                        if (item.text == "-") index = min_idx - 1;
                        if (index < 0) index = 0;
                        // Serial.printf("\nPressed [%s], next index: %d\n",item.text,index);
                        redraw = true;
                        break;
                    } else {
                        if (index == item.name.toInt()) SelPress = true;
                        else redraw = true;
                        index = item.name.toInt();
                        break;
                    }
                }
            }
        }
#endif
        if (check(PrevPress) || check(UpPress)) {
            if (index == 0) index = maxFiles - 1;
            else if (index > 0) index--;
            redraw = true;
        }
        /* DW Btn to next item */
        if (check(NextPress) || check(DownPress)) {
            index++;
            if (index == maxFiles) index = 0;
            redraw = true;
        }

/* Select to install */
#ifndef E_PAPER_DISPLAY
        if (LongPress || SelPress) {
            if (!LongPress) {
                LongPress = true;
                LongPressTmp = millis();
            }
            if (LongPress && millis() - LongPressTmp < 200) goto WAITING;
            LongPress = false;

            if (check(SelPress)) {
                while (check(SelPress)) yield();
#else
        if (check(SelPress)) {
            if (true) {
#endif
                // Definição da matriz "Options"
                if (fileList[index][2] == "folder") {
                    options = {
#ifdef E_PAPER_DISPLAY
                        {"Open Folder", [&]() { Folder = fileList[index][1]; }                       },
#endif
                        {"New Folder",  [=]() { createFolder(Folder); }                              },
                        {"Rename",      [=]() { renameFile(fileList[index][1], fileList[index][0]); }},
                        {"Delete",      [=]() { deleteFromSd(fileList[index][1]); }                  },
                        {"Main Menu",   [=]() { returnToMenu = true; }                               },
                    };
                    loopOptions(options);
                    tft->drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, FGCOLOR);
                    reload = true;
                    redraw = true;
                } else if (fileList[index][2] == "file") {
                    goto Files;
                } else {
                    bool bkf = false;
                    options = {
#ifdef E_PAPER_DISPLAY
                        {"Back Folder", [&]() { bkf = true; }          },
#endif
                        {"New Folder",  [=]() { createFolder(Folder); }},
                    };
                    if (fileToCopy != "") options.push_back({"Paste", [=]() { pasteFile(Folder); }});
                    options.push_back({"Main Menu", [=]() { returnToMenu = true; }});
                    loopOptions(options);
                    if (bkf) goto BACK_FOLDER;
                    reload = true;
                    redraw = true;
                }
            } else {
            Files:
                if (fileList[index][2] == "folder") {
                    Folder = fileList[index][1];
                    redraw = true;
                } else if (fileList[index][2] == "file") {
                    options = {
                        {"Install",    [=]() { updateFromSD(fileList[index][1]); }                  },
                        {"New Folder", [=]() { createFolder(Folder); }                              },
                        {"Rename",     [=]() { renameFile(fileList[index][1], fileList[index][0]); }},
                        {"Copy",       [=]() { copyFile(fileList[index][1]); }                      },
                    };
                    if (fileToCopy != "") options.push_back({"Paste", [=]() { pasteFile(Folder); }});
                    options.push_back({"Delete", [=]() { deleteFromSd(fileList[index][1]); }});
                    options.push_back({"Main Menu", [=]() { returnToMenu = true; }});

                    if (!filePicker) loopOptions(options);
                    else {
                        result = fileList[index][1];
                        break;
                    }
                    reload = true;
                    redraw = true;
                } else {
                BACK_FOLDER:
                    if (Folder == "/") break;
                    while (fileList[index][2] != "operator") index++; // to reach pre_folder
                    Folder = fileList[index][1];
                    index = 0;
                    redraw = true;
                }
                redraw = true;
            }
            tft->fillRoundRect(6, 6, tftWidth - 12, tftHeight - 12, 5, BGCOLOR);
            tft->drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, FGCOLOR);
            redraw = true;
        }
    WAITING:
        yield();

        if (check(EscPress)) goto BACK_FOLDER;
    }
    // clear fileList memory
    for (int i = 0; i < MAXFILES; i++) {
        fileList[i][0] = "";
        fileList[i][1] = "";
        fileList[i][2] = "";
    }
    closeSdCard();
    setupSdCard();
    tft->fillScreen(BGCOLOR);
    return result;
}
/***************************************************************************************
** Function name: performUpdate
** Description:   this function performs the update
***************************************************************************************/
void performUpdate(Stream &updateSource, size_t updateSize, int command) {
    // command = U_FAT_vfs = 300
    // command = U_FAT_sys = 400
    // command = U_SPIFFS = 100
    // command = U_FLASH = 0

    tft->fillRoundRect(6, 6, tftWidth - 12, tftHeight - 12, 5, BGCOLOR);
    progressHandler(0, 500);

    if (Update.begin(updateSize, command)) {
        int written = 0;
        uint8_t buf[1024];
        int bytesRead;

        prog_handler = 0; // Install flash update
        if (command == U_SPIFFS || command == U_FAT_vfs || command == U_FAT_sys)
            prog_handler = 1; // Install flash update
        log_i("updateSize = %d", updateSize);
        while (written < updateSize) { // updateSource.available() > 0 &&
            bytesRead = updateSource.readBytes(buf, sizeof(buf));
            written += Update.write(buf, bytesRead);
            progressHandler(written, updateSize);
        }
        if (Update.end()) {
            if (Update.isFinished()) log_i("Update successfully completed.");
            else log_i("Update not finished? Something went wrong!");
        } else {
            log_i("Error Occurred. Error #: %s", String(Update.getError()));
        }
    } else {
        uint8_t error = Update.getError();
        displayRedStripe("E:" + String(error) + "-Wrong Partition Scheme");
        delay(2500);
    }
}

/***************************************************************************************
** Function name: updateFromSD
** Description:   this function analyse the .bin and calls performUpdate
***************************************************************************************/
void updateFromSD(String path) {
    uint8_t firstThreeBytes[16];
    uint32_t spiffs_offset = 0;
    uint32_t spiffs_size = 0;
    uint32_t app_size = 0;
    bool spiffs = false;
    uint32_t fat_offset_sys = 0;
    uint32_t fat_size_sys = 0;
    uint32_t fat_offset_vfs = 0;
    uint32_t fat_size_vfs = 0;
    bool fat = false;

    File file = SDM.open(path);

    if (!file) goto Exit;
    if (!file.seek(0x8000)) goto Exit;
    file.read(firstThreeBytes, 16);

    if (firstThreeBytes[0] != 0xAA || firstThreeBytes[1] != 0x50 || firstThreeBytes[2] != 0x01) {
        if (!file.seek(0x0)) goto Exit;
        performUpdate(file, file.size(), U_FLASH);
        file.close();
        tft->fillScreen(BGCOLOR);
        FREE_TFT
        const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
            esp_ota_set_boot_partition(partition);
        ESP.restart();
    } else {
        if (!file.seek(0x8000)) goto Exit;
        for (int i = 0; i < 0x0A0; i += 0x20) {
            if (!file.seek(0x8000 + i)) goto Exit;
            file.read(firstThreeBytes, 16);

            if ((firstThreeBytes[0x03] == 0x00 || firstThreeBytes[0x03] == 0x10 ||
                 firstThreeBytes[0x03] == 0x20) &&
                firstThreeBytes[0x06] == 0x01) {
                app_size = (firstThreeBytes[0x0A] << 16) | (firstThreeBytes[0x0B] << 8) | 0x00;
                if (file.size() < (app_size + 0x10000)) app_size = file.size() - 0x10000;
                else if (app_size > MAX_APP) app_size = MAX_APP;
            }

            if (firstThreeBytes[3] == 0x82) {
                spiffs_offset =
                    (firstThreeBytes[0x06] << 16) | (firstThreeBytes[0x07] << 8) | firstThreeBytes[0x08];
                spiffs_size = (firstThreeBytes[0x0A] << 16) | (firstThreeBytes[0x0B] << 8) | 0x00;
                if (file.size() < spiffs_offset) spiffs = false;
                else if (spiffs_size > MAX_SPIFFS) {
                    spiffs_size = MAX_SPIFFS;
                    spiffs = true;
                }
                if (spiffs && file.size() < (spiffs_offset + spiffs_size))
                    spiffs_size = file.size() - spiffs_offset;
            }

            if (firstThreeBytes[3] == 0x81 && firstThreeBytes[0x0C] == 0x73) {
                fat_offset_sys =
                    (firstThreeBytes[0x06] << 16) | (firstThreeBytes[0x07] << 8) | firstThreeBytes[0x08];
                fat_size_sys = (firstThreeBytes[0x0A] << 16) | (firstThreeBytes[0x0B] << 8) | 0x00;
                if (file.size() < fat_offset_sys) fat = false;
                else fat = true;
                if (fat && fat_size_sys > MAX_FAT_sys) fat_size_sys = MAX_FAT_sys;
                if (fat && file.size() < (fat_offset_sys + fat_size_sys))
                    fat_size_sys = file.size() - fat_offset_sys;
            }

            if (firstThreeBytes[3] == 0x81 && firstThreeBytes[0x0C] == 0x76) {
                fat_offset_vfs =
                    (firstThreeBytes[0x06] << 16) | (firstThreeBytes[0x07] << 8) | firstThreeBytes[0x08];
                fat_size_vfs = (firstThreeBytes[0x0A] << 16) | (firstThreeBytes[0x0B] << 8) | 0x00;
                if (file.size() < fat_offset_vfs) fat = false;
                else fat = true;
                if (fat && fat_size_vfs > MAX_FAT_vfs) fat_size_vfs = MAX_FAT_vfs;
                if (fat && file.size() < (fat_offset_vfs + fat_size_vfs))
                    fat_size_vfs = file.size() - fat_offset_vfs;
            }
        }

        log_i("Appsize: %d", app_size);
        log_i("Spiffsize: %d", spiffs_size);
        log_i("FATsize[0]: %d - max: %d at offset: %d", fat_size_sys, MAX_FAT_sys, fat_offset_sys);
        log_i("FATsize[1]: %d - max: %d at offset: %d", fat_size_vfs, MAX_FAT_vfs, fat_offset_vfs);
        log_i("FAT: %d", fat);
        log_i("------------------------");

        if (!fat) {
            fat_size_sys = 0;
            fat_size_vfs = 0;
            fat_offset_sys = 0;
            fat_offset_vfs = 0;
        }

        prog_handler = 0; // Install flash update
        if (spiffs && askSpiffs) {
            options = {
                {"SPIFFS No",  [&]() { spiffs = false; }},
                {"SPIFFS Yes", [&]() { spiffs = true; } },
            };

            loopOptions(options);
            tft->fillRoundRect(6, 6, tftWidth - 12, tftHeight - 12, 5, BGCOLOR);
        }

        log_i("Appsize: %d", app_size);
        log_i("Spiffsize: %d", spiffs_size);
        log_i("FATsize[0]: %d - max: %d at offset: %d", fat_size_sys, MAX_FAT_sys, fat_offset_sys);
        log_i("FATsize[1]: %d - max: %d at offset: %d", fat_size_vfs, MAX_FAT_vfs, fat_offset_vfs);

        if (!file.seek(0x10000)) goto Exit;
        performUpdate(file, app_size, U_FLASH);

        prog_handler = 1; // Install SPIFFS update
        if (spiffs) {
            if (!file.seek(spiffs_offset)) goto Exit;
            performUpdate(file, spiffs_size, U_SPIFFS);
        }

        if (fat) {
            displayRedStripe("Formating FAT");
            if (fat_size_sys > 0) {
                if (!file.seek(fat_offset_sys)) goto Exit;
                if (!performFATUpdate(file, fat_size_sys, "sys")) log_i("FAIL updating FAT sys");
                else displayRedStripe("sys FAT complete");
            }
            displayRedStripe("Formating FAT");
            if (fat_size_vfs > 0) {
                if (!file.seek(fat_offset_vfs)) goto Exit;
                if (!performFATUpdate(file, fat_size_vfs, "vfs")) log_i("FAIL updating FAT vfs");
                else displayRedStripe("vfs FAT complete");
            }
        }
        displayRedStripe("Complete");
        delay(1000);
        FREE_TFT
        const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
            esp_ota_set_boot_partition(partition);
        ESP.restart();
    }
Exit:
    displayRedStripe("Update Error.");
    delay(2500);
}

/***************************************************************************************
** Function name: performFATUpdate
** Description:   this function performs the update
***************************************************************************************/
uint8_t buffer2[1024];

bool performFATUpdate(Stream &updateSource, size_t updateSize, const char *label) {
    // Preencher o buffer com 0xFF
    memset(buffer2, 0x00, sizeof(buffer2));
    const esp_partition_t *partition;
    esp_err_t error;
    size_t paroffset = 0;
    int written = 0;
    int bytesRead = 0;
    error = esp_flash_set_chip_write_protect(NULL, false);

    if (error != ESP_OK) {
        log_i("Protection error: %d", error);
        // return false;
    }

    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, label);
    if (!partition) {
        error = UPDATE_ERROR_NO_PARTITION;
        return false;
    }

    log_i("Start updating: %s", partition->label);
    paroffset = partition->address;
    log_i("Erasing updating: %s from: %d with size: %d", label, paroffset, updateSize);

    error = esp_flash_erase_region(NULL, partition->address, updateSize);
    if (error != ESP_OK) {
        log_i("Erase error %d", error);
        return false;
    }

    progressHandler(0, 500);
    displayRedStripe("Updating FAT");
    log_i("Updating updating: %s", label);

    while (written < updateSize) { // updateSource.available() &&
        bytesRead = updateSource.readBytes(buffer2, sizeof(buffer2));
        error = esp_flash_write(NULL, buffer2, paroffset, bytesRead);
        if (error != ESP_OK) {
            log_i("[FLASH] Failed to write to flash (0x%x)", error);
            return false;
        }
        if (bytesRead == 0) break; // Evitar loop infinito se não houver bytes para ler
        paroffset += bytesRead;
        written += bytesRead;
        progressHandler(written, updateSize);
    }

    if (written == updateSize) {
        log_i("Success updating %s", label);
    } else {
        log_i("FAIL updating %s", label);
        return false;
    }

    return true;
}
