#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <console/console.h>
#include <console/telnet_console.h> // recv_buf
#include <xenos/xenos.h>
#include <input/input.h>
#include <network/network.h>
#include <xenon_uart/xenon_uart.h>
#include <usb/usbmain.h>
#include <xenon_smc/xenon_smc.h>

int main(){
	xenos_init(VIDEO_MODE_AUTO);
	console_init();
	usb_init();
	network_init();
	network_print_config();

	printf("On controller, UART or telnet: Press X to reload XeLL. Y to shutdown. B to reboot.\n");
	printf("Telnet requires a newline to be sent.\n\n");
	telnet_console_init(); // redirect printf output here

	// Initialize 360 controller - taken from XeLL kbootconf.c
	struct controller_data_s ctrl;
	struct controller_data_s old_ctrl;

	// Controller, UART, or telnet can be used now to control things.
	while(1){
		// For controllers
		usb_do_poll();

		// For telnet
		network_poll();
		unsigned char latest_telnet_char = telnet_recv_buf[0];

		// Controller 
		if (get_controller_data(&ctrl, 0)) {
			if (ctrl.x){
				exit(0);
				break;
			} if (ctrl.y){
				xenon_smc_power_shutdown();
				break;
			} if (ctrl.b){
				xenon_smc_power_reboot();
				break;
			}
			old_ctrl=ctrl;
		}

		// If UART sends a keystroke then see what char it was and process it.
		if(kbhit()){
			switch(getch()){
				case 'x':
					// Try reloading XeLL from NAND. (1f, 2f, gggggg)
					// Platform specific functioanlity defined in libxenon/drivers/newlib/xenon_syscalls.c
					exit(0);
					break;
				case 'y':
					xenon_smc_power_shutdown();
					break;
				case 'b':
					xenon_smc_power_reboot();
					break;
				default:
					printf("Remote char received via UART.\n");
					break;
			}
		}

		// Check the telnet receive buffer for any chars and process them as well.
		// Requires update to LibXenon that externalizes the receive buffer.
		if(latest_telnet_char == 'x'){
			exit(0);
			break;
		} else if(latest_telnet_char == 'y'){
			xenon_smc_power_shutdown();
			break;
		} else if(latest_telnet_char == 'b'){
			xenon_smc_power_reboot();
			break;
		}
	}

	// Return to XeLL on NAND or else hard reboot. Should never get here.
	exit(0);
}
