#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <libfat/fat.h>
#include <usb/usbmain.h>
#include <xenos/xenos.h>
#include <console/console.h>
#include <elf/elf.h>

int main(){
    // Turn on the GPU
    xenos_init(VIDEO_MODE_AUTO);
	
    // Turn on the text console
    console_init();

    // Initialize any/all connected USB devices with fat32 filesystems
    usb_init();
    usb_do_poll();
    fatInitDefault();

    // File pointer for the xell stage2 file.
    FILE *xell_fptr;

    // File name to look for.
    // default: stage2.elf32
    // If this fails, each xell binary variant will also be tried (xell-1f.bin, xell-2f.bin, xell-gggggg.bin)
    char *xell_filename = "stage2.elf32";

    // Full file path for the xell file.
    char *xell_file_path = "";

    // Check if a file named "stage2.elf32" exists on any fat32 formatted USB drive.
    // There can only be a maximum of 3 connected fat32 devices in libxenon.
    for(int i = 0; i <= 2; i++){
        // Track the iteration.
        char disk_letter = 'a';
        if(i == 0){
            disk_letter = 'a';
        } else if(i == 1){
            disk_letter = 'b';
        } else if(i == 2){
            disk_letter = 'c';
        }

        // Loads the formatted filename string into memory.
        asprintf(&xell_file_path, "ud%c0:/%s", disk_letter, xell_filename);

        xell_fptr = fopen(xell_file_path, "rb");
        if(xell_fptr == NULL){ // If there is an error opening the file:
            printf("Could not open: ud%c0:/%s\n", disk_letter, xell_filename);
        } else{ // If the file was found, try loading it.
            printf("Opening file: ud%c0:/%s", disk_letter, xell_filename);
           	elf_runFromDisk(xell_file_path);
            fclose(xell_fptr);
        }
    }

    // Either XeLL will reload from NAND as per libxenon builtin functionality,
    // or this just hangs forever until a hard reboot.
    exit(0);
}