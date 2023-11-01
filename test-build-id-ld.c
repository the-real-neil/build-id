// build-id/ld-build-id.c

// Introspect the `.note.gnu.build-id` with some help from a linker
// script. This is a build-time, linker-assisted counterpoint to the run-time,
// loader-assisted strategies presented by mattst88.

// References:
//
//
// > The linker always uses a linker script. If you do not supply one yourself,
// > the linker will use a default script that is compiled into the linker
// > executable. You can use the `--verbose' command-line option to display the
// > default linker script.
//
// -- https://sourceware.org/binutils/docs/ld/Scripts.html
//
//
// > If the linker can not recognize the format of an object file, it will
// > assume that it is a linker script. A script specified in this way augments
// > the main linker script used for the link (either the default linker script
// > or the one specified by using `-T'). This feature permits the linker to
// > link against a file which appears to be an object or an archive, but
// > actually merely defines some symbol values, or uses INPUT or GROUP to load
// > other objects. Note that specifying a script in this way should only be
// > used to augment the main linker script; if you want to use some command
// > that logically can only appear once, such as the SECTIONS or MEMORY
// > command, you must replace the default linker script using the `-T' option.
//
// -- https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_node/ld_3.html
//
//
// > An implicit linker script will not replace the default linker
// > script. Typically an implicit linker script would contain only symbol
// > assignments, or the INPUT, GROUP, or VERSION commands.
//
// -- https://sourceware.org/binutils/docs/ld/Implicit-Linker-Scripts.html#Implicit-Linker-Scripts
//
//
// > build_id: Provide ld-embedded build-ids; This patch enables the Elf to be
// > built with the build-id and provide in the Xen hypervisor the code to
// > extract it.
//
// -- https://github.com/xen-project/xen/commit/a353cab905af5eead00a91267f51f79ff34e0ee3
//
//
// > ELF notes allow for appending arbitrary information for the system to
// > use. They are largely used by core files (e_type of ET_CORE), but many
// > projects define their own set of extensions. For example, the GNU tool
// > chain uses ELF notes to pass information from the linker to the C library.
//
// -- https://www.mankier.com/5/elf#Description-Notes_(Nhdr)
//
//
// Even more links:
//
// * https://sourceware.org/binutils/docs/ld/Source-Code-Reference.html#Source-Code-Reference
//
// * https://stackoverflow.com/questions/55622174/is-accessing-the-value-of-a-linker-script-variable-undefined-behavior-in-c
//
// * https://stackoverflow.com/questions/48561217/how-to-get-value-of-variable-defined-in-ld-linker-script-from-c
//
// * https://stackoverflow.com/questions/19912881/how-to-tell-force-gnu-ld-to-put-a-section-symbol-in-a-specific-part-of-the-out

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>
#include <link.h>

typedef ElfW(Nhdr) Elf_Nhdr;

#define ELFNOTE_ALIGN(_n_) (((_n_) + 3) & ~3)
#define ELFNOTE_NAME(_n_) ((char*) (_n_) + sizeof(*(_n_)))
#define ELFNOTE_DESC(_n_) (ELFNOTE_NAME(_n_) + ELFNOTE_ALIGN((_n_)->n_namesz))

/* Defined in linker script. */
extern const unsigned char __note_gnu_build_id_begin[];
extern const unsigned char __note_gnu_build_id_end[];

int main(int argc, char** argv) {
  (void) argc;
  (void) argv;
  assert((size_t) __note_gnu_build_id_begin < (size_t) __note_gnu_build_id_end);
  printf("__note_gnu_build_id_begin : %#0lx\n", (size_t) __note_gnu_build_id_begin);
  printf("__note_gnu_build_id_end   : %#0lx\n", (size_t) __note_gnu_build_id_end);

  const size_t len = (size_t) (__note_gnu_build_id_end - __note_gnu_build_id_begin);
  for (size_t idx = 0; idx < len; ++idx) {
    printf("__note_gnu_build_id_begin[%2ld]: 0x%02x\n", idx,
           (unsigned char) __note_gnu_build_id_begin[idx]);
  }

  printf("sizeof(Elf_Nhdr): %ld\n", sizeof(Elf_Nhdr));
  const Elf_Nhdr* note = (void*) __note_gnu_build_id_begin;
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
