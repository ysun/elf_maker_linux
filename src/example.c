#include "elf_maker.h"
//#include "elf_maker_special_sections.h"
#include <string.h>

int main()
{
  /* elf maker init */
  elf_file_t *elf_file = elf_maker_init();

  /* get sections info */
//  SLLInfo *sections_info = elf_file->sections->data;

 //  /* add null section */
 //  elf_section_t *null_section = add_null_section(elf_file);

 //  /* add strings section (can't be the first section)*/
 //  elf_section_t *strings_section = add_strings_section(elf_file);

 //  /* add symbol table section */
 //  elf_section_t *symbol_table_section = add_symbol_table_section(elf_file);

 //  /* add symbol table strings section */
 //  elf_section_t *symbol_table_strings_section = add_symbol_table_strings_section(elf_file, symbol_table_section);

 //  /* add text section*/
 //  elf_section_t *text_section = add_text_section(elf_file, symbol_table_section, symbol_table_strings_section);
 //  int32_t i1 = 0x000001b8;
 //  int32_t i2 = 0x00ffbb00;
 //  int32_t i3 = 0x80cd0000;
 //  elf_maker_add_section_entry(text_section, &i1, 4);
 //  elf_maker_add_section_entry(text_section, &i2, 4);
 //  elf_maker_add_section_entry(text_section, &i3, 4);

 //  /* finalize the string section */
 //  finalize_strings_section(elf_file, strings_section);

   /* prepare output file */
   FILE *ofile = fopen("out_bin", "w");
   if (!ofile)
   {
     fprintf(stderr, "ERORR: couldn't open output file\n");
     exit(1);
   }

   FILE *elfdemo = fopen("elfdemo", "r");
   if (!elfdemo)
   {
     fprintf(stderr, "ERORR: couldn't open ELFDEMO file\n");
     exit(1);
   }
   fseek(elfdemo,0xda, SEEK_SET);
   unsigned char *buffer = (unsigned char*)malloc(1024);
   int size = fread(buffer, 1, 56, elfdemo); 

   elf_segment_t *p_segment = elf_maker_add_segment(elf_file, ".text");
   elf_program_header_t *prog_header = &p_segment->program_header;
   elf_program_code_t *prog_code = &p_segment->program_code;

   prog_header->p_type = 1;
   prog_header->p_flags = 5; //executable | readable
   prog_header->p_offset = 0; //不明觉厉
   prog_header->p_vaddr = 0x400000; //临时
   prog_header->p_memsz = 0xB0;   //hello world
   prog_header->p_filesz = 0xB0;   //hello world
   prog_header->p_align = 0x200000; //2M align

   prog_code->code = buffer;
   prog_code->size = size;

   /* write the elf file*/
   elf_maker_write(elf_file, ofile);

 /*cleanup */
//  fclose(ofile);
//  elf_maker_free(elf_file);
}