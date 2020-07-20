/** Includes **/
#include "elf_maker.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ELF_HEADER_SIZE 0x40
#define ELF_PROGREAM_HEADER_SIZE 0x38
#define ELF_SECTION_HEADER_SIZE 0x40

/** Public Functions **/
elf_file_t *elf_maker_init()
{
  /* init the header */
  elf_header_t header;
  memset(&header, 0, sizeof(elf_header_t));
  header.e_ident[EI_MAG0] = ELFMAG0;       /* magic start of elf file '0x7F' */
  header.e_ident[EI_MAG1] = ELFMAG1;       /* magic start of elf file E */
  header.e_ident[EI_MAG2] = ELFMAG2;       /* magic start of elf file L */
  header.e_ident[EI_MAG3] = ELFMAG3;       /* magic start of elf file F */
  header.e_ident[EI_CLASS] = ELFCLASS64;   /* architecture version (32bit) */
  header.e_ident[EI_DATA] = ELFDATA2LSB;   /* endianness (little endian => LSB) */
  header.e_ident[EI_VERSION] = EV_CURRENT; /* elf header version (current = 1) */
  header.e_ident[EI_OSABI] = 0;            /* target os => linux ignores this, so use 0 */
  header.e_ident[EI_ABIVERSION] = 0;       /* target os abreviation => linux ignores this, so use 0 */
  header.e_type = ET_EXEC;                  /* Object file type (executable binary) */
  header.e_machine = EM_X86_64;               /* instruction set (x86 for now) */
  header.e_version = EV_CURRENT;           /* elf header version (current = 1) */

  // This is only for execuable file!!!
  header.e_entry = 0x400000;

  header.e_phoff= ELF_HEADER_SIZE;        /* program headers offset, following by ELF header */

  header.e_ehsize = ELF_HEADER_SIZE;       /* size of the elf header */
  header.e_phentsize = ELF_PROGREAM_HEADER_SIZE;       /* size of the elf header */
  header.e_shentsize = ELF_SECTION_HEADER_SIZE; /* size of entry in the section header table */

  /* add the elf header to the elf file */
  elf_file_t *file = (elf_file_t *)malloc(sizeof(elf_file_t));
  memset(file, 0, sizeof(elf_file_t));
  file->header = header;

  file->list_segments = SLL_init();

  elf_maker_add_segment(file, "");
  elf_maker_add_segment(file, ".shstrab");

  /* return new file*/
  return file;
}
int _get_program_header_offset (elf_file_t *elf_file) {
  int pos = 0;
  pos += ELF_HEADER_SIZE;

  return pos;
}


int _get_string_table_offset (elf_file_t *elf_file) {
  int pos = 0, cnt_prog_header = 0;
  pos += _get_program_header_offset(elf_file);

  SLL *seg_iter = elf_file->list_segments->next;
  while (seg_iter)
  {
    elf_segment_t* p_segment= (elf_segment_t*)seg_iter->data;

    if(p_segment->has_program)
        cnt_prog_header ++;

    seg_iter = seg_iter->next;
  }

  pos += cnt_prog_header * ELF_PROGREAM_HEADER_SIZE;

  return pos;
}

int _get_code_offset (elf_file_t *elf_file) {
  int pos = 0;
  pos += _get_string_table_offset(elf_file);

  SLL *seg_iter = elf_file->list_segments->next;
  while (seg_iter)
  {
    elf_segment_t* p_segment= (elf_segment_t*)seg_iter->data;
    pos += strlen(p_segment->name_string);

    seg_iter = seg_iter->next;
  }

  return pos;
}

int _get_section_offset (elf_file_t *elf_file) {
  int pos = 0;
  pos += _get_code_offset(elf_file);

  SLLNode *header_iter = elf_file->list_segments->next;
  while (header_iter)
  {
    elf_segment_t* p_segment= (elf_segment_t*)header_iter->data;
    pos += p_segment->program_code.size;

    header_iter= header_iter->next;
  }
  return pos;
}

int _get_string_table_section_index(elf_file_t *elf_file) {
  int index = 0;
  SLL *seg_iter = elf_file->list_segments->next;
  SLLInfo *seg_info = (SLLInfo*)elf_file->list_segments->data;

  while (seg_iter)
  {
    elf_segment_t* p_segment= (elf_segment_t*)seg_iter->data;

    if(strcmp(p_segment->name_string, ".shstrab") == 0)
        break;
    else
        index ++;

    seg_iter = seg_iter->next;
  }

  return (index > seg_info->size)?  -1: index;
}
int _get_string_table_lenth(elf_file_t *elf_file) {
  int lenth = 0;
  SLL *seg_iter = elf_file->list_segments->next;

  while (seg_iter)
  {
    elf_segment_t* p_segment= (elf_segment_t*)seg_iter->data;

    lenth += strlen(p_segment->name_string) + 1;

    seg_iter = seg_iter->next;
  }

  return lenth;
}

