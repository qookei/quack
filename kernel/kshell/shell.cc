#include "shell.h"
#include <cpuid.h>
#include "../paging/pmm.h"
#include "../kheap/heap.h"

#define COMMAND(command, x) (strncmp(command, x, strlen(x)) == 0)

#define KSHELL_PROMPT "quack> "

int quackfetch(char*);
int free(char*);

const char* command_names[] = {
	"quackfetch",
	"free"
};

int (*command_ptrs[])(char*) = {
	quackfetch,
	free
};

uint32_t ncommands = 2;

// commands

int kshell_exec(char* command);

extern void mem_dump(void *data, size_t nbytes, size_t bytes_per_line);
extern void* memset(void*, int, size_t);

int strcmp(const char *str1, const char *str2) {
	size_t i = 0;
	while (str1[i] && str1[i] == str2[i]) i++;

	return str1[i] - str2[i];
}

int strncmp(const char *str1, const char *str2, size_t len) {
	size_t i = 0;
	while (i < len - 1 && str1[i] == str2[i]) i++;
	return str1[i] - str2[i];
}

void kshell_main() {
	
	printf("Welcome to the kernel shell!\n");

	char buffer[256] = {0};
	uint32_t buffer_idx = 0;

	printf("%s", KSHELL_PROMPT);

	while(1) {
		char c = getch();	// this is blocking so i don't have to worry about anything
		if (c == '\b') {
			if(buffer_idx > 0) {
				printf("\b");
				buffer[--buffer_idx] = 0;
			}
		} else if (c == '\n') {
			// exec command
			if (buffer_idx > 0) {
				printf("\n");
				kshell_exec(buffer);

				memset(buffer, 0, 256);
				buffer_idx = 0;

			}
			printf("\n%s", KSHELL_PROMPT);
		} else {
			if (buffer_idx < 256) {
				buffer[buffer_idx++] = c;
				printf("%c",c);
			}
		}

		// that should be fine for now
	}
}


uint8_t *mem;

int kshell_exec(char* command) {

	for (uint32_t i = 0; i < ncommands; i++) {
		if (COMMAND(command, command_names[i])) {
			return command_ptrs[i](command + strlen(command_names[i]));
		}
	}

	printf("command not found: %s\n", command);

	return 1;

}

int free(char* args) {

	return 0;

}

