// build-id/ld-build-id.c

#include <assert.h>
#include <elf.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if UINT_MAX == ULONG_MAX
typedef Elf32_Nhdr Elf_Nhdr;
#else
typedef Elf64_Nhdr Elf_Nhdr;
#endif

#define ELFNOTE_ALIGN(_n_) (((_n_) + 3) & ~3)
#define ELFNOTE_NAME(_n_) ((char *)(_n_) + sizeof(*(_n_)))
#define ELFNOTE_DESC(_n_) (ELFNOTE_NAME(_n_) + ELFNOTE_ALIGN((_n_)->n_namesz))

/* Defined in linker script. */
extern const unsigned char __note_gnu_build_id_begin[];
extern const unsigned char __note_gnu_build_id_end[];

int main(int argc, char ** argv) {
  (void)argc;
  (void)argv;
  assert((size_t)__note_gnu_build_id_begin < (size_t)__note_gnu_build_id_end);
  printf("__note_gnu_build_id_begin : %#0lx\n", (size_t)__note_gnu_build_id_begin);
  printf("__note_gnu_build_id_end   : %#0lx\n", (size_t)__note_gnu_build_id_end);

  const size_t len = (size_t)(__note_gnu_build_id_end - __note_gnu_build_id_begin);
  for (size_t idx = 0; idx < len; ++idx) {
    printf("__note_gnu_build_id_begin[%2ld]: 0x%02x\n", idx,
           (unsigned char)__note_gnu_build_id_begin[idx]);
  }

  printf("sizeof(Elf_Nhdr): %ld\n", sizeof(Elf_Nhdr));
  const Elf_Nhdr * note = (void *)__note_gnu_build_id_begin;
  assert(len == sizeof(Elf_Nhdr) + note->n_namesz + note->n_descsz);

  printf("note->n_namesz: %d\n", note->n_namesz);
  printf("note->n_descsz: %d\n", note->n_descsz);
  printf("note->n_type: %#06x\n", note->n_type);

  char name[note->n_namesz];
  memcpy(name, ELFNOTE_NAME(note), note->n_namesz);
  assert(!strncmp("GNU", name, note->n_namesz));
  // printf("name: %s\n", name);

  assert(NT_GNU_BUILD_ID == note->n_type);

  unsigned char desc[note->n_descsz];
  memcpy(desc, ELFNOTE_DESC(note), note->n_descsz);
  printf("Build ID: ");
  for (size_t idx = 0; idx < note->n_descsz; ++idx) {
    printf("%02x", desc[idx]);
  }
  printf("\n");

  return EXIT_SUCCESS;
}