void elf_maker_write(elf_file_t *elf_file, FILE *output)
{
  if (!elf_file || !output)
    return;

  int cur_pos = 0;
  int string_offset = 0;
  int program_code_offset = 0;

  SLLInfo *si_segment_header = (SLLInfo*)elf_file->list_segments->data;
  elf_file->header.e_phnum = si_segment_header->size;
  elf_file->header.e_shnum = si_segment_header->size;
  elf_file->header.e_phoff = _get_program_header_offset(elf_file);
  elf_file->header.e_shoff = _get_section_offset(elf_file);

  elf_file->header.e_entry += _get_code_offset(elf_file);
  elf_file->header.e_shstrndx = _get_string_table_section_index(elf_file);


  /* fill in section */
  SLLNode *header_iter = elf_file->list_segments->next;
  while (header_iter)
  {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
    p_segment->section_header.sh_name = string_offset;

    if(strcmp(p_segment->name_string, "") != 0) {
      string_offset += strlen(p_segment->name_string)+1;

      if(p_segment->has_program)
          program_code_offset += p_segment->program_code.size;

      if(strcmp(p_segment->name_string, ".shstrab") == 0){
          p_segment->section_header.sh_type = 3;
          p_segment->section_header.sh_flags = 0;
          p_segment->section_header.sh_offset = _get_string_table_offset(elf_file);
          p_segment->section_header.sh_size = _get_string_table_lenth(elf_file);
          printf("ysun: _get_string_table_offset: %ld, lenth: %ld\n", 
                 p_segment->section_header.sh_size, p_segment->section_header.sh_size);
      }
      if(strcmp(p_segment->name_string, ".text") == 0) {
          p_segment->section_header.sh_type = 1;
          p_segment->section_header.sh_flags = 6;

          /* 可能这个地方不需要加offset，直接把地址写给program_header.p_vaddr就好*/
          p_segment->section_header.sh_addr = p_segment->program_header.p_vaddr + program_code_offset;
          p_segment->section_header.sh_offset = program_code_offset;
          p_segment->section_header.sh_size = p_segment->program_code.size;
      }
      p_segment->section_header.sh_addralign = 1;

      printf("ysun: go througn string_name, string_offset:%d\n", string_offset);
    }
    header_iter= header_iter->next;
  }

  /* write main ELF header */
  cur_pos += fwrite(&elf_file->header, 1, ELF_HEADER_SIZE, output);
  printf("ysun: after write ELF_header, cur_pos:%ld/%d\n", ftell(output), cur_pos);

  
  /* write ELF program headers to file */
  header_iter = elf_file->list_segments->next;
  while (header_iter)
  {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
    if(p_segment->has_program) {
      printf("ysun: program_header:\n");
  //    for(int i= 0; i < ELF_PROGREAM_HEADER_SIZE; i++)
  //        printf("%2x ", *((int *)&(p_segment->program_header)+i));
  
      cur_pos += fwrite((void*)&(p_segment->program_header), 1, ELF_PROGREAM_HEADER_SIZE, output);
      printf("ysun: after write program_header, cur_pos:%ld/%d\n", ftell(output), cur_pos);
    }
    header_iter= header_iter->next;
  }

  /* write ELF string table to file */
  header_iter = elf_file->list_segments->next;
  while (header_iter)
  {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
    cur_pos += fwrite(p_segment->name_string, 1, strlen(p_segment->name_string)+1, output);
    printf("ysun: after write name string, cur_pos:%ld/%d, name_string:%s\n", ftell(output), cur_pos, p_segment->name_string);
    header_iter= header_iter->next;
  }

  /* write program text (code) to file */
  header_iter = elf_file->list_segments->next;
  while (header_iter)
  {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
    cur_pos += fwrite(p_segment->program_code.code, 1, p_segment->program_code.size, output);
    printf("ysun: after write code, cur_pos:%ld/%d\n", ftell(output), cur_pos);
    header_iter= header_iter->next;
  }

  /* write section header to file */
  header_iter = elf_file->list_segments->next;
//  char buf_null_secton[64] = {0};
//  cur_pos += fwrite(buf_null_secton, 1, 64, output);
//  printf("ysun: after write NULL section, cur_pos:%ld/%d\n", ftell(output), cur_pos);

  elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
  memset(&p_segment->section_header, 0, ELF_SECTION_HEADER_SIZE);

  while (header_iter)
  {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
    cur_pos += fwrite(&p_segment->section_header, 1, sizeof(elf_section_header_t), output);
    printf("ysun: after write section, cur_pos:%ld/%d\n", ftell(output), cur_pos);
    header_iter= header_iter->next;
  }

  fclose(output);

//  /* write sections headers */
//  SLLNode *sections_iter = elf_file->sections->next;
//  while (sections_iter)
//  {
//    elf_section_t *section = sections_iter->data;
//    fwrite(&section->header, 1, sizeof(elf_section_header_t), output);
//    sections_iter = sections_iter->next;
//  }
//
//  /* write sections entries */
//  sections_iter = elf_file->sections->next;
//  while (sections_iter)
//  {
//    elf_section_t *section = sections_iter->data;
//    SLLNode *section_entries_iter = section->entries->next;
//    while (section_entries_iter)
//    {
//      elf_section_entry_t *entry = section_entries_iter->data;
//      fwrite(entry->data, 1, entry->size, output);
//      section_entries_iter = section_entries_iter->next;
//    }
//    sections_iter = sections_iter->next;
//  }
}

