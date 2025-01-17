#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAP_SIZE	  100
#define MAX_CODE_SIZE 1024
#define MAX_DATA_SIZE 512

typedef struct sym_pair {
	char *label;
	int address;
	struct sym_pair *next;
} sym_pair;

typedef struct sym_table {
	sym_pair *sym_table[MAP_SIZE];
} sym_table;

sym_table global_table = {{NULL}};

typedef struct instruction {
	const char *mnemonic;
	int operand_count;
	const char **operand_types;
	unsigned char opcode;

} instruction;

instruction instruction_set[] = {
	{"MOV", 2, (const char *[]){"reg", "reg"}, 0x89},
	{"ADD", 2, (const char *[]){"reg", "imm"}, 0x83},
	{"SUB", 2, (const char *[]){"reg", "reg"}, 0x29},
};

int instruction_set_size = (sizeof(instruction_set) / sizeof(instruction_set[0]));

unsigned int hash_func(const char *label) {
	unsigned int hash = 0;
	while (*label != '\0') {
		hash = (hash * 31) + *label;
		label++;
	}
	return hash % MAP_SIZE;
}

void print_sym_table(sym_table *table) {
	printf("\n-------Printing symbol table:-------\n");
	for (int i = 0; i < MAP_SIZE; i++) {
		sym_pair *current = table->sym_table[i];
		while (current != NULL) {
			printf("Index: %d, Label: %s, Address: %d\n", i, current->label, current->address);
			current = current->next;
		}
	}
}

void insert_label(const char *label, unsigned int addr, sym_table *table) {
	unsigned int index = hash_func(label);

	sym_pair *pair = malloc(sizeof(sym_pair)); // allocate pair to add
	if (pair == NULL) {
		perror("Cant allocate mem for new pair");
		exit(1);
	}

	pair->label = malloc(strlen(label) + 1); // allocate mem for the label char*
	if (pair->label == NULL) {
		perror("Cant allocate pair to insert's label");
		exit(1);
	}

	pair->address = addr;						   // update label and insert in hash table (avoid collisions)
	memcpy(pair->label, label, strlen(label) + 1); // syn to pair.label = label
	pair->next = NULL;

	// handle collision
	sym_pair *current = table->sym_table[index];
	if (current == NULL) {
		table->sym_table[index] = pair; // no collision
	} else {
		pair->next				= current; // allocate mem for the label char*
		table->sym_table[index] = pair;
	}

	printf("Inserted label: %s at address: %d\n", label, addr);
}

unsigned int duplicate_label(const char* token){
	for(int i=0; i<MAP_SIZE; i++){
		sym_pair* current = global_table.sym_table[i];
		while (current!=NULL)
		{
			if (strcmp(current->label, token)==0){
				return 1;
			}
			current = current->next;
		}
	}
	return 0;
}


void process_tokens(const char *line, unsigned int *code_ptr, unsigned int *data_ptr) {

	const char *delimiters = "\t,\n()[]; "; // split each line into tokens based on delims

	char *line_copy = malloc(strlen(line) + 1); // use line_copy as strtok changes input str
	if (line_copy == NULL) {
		perror("Couldn't allocate line");
		exit(1);
	}
	memcpy(line_copy, line, strlen(line) + 1);

	char *cmnt_start = strchr(line_copy, ';');
	if (cmnt_start != NULL) {
		*(cmnt_start) = '\0'; // truncate string
	}

	char *token = strtok(line_copy, delimiters);

	while (token != NULL) {
		// check if label
		if (token[strlen(token) - 1] == ':') {
			token[strlen(token) - 1] = '\0';
			// check for duplicating labels and insert
			if (duplicate_label(token)==0) {
				insert_label(token, *code_ptr, &global_table);
				printf("tokens = [%s]\n", token);
			}
			else {
				printf("duplicate token : [%s]\n", token);
			}

			token = strtok(NULL, delimiters);
			continue;
		}

		int is_instruction = 0;

		for (int i = 0; i < instruction_set_size; i++) {
			if (strcmp(instruction_set[i].mnemonic, token) == 0) {
				printf("Instruction found: %s\n", token);
				is_instruction = 1;
				*code_ptr += instruction_set[i].operand_count + 1;
				break;
			}
		}

		if (!is_instruction) {
			printf("Not an instruction\n");
		} 

		token = strtok(NULL, delimiters);
	}	

	print_sym_table(&global_table);
	printf("Current code pointer: %d\n", *code_ptr);

	free(line_copy);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Using: %s <f_name>\n", argv[0]);
		return 1;
	}
	const char *f_name = argv[1];

	FILE *file = fopen(f_name, "r");
	if (file == NULL) {
		perror("Can't open file for reading");
		return 1;
	}

	unsigned char code_seg[MAX_CODE_SIZE];
	unsigned int code_ptr = 0;
	unsigned char data_seg[MAX_DATA_SIZE];
	unsigned int data_ptr = 0;

	char line[64];

	// fgets takes string, size, stream
	while (fgets(line, sizeof(line), file)) {
		process_tokens(line, &code_ptr, &data_ptr);
	}

	fclose(file);
	return 0;
}