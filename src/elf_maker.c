#include "elf_maker.h"

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
  header.e_type = ET_EXEC;                 /* Object file type (executable binary) */
  header.e_machine = EM_X86_64;            /* instruction set (x86 for now) */
  header.e_version = EV_CURRENT;           /* elf header version (current = 1) */

  // This is only for execuable file!!!
  header.e_entry = 0x400000;

  header.e_phoff= ELF_HEADER_SIZE;         /* program headers offset, following by ELF header */
  header.e_ehsize = ELF_HEADER_SIZE;       /* size of the elf header */
  header.e_phentsize = ELF_PROGREAM_HEADER_SIZE;    /* size of the elf header */
  header.e_shentsize = ELF_SECTION_HEADER_SIZE;     /* size of entry in the section header table */

  /* add the elf header to the elf file */
  elf_file_t *file = (elf_file_t *)malloc(sizeof(elf_file_t));
  memset(file, 0, sizeof(elf_file_t));
  file->header = header;

  file->list_segments = SLL_init();

  /* 
   * Add a null section at the begin of section header table 
   * And followed by the string table section .shstrtab 
   * */
  elf_maker_add_segment(file, "");
  elf_maker_add_segment(file, ".shstrtab");

  /* return new file*/
  return file;
}

/*
 * Program header table is right after the 'ELF header'
 * which has fixed lenght 64 bytes.
 */
int _get_program_header_offset (elf_file_t *elf_file) {
  int pos = 0;
  pos += ELF_HEADER_SIZE;

  return pos;
}

/*
 * Only .text section has program header
 */
int _get_program_header_num(elf_file_t *elf_file) {
  int cnt = 0;

  SLLNode *header_iter = elf_file->list_segments->next;
  while (header_iter)
  {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
    if(p_segment->has_program)
        cnt ++;

    header_iter= header_iter->next;
  }
  return cnt;
}

/* 
 * String table is right after the program header table
 * Only .text section has program header!
 */
int _get_string_table_offset (elf_file_t *elf_file) {
  int pos = 0, cnt_prog_header = 0;
  pos += _get_program_header_offset(elf_file);

  SLL *seg_iter = elf_file->list_segments->next;
  while (seg_iter) {
    elf_segment_t* p_segment= (elf_segment_t*)seg_iter->data;

    if(p_segment->has_program)
        cnt_prog_header ++;

    seg_iter = seg_iter->next;
  }

  pos += cnt_prog_header * ELF_PROGREAM_HEADER_SIZE;

  return pos;
}

/*
 * Each program header has a program code, located right after
 * name string table.
 */
int _get_code_offset (elf_file_t *elf_file) {
  int pos = 0;
  pos += _get_string_table_offset(elf_file);

  SLL *seg_iter = elf_file->list_segments->next;
  while (seg_iter) {
    elf_segment_t* p_segment= (elf_segment_t*)seg_iter->data;

    if(strcmp(p_segment->name_string, "") != 0)
        pos += strlen(p_segment->name_string) + 1;

    seg_iter = seg_iter->next;
  }

  return pos;
}
/*
 * All code text size.
 */
int _get_code_length (elf_file_t *elf_file) {
  int len = 0;

  SLL *seg_iter = elf_file->list_segments->next;
  while (seg_iter) {
    elf_segment_t* p_segment= (elf_segment_t*)seg_iter->data;

    if(p_segment->has_program)
        len += p_segment->program_code.size;

    seg_iter = seg_iter->next;
  }

  return len;
}

/* 
 * Section header table located right after program code
 */
int _get_section_offset (elf_file_t *elf_file) {
  int pos = 0;
  pos += _get_code_offset(elf_file);

  SLLNode *header_iter = elf_file->list_segments->next;
  while (header_iter) {
    elf_segment_t* p_segment= (elf_segment_t*)header_iter->data;
    pos += p_segment->program_code.size;

    header_iter= header_iter->next;
  }
  return pos;
}

/*
 * Return the index of .shstrtab in string table.
 */
int _get_string_table_section_index(elf_file_t *elf_file) {
  int index = 0;
  SLL *seg_iter = elf_file->list_segments->next;
  SLLInfo *seg_info = (SLLInfo*)elf_file->list_segments->data;

  while (seg_iter)
  {
    elf_segment_t* p_segment= (elf_segment_t*)seg_iter->data;

    if(strcmp(p_segment->name_string, ".shstrtab") == 0)
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

    /* +1 is for '\0' terminal */
    lenth += strlen(p_segment->name_string) + 1;

    seg_iter = seg_iter->next;
  }

  return lenth;
}

