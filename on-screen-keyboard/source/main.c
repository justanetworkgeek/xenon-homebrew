#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <console/console.h>
#include <console/telnet_console.h> // recv_buf
#include <xenos/xenos.h>
#include <xenos/xe.h>
#include <xenos/edram.h>
#include <input/input.h>
#include <network/network.h>
#include <xenon_uart/xenon_uart.h>
#include <usb/usbmain.h>
#include <xenon_smc/xenon_smc.h>
#include <xenon_soc/xenon_power.h>

// xenon_run_thread_task(int thread, void *stack, void *task);
void *stack[5 * 0x1000]; // xenon_power.c

// Read input forever in thread number 1
void *input_thread(){
	// Initialize 360 controller - taken from XeLL kbootconf.c
	struct controller_data_s ctrl;
	struct controller_data_s old_ctrl;

	while(1){
		// For controllers
		usb_do_poll();
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

		// For UART
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

		// For telnet
		network_poll();
		unsigned char latest_telnet_char = telnet_recv_buf[0];
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
}

// Refresh the GPU forever in thread number 2
void *video_thread(){
	struct XenosDevice _xe, *xe;
	xe = &_xe;

	// Initialize Xenos using the created virtual device.
	Xe_Init(xe);

	// Create and set the render target (Xenos Framebuffer).
	struct XenosSurface *fb = Xe_GetFramebufferSurface(xe);
	Xe_SetRenderTarget(xe, fb);

	// Initialize EDRAM before the first render frame
	edram_init(xe);

	// Close the console, so it doesn't slow down rendering
	console_close();

	// Create a loop to do rendering.
	while(1){
		Xe_SetClearColor(xe, 0xFFFFFF); // test

		// Resolve and clear
		Xe_Resolve(xe);

		// Synchronize frame
		Xe_Sync(xe);
	}
}

int main(){
	// This should all run in thread zero in a perfect world.
	// Bring up all the hardware so this program can use it.
	xenos_init(VIDEO_MODE_AUTO);
	console_init();
	xenon_thread_startup();
	xenon_make_it_faster(1); // Full speed, in case this launched from NAND.
	usb_init();
	network_init();
	network_print_config();
	telnet_console_init(); // redirect printf output here

	// Create threads for input handling and video output.
	// Leaves 3 threads available for programs to do whatever other logic.
	xenon_run_thread_task(1, stack[1], input_thread());
	xenon_run_thread_task(2, stack[2], video_thread());

	// Return to XeLL on NAND or else hard reboot. Should never get here.
	exit(0);
}