int quackfetch(char* args) {

	uint32_t brand[12];
	__cpuid(0x80000002 , brand[0], brand[1], brand[2], brand[3]);
	__cpuid(0x80000003 , brand[4], brand[5], brand[6], brand[7]);
	__cpuid(0x80000004 , brand[8], brand[9], brand[10], brand[11]);

	printf("%cc",0x1B);
	printf("\n");
	printf("                 `:+sdmNNmdyo:                                                                        \n");
	printf("               .smmNNNMMMMMMMNmo`                                                                     \n");
	printf("              /hhdddNMMMMMMMMMMMNy-                                                                   \n");
	printf("             :oohhdNMMNNMMMNMNMMMMNo                                                                  \n");
	printf("            .osddmmNNNNNNNNNNNNMMMMNh`                                                                \n");
	printf("            -+/oNNNNNNNNNNNNNNNMMMMMNy                                                                \n");
	printf("            `-/smNNNNMMMMMMMMMMMMMMMMN`                                                               \n");
	printf("          `-sy/shdmMMMMMMMMMMMMMMMMMMN/                                                               \n");
	printf("         .://+oyhhmNMMMMMMMMMMMMMMMMMN-                                                               \n");
	printf("      `.:/+oyhmNmsoshmMMMMMMMMMMMMMMMN.                                                               \n");
	printf("   ...:/oydmNh+-     .mMMMMMMMMMMMMMMs                                                                \n");
	printf("  /y+shddyo:`    `:o+sNMMMMMMMMMMMMMM`                                                                \n");
	printf("   .o+/-      `:odmNNNdmNNmNNNMMMMMMN                                                                 \n");
	printf("           `:+shdmmNNMMMMMMmmdydhhmMy                                                                 \n");
	printf("         ./o+syhmmNNMMMMMMMMMMMMMNdmyss+++++//:-`                                                     \n");
	printf("       ./+osohdmmNNMMMMMMMMMMMNNmmddmddddddhdhddddhyo/:.                                              \n");
	printf("     `/ssssyhdmmNmNMMMMMMMMNmmddddmmNNNmys+oosyyysyyyyydmmdy/.                                        \n");
	printf("    `:ossoshdmmmmmmNMMMMNNmddddmmmNNmh+//oo++ossossssoooo+oymNNmhso:.                                 \n");
	printf("    /ooo/+syddddhhddmmmdddddmmmNNNmh/--:://++o++++oooossso/::/sdNMMMMNh+-                             \n");
	printf("   :yso+/ossyyhhhdddddyyosyhdmNNmdh+-:/+++oossooosoooooooooo+/::/ohmMMMMMms-                          \n");
	printf("   sddyosoyysyhhhdmmdhssyhhdddho+/::--::/+oyhhhhhhyhyyyyssssssssoo+/+ymMMMMMh+-                       \n");
	printf("   hddyyhhhdmmmNNmNNNdsyydddds:-..-....-/+++oydmmmmmmmddhhhhhhhhhyyyso++shNMMMMNy/`                   \n");
	printf("   hddyydmNNNNNNMMMMMNmddddhs+:----:::-:///::::/oydmNNNNNNNmmmddddddhhyyo+/oymMMMMMh/                 \n");
	printf("   +mmhdmNNNNMMMMMMMMNNmmddhyo:----://:://::://///+oshmMMMMMNNNNmmmmddhhhhyso::odNMMMNs.              \n");
	printf("   `hNdmmNNNMMMMMMMMMMNNNNNdm+:::-:/::::::::::://++++ooshdNMMMMMMNNNNNmmmddyyso+//ohNMMN+             \n");
	printf("    .mNmmNNMMMMMMMMMMMMMMMMNdooo+///+/::/::////++o++o+o++osydNMMMMMMMMMNNNmdhyysoo++/+ymMyss+//.      \n");
	printf("     .dmNmNNMMMMMMMMMMMMMMNdsoooooooo+///////+++++++++++ooosssydNMMMMMMMMNNNmmdhhyysoo+/+ssohm-mh     \n");
	printf("       +mNNMMMMMMMMMMMMMMmhssooossssso+++++++++++oooo++oooossssssyhdNMMMMMMMNNNmmddhyysso+/:+oshd     \n");
	printf("        `omNNMMMMMMMMMMMNysosssyyssossooooooossossosoooooosyyyyyyyyyyydNMMMMMMMNNNmmddhyysoshddds+:   \n");
	printf("           :smMNMMMMMNNNmyysssoossooosososssssyyysssssssyyyhhhhhhhhhhhhhdNMMMMMMMNNmmddhssoo++++/:-`` \n");
	printf("              :smNNNNNNNmdhhhysssssossssssyhhhhhyyysssyyhhhhhhhhhdhhhhhhhdddddddddyshMMMNmmmy+.`      \n");
	printf("                 -+hNNmmdddhyhyysyssyyyyyyyhhyhyhhhhhhhhhddddddddddddddddddhhyyyyyo++dMMNh+.          \n");
	printf("                    `:ohhhyyyyyysyyyyyhhhhhhhhhhhhhhhddddddddddddddddddddddhyyyyhyssoso:              \n");
	printf("                        -/syyyyyyhhhhhyhhhhhhhhdhhhhhddddddddddddddmmmmmdhhhyyyhhhys/`                \n");
	printf("                           ./oyyhhhhhhhhdddddddddddddddddmmmmmmmmmmmmmdhhhhhhhhhhhs-                  \n");
	printf("                               .-/oyhhhhhdhdddddddddmmmmmmmmmmmmmmmmdddddddhhyo+/-                    \n");
	printf("                                     `.://hdmysssysdmmmNNmmNNNmmmmhys+/:.`                            \n");
	printf("                                         :hmh`   shdm:  ``..---.`                                     \n");
	printf("                                        .ydm:    :odh                                                 \n");
	printf("                                       `shdm`   `+hm:                                                 \n");
	printf("                                       /hddh`   :ydy                                                  \n");
	printf("                         /::::::::://++sshmoo. -ydd-                                                  \n");
	printf("                      -+oossssyyyyyyysssys. `-:+hmd`                                                  \n");
	printf("                          ``..--/syooo++-...../sdmdh`                                                 \n");
	printf("                           `-+/++ooooosooooosyyys: `                                                  \n");
	printf("                                -yo++/////:--`                                                        \n");

	printf("%c[51;3H", 0x1B);
	printf("OS: quack");
	printf("%c[51;4H", 0x1B);
	printf("Kernel: quack 0.0.1");
	printf("%c[51;5H", 0x1B);
	printf("Uptime: ?");
	printf("%c[51;6H", 0x1B);
	printf("Packages: ?");
	printf("%c[51;7H", 0x1B);
	printf("Shell: ?");
	printf("%c[51;8H", 0x1B);
	printf("CPU: %s", (const char*)brand);
	printf("%c[51;9H", 0x1B);
	printf("GPU: ? using VBE");
	printf("%c[51;10H", 0x1B);
	printf("RAM: %uMiB/%uMiB", free_pages() * 4096 / 1024 / 1024, max_pages() * 4096 / 1024 / 1024);
	getch();
	printf("%cc", 0x1B);

	return 0;

}