elf_segment_t*
elf_maker_add_segment(elf_file_t *elf_file, char name[])
{
  /* error checking */
  if (!elf_file)
    return NULL;

  /* make section */
  elf_segment_t *segment = (elf_segment_t *)malloc(sizeof(elf_segment_t));
  memset(segment, 0, sizeof(elf_segment_t));

  printf("sizeof elf_segment_t: %ld\n", sizeof(elf_segment_t));

  /* section name */
  if (name)
  {
    int len = strlen(name);
    segment->name_string = (char*) malloc(len + 1);
    memcpy(segment->name_string, name, len);
//    printf("ysun: add segment string: %s,%d\n", segment->name_string, len);

    segment->name_string[len] = '\0';
    
    if(strcmp(name, ".shstrab") != 0 && strcmp(name,"\0") != 0)
        segment->has_program = 1;
  }

  /* add section to the elf file */
  SLL_insert(elf_file->list_segments, segment);

  /*return the created section */
  return segment;
}

// void elf_maker_free(elf_file_t *elf_file)
// {
//   if (!elf_file)
//     return;
// 
//   /* free sections */
//   SLLNode *sections_iter = elf_file->sections->next;
//   while (sections_iter)
//   {
//     elf_section_t *section = sections_iter->data;
//     _elf_maker_section_free(section);
//     sections_iter = sections_iter->next;
//   }
//   SLL_free(elf_file->sections);
//   elf_file->sections = NULL;
// 
//   /* free the file */
//   free(elf_file);
//   elf_file = NULL;
// }
// /*********************************************** Private Functions ***************************************************/
// void _elf_maker_prepare_for_writing(elf_file_t *elf_file)
// {
//   if (!elf_file)
//     return;
// 
//   /* get sections info */
//   SLLInfo *sections_info = elf_file->sections->data;
// 
//   /* update number of sections */
//   elf_file->header.e_shnum = sections_info->size;
// 
//   /* update sections offset */
//   size_t section_offset = elf_file->header.e_shoff + elf_file->header.e_shnum * sizeof(elf_section_header_t);
//   SLLNode *sections_iter = elf_file->sections->next;
//   while (sections_iter)
//   {
//     elf_section_t *section = sections_iter->data;
//     if (section->header.sh_size && section->header.sh_type != SHT_NOBITS)
//     {
//       section->header.sh_offset = section_offset;
//       section_offset += section->header.sh_size;
//     }
//     /* next iteration */
//     sections_iter = sections_iter->next;
//   }
// }
// 
// void _elf_maker_section_free(elf_section_t *section)
// {
//   if (!section)
//     return;
// 
//   /* free the name */
//   if (section->name)
//   {
//     free(section->name);
//     section->name = NULL;
//   }
// 
//   /* free the entries */
//   SLLNode *section_entries_iter = section->entries->next;
//   while (section_entries_iter)
//   {
//     elf_section_entry_t *entry = section_entries_iter->data;
//     _elf_maker_section_entry_free(entry);
//     section_entries_iter = section_entries_iter->next;
//   }
//   SLL_free(section->entries);
//   section->entries = NULL;
// 
//   /* free the section */
//   free(section);
//   section = NULL;
// }
// 
// void _elf_maker_section_entry_free(elf_section_entry_t *entry)
// {
//   if (!entry)
//     return;
// 
//   /* free data */
//   if (entry->data)
//   {
//     free(entry->data);
//     entry->data = NULL;
//   }
// 
//   /* free entry */
//   free(entry);
//   entry = NULL;
// }
