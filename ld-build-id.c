// build-id/ld-build-id.c

#include <assert.h>
#include <elf.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if UINT_MAX == ULONG_MAX
typedef struct {
	Elf32_Word namesz;
	Elf32_Word descsz;
	Elf32_Word type;
} Elf_Note;
#else
typedef struct {
	Elf64_Word namesz;
	Elf64_Word descsz;
	Elf64_Word type;
} Elf_Note;
#endif

#define ELFNOTE_ALIGN(_n_) (((_n_)+3)&~3)
#define ELFNOTE_NAME(_n_) ((char*)(_n_) + sizeof(*(_n_)))
#define ELFNOTE_DESC(_n_) (ELFNOTE_NAME(_n_) + ELFNOTE_ALIGN((_n_)->namesz))

/* Defined in linker script. */
extern const unsigned char __note_gnu_build_id_begin[];
extern const unsigned char __note_gnu_build_id_end[];

int main(int argc, char**argv){
  (void)argc;
  (void)argv;
  assert((size_t)__note_gnu_build_id_begin < (size_t)__note_gnu_build_id_end);
  printf("__note_gnu_build_id_begin : %#0lx\n", (size_t)__note_gnu_build_id_begin);
  printf("__note_gnu_build_id_end   : %#0lx\n", (size_t)__note_gnu_build_id_end);

  const size_t len = (size_t)(__note_gnu_build_id_end - __note_gnu_build_id_begin);
  for (size_t idx = 0; idx < len; ++idx) {
    printf("__note_gnu_build_id_begin[%2ld]: 0x%02x\n", idx, (unsigned char)__note_gnu_build_id_begin[idx]);
  }

  printf("sizeof(Elf_Note): %ld\n", sizeof(Elf_Note));
  const Elf_Note *note = (void*)__note_gnu_build_id_begin;
  assert(len == sizeof(Elf_Note) + note->namesz + note->descsz);

  printf("note->namesz: %d\n", note->namesz);
  printf("note->descsz: %d\n", note->descsz);
  printf("note->type: %#06x\n", note->type);

  char name[note->namesz];
  memcpy(name,ELFNOTE_NAME(note), note->namesz);
  assert(!strncmp("GNU", name, note->namesz));
  // printf("name: %s\n", name);
  
  assert(NT_GNU_BUILD_ID == note->type);

  unsigned char desc[note->descsz];
  memcpy(desc,ELFNOTE_DESC(note), note->descsz);
  printf("Build ID: ");
  for (size_t idx = 0; idx < note->descsz; ++idx) {
    printf("%02x", desc[idx]);
  }
  printf("\n");

  return EXIT_SUCCESS;
}
