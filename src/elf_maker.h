#ifndef elf_maker_h
#define elf_maker_h

#include <elf.h>
#include "../misc/SLL.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ELF_HEADER_SIZE 0x40
#define ELF_PROGREAM_HEADER_SIZE 0x38
#define ELF_SECTION_HEADER_SIZE 0x40

/** Types Definitions **/
/* elf and section headers - wrapers*/
typedef Elf64_Ehdr elf_header_t;
typedef Elf64_Phdr elf_program_header_t;
typedef Elf64_Shdr elf_section_header_t;
typedef Elf64_Sym  elf_symbol_table_entry_t;

typedef struct elf_program_code_t
{
    unsigned char *code;
    int size;
} elf_program_code_t;

/* elf file struct */
typedef struct elf_file_t
{
  elf_header_t header;
  SLL *list_segments;
} elf_file_t;

/* elf section struct */
typedef struct elf_segment_t
{
  char *name_string;
  elf_program_header_t  program_header;
  elf_program_code_t    program_code;
  elf_section_header_t  section_header;
  int has_program;
} elf_segment_t;

/* section entry struct */
typedef struct elf_section_entry_t
{
  size_t size;
  void *data;
} elf_section_entry_t;

/** Public Functions **/
extern elf_file_t *elf_maker_init();
extern void elf_maker_free(elf_file_t *elf_file);
extern elf_program_header_t* elf_maker_add_empty_prom_header(elf_file_t *elf_file);
extern elf_segment_t *elf_maker_add_segment(elf_file_t *elf_file, char name[]);
extern int elf_maker_add_prog_code(elf_file_t *elf_file);
extern void elf_maker_write(elf_file_t *elf_file, FILE *output);

/** Private Functions **/
int _get_program_header_offset (elf_file_t *elf_file);
int _get_program_header_num(elf_file_t *elf_file);
int _get_string_table_offset (elf_file_t *elf_file);
int _get_code_offset (elf_file_t *elf_file);
int _get_code_length (elf_file_t *elf_file);
int _get_section_offset (elf_file_t *elf_file);
int _get_string_table_section_index(elf_file_t *elf_file);
int _get_string_table_lenth(elf_file_t *elf_file);
void _fill_section (elf_file_t *elf_file);
void elf_maker_write(elf_file_t *elf_file, FILE *output);

#endif