void _fill_section (elf_file_t *elf_file){
  int string_offset = 0;
  int program_code_offset = _get_code_offset(elf_file);

  SLLNode *header_iter = elf_file->list_segments->next;
  while (header_iter)
  {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
    p_segment->section_header.sh_name = string_offset;

    if(strcmp(p_segment->name_string, "") != 0) {
      string_offset += strlen(p_segment->name_string)+1;

      if(strcmp(p_segment->name_string, ".shstrtab") == 0){
          p_segment->section_header.sh_type = 3;
          p_segment->section_header.sh_flags = 0;
          p_segment->section_header.sh_offset = _get_string_table_offset(elf_file);
          p_segment->section_header.sh_size = _get_string_table_lenth(elf_file);
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
    }
    header_iter= header_iter->next;
  }
}

void elf_maker_write(elf_file_t *elf_file, FILE *output)
{
  if (!elf_file || !output)
    return;

  int cur_pos = 0;
  SLLNode *header_iter = elf_file->list_segments->next;

  SLLInfo *si_segment_header = (SLLInfo*)elf_file->list_segments->data;
  elf_file->header.e_phnum = _get_program_header_num(elf_file);
  elf_file->header.e_shnum = si_segment_header->size;
  elf_file->header.e_phoff = _get_program_header_offset(elf_file);
  elf_file->header.e_shoff = _get_section_offset(elf_file);
  elf_file->header.e_entry += _get_code_offset(elf_file);
  elf_file->header.e_shstrndx = _get_string_table_section_index(elf_file);

  /* fill in section */
  _fill_section (elf_file);

  /* write main ELF header */
  cur_pos += fwrite(&elf_file->header, 1, ELF_HEADER_SIZE, output);

  /* write ELF program headers to file */
  header_iter = elf_file->list_segments->next;
  while (header_iter)
  {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
    if(p_segment->has_program) {
  
      cur_pos += fwrite((void*)&(p_segment->program_header), 1, ELF_PROGREAM_HEADER_SIZE, output);
    }
    header_iter= header_iter->next;
  }

  /* write ELF string table to file */
  header_iter = elf_file->list_segments->next;
  while (header_iter)
  {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;

    if(strcmp(p_segment->name_string, "") != 0) {
        cur_pos += fwrite(p_segment->name_string, 1, strlen(p_segment->name_string)+1, output);
    }
    header_iter= header_iter->next;
  }

  /* write program text (code) to file */
  header_iter = elf_file->list_segments->next;
  while (header_iter)
  {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;

    if(p_segment->has_program) {
        cur_pos += fwrite(p_segment->program_code.code, 1, p_segment->program_code.size, output);
    }
    header_iter= header_iter->next;
  }

  /* write section header to file */
  header_iter = elf_file->list_segments->next;

  elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
  memset(&p_segment->section_header, 0, ELF_SECTION_HEADER_SIZE);

  while (header_iter) {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
    cur_pos += fwrite(&p_segment->section_header, 1, sizeof(elf_section_header_t), output);
    header_iter= header_iter->next;
  }

  fclose(output);

  printf("\nGenerated ELF file(out_bin) overlay:\n");
  printf("    ELF header offset:    0x%x\n",      0);
  printf("                 size:    %d bytes\n",      ELF_HEADER_SIZE);
  printf("Program header offset:    0x%x\n",    _get_program_header_offset(elf_file));
  printf("           size x num:    %dx%d bytes\n",   ELF_HEADER_SIZE, si_segment_header->size);
  printf("  String table offset:    0x%x\n",    _get_string_table_offset(elf_file));
  printf("                 size:    %d bytes\n",      _get_string_table_lenth(elf_file));
  printf("  Program code offset:    0x%x\n",    _get_code_offset(elf_file));
  printf("                 size:    %d bytes\n",      _get_code_length(elf_file));
  printf(" Section table offset:    0x%x\n",    _get_section_offset(elf_file));
  printf("           size x num:    %dx%d bytes\n", ELF_SECTION_HEADER_SIZE, si_segment_header->size);

  printf("\nRun: chmod a+x ./out_bin; ./out_bin\n");
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

  /* section name */
  if (name)
  {
    int len = strlen(name);
    segment->name_string = (char*) malloc(len + 1);
    memcpy(segment->name_string, name, len);

    segment->name_string[len] = '\0';
    
    if(strcmp(name, ".text") == 0)
        segment->has_program = 1;
  }

  /* add section to the elf file */
  SLL_insert(elf_file->list_segments, segment);

  /*return the created section */
  return segment;
}

void elf_maker_free(elf_file_t *elf_file)
{
  if (!elf_file)
    return;

  /* free sections */
  SLLNode *header_iter = elf_file->list_segments->next;
  while (header_iter) {
    elf_segment_t* p_segment = (elf_segment_t*)header_iter->data;
    free(p_segment->name_string);
    free(p_segment);

    header_iter = header_iter->next;
  }

  SLL_free(elf_file->list_segments);
  elf_file->list_segments = NULL;

  /* free the file */
  free(elf_file);
  elf_file = NULL;
}
