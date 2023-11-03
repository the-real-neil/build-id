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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>
#include <link.h>

////////////////////////////////////////////////////////////////////////////////
//
// The 'ElfW' macro is defined in 'link.h':
//
//    We use this macro to refer to ELF types independent of the native
//    wordsize.  `ElfW(TYPE)' is used in place of `Elf32_TYPE' or `Elf64_TYPE'.
//
// Abuse 'TYPE' to hold 'Nhdr' and (on a 64-bit platform) force the
// preprocessor to emit
//
//    typedef Elf64_Nhdr Elf_Nhdr;
//
// The 'Elf64_Nhdr' type is defined in 'elf.h' and looks like this:
//
//   typedef struct
//   {
//     Elf64_Word n_namesz;			/* Length of the note's name.  */
//     Elf64_Word n_descsz;			/* Length of the note's descriptor.  */
//     Elf64_Word n_type;			/* Type of the note.  */
//   } Elf64_Nhdr;
//
typedef ElfW(Nhdr) Elf_Nhdr;

////////////////////////////////////////////////////////////////////////////////
//
// These macros appear within several prominent projects, including but not
// limited to the following:
//
//   https://github.com/xen-project/xen/blob/7befef87cc9b1bb8ca15d866ce1ecd9165ccb58c/xen/include/xen/elf.h#L32
//   https://github.com/elfmaster/ecfs/blob/175806336b9c1b815e03e0b8c06f949aa333da51/include/ecfs.h#L127

// Take a pointer and round up to the nearest 4-byte boundary.
#define ELFNOTE_ALIGN(_n_) (((_n_) + 3) & ~3)
//
// Take a 'Elf_Nhdr*' and return the pointer to the name.
#define ELFNOTE_NAME(_n_) ((char*) (_n_) + sizeof(*(_n_)))
//
// Take a 'Elf_Nhdr*' and return the pointer to the descriptor.
#define ELFNOTE_DESC(_n_) (ELFNOTE_NAME(_n_) + ELFNOTE_ALIGN((_n_)->n_namesz))

////////////////////////////////////////////////////////////////////////////////

// Defined in linker script.
extern const unsigned char __note_gnu_build_id_begin[];
extern const unsigned char __note_gnu_build_id_end[];

int main(int argc, char** argv) {
  (void) argc;
  (void) argv;

  // Look at the memory between the linker-defined identifiers.
  printf("\n");
  printf("__note_gnu_build_id_begin : %#0lx\n", (size_t) __note_gnu_build_id_begin);
  printf("__note_gnu_build_id_end   : %#0lx\n", (size_t) __note_gnu_build_id_end);
  assert((size_t) __note_gnu_build_id_begin < (size_t) __note_gnu_build_id_end);
  const size_t len = (size_t) (__note_gnu_build_id_end - __note_gnu_build_id_begin);
  for (size_t idx = 0; idx < len; ++idx) {
    printf("__note_gnu_build_id_begin[%2ld](%#0lx): 0x%02x\n", idx,
           (size_t) __note_gnu_build_id_begin + idx,
           (unsigned char) __note_gnu_build_id_begin[idx]);
  }

  // Look at the type information for 'Elf_Nhdr'.
  printf("\n");
  printf("sizeof(Elf_Nhdr): %ld\n", sizeof(Elf_Nhdr));
  printf("offsetof(Elf_Nhdr, n_namesz) : %ld\n", offsetof(Elf_Nhdr, n_namesz));
  printf("offsetof(Elf_Nhdr, n_descsz) : %ld\n", offsetof(Elf_Nhdr, n_descsz));
  printf("offsetof(Elf_Nhdr, n_type)   : %ld\n", offsetof(Elf_Nhdr, n_type));
  printf("sizeof(((Elf_Nhdr*) 0)->n_namesz) : %ld\n", sizeof(((Elf_Nhdr*) 0)->n_namesz));
  printf("sizeof(((Elf_Nhdr*) 0)->n_descsz) : %ld\n", sizeof(((Elf_Nhdr*) 0)->n_descsz));
  printf("sizeof(((Elf_Nhdr*) 0)->n_type)   : %ld\n", sizeof(((Elf_Nhdr*) 0)->n_type));

  // Assign the pointer and look at it.
  const Elf_Nhdr* note = (void*) __note_gnu_build_id_begin;
  printf("\n");
  printf("note               : %#0lx\n", (size_t) note);
  printf("ELFNOTE_NAME(note) : %#0lx\n", (size_t) ELFNOTE_NAME(note));
  printf("ELFNOTE_DESC(note) : %#0lx\n", (size_t) ELFNOTE_DESC(note));
  printf("note->n_namesz     : %2d\n", note->n_namesz);
  printf("note->n_descsz     : %2d\n", note->n_descsz);
  printf("note->n_type       : %2d\n", note->n_type);

  // The length of the note must equal the sum of the constituents.
  assert(len == sizeof(Elf_Nhdr) + note->n_namesz + note->n_descsz);

  // The note type must be this.
  assert(NT_GNU_BUILD_ID == note->n_type);

  printf("\n");

  // Create and populate a buffer for the note name.
  char name[note->n_namesz];
  memcpy(name, ELFNOTE_NAME(note), note->n_namesz);
  // Print the note name.
  printf("name: %s\n", name);

  // This is defined in 'elf.h'.
  //
  //   /* Note entries for GNU systems have this name. */
  //   #define ELF_NOTE_GNU "GNU"
  //
  // The note name must be this.
  assert(0 == strncmp(ELF_NOTE_GNU, name, note->n_namesz));

  // Create and populate a buffer for the note descriptor.
  unsigned char desc[note->n_descsz];
  memcpy(desc, ELFNOTE_DESC(note), note->n_descsz);

  // Print the note descriptor; i.e., the Build ID.
  printf("Build ID: ");
  for (size_t idx = 0; idx < note->n_descsz; ++idx) {
    printf("%02x", desc[idx]);
  }
  printf("\n");

  // The note descriptor size must be 20 bytes because the (assumed)
  // '--build-id=sha1' digest is 160 bits.
  assert(20 == note->n_descsz);

  return EXIT_SUCCESS;
}
