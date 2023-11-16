# Copyright © 2016 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

.POSIX:

CC ?= cc
CFLAGS += -O -fPIC -Werror -Wall
LD ?= ld
LDFLAGS += -Wl,--build-id=sha1

SRCS = \
  test-build-id-dlopen.c \
  test-build-id-ld.c \
  test-build-id-so.c \
  test-build-id.c

all: $(SRCS:.c=)

################################################################################

# test-build-id
test-build-id: LDLIBS += -ldl
test-build-id: build-id.o

# test-build-id-so
test-build-id-so: LDLIBS += -ldl
test-build-id-so: libbuild-id.so

# test-build-id-dlopen
test-build-id-dlopen: LDLIBS += -ldl

# test-build-id-ld
# test-build-id-ld: ld-build-id.ld

test-build-id-ld: LDFLAGS += -Wl,--script=ld-build-id.ld
test-build-id-ld: test-build-id-ld.o ld-build-id.ld
	$(LINK.c) $(OUTPUT_OPTION) $<

# How to generate the default linker script:
#
# ld		: `ld --verbose`
# ld.gold	: ???
# ld.bfd	: ???
# ld.lld	: ???
#
# sed expression assumes default linker script from GNU binutils ld
ld-build-id.ld:
	ld --verbose \
		| sed -E \
			-e '1,/^={50}$$/d' \
			-e '/^={50}$$/,$$d' \
			-e 's|([*][(].note.gnu.build-id[)])|__note_gnu_build_id_begin = .; \1; __note_gnu_build_id_end = .;|' \
			>$@

################################################################################

libbuild-id.so: LDFLAGS += -shared
libbuild-id.so: build-id.o
	$(LINK.c) $(OUTPUT_OPTION) $^

################################################################################

# In general, we can 'file foo' and grep its Build ID to generate 'foo.want'.
#
# Note how 'file' takes all targets '$^' rather than the first target
# '$<'. This is intentional and necessary for the specializations that follow.
%.want: %
	file $^ | tac | grep -Eom1 '[[:xdigit:]]{40}' >$@

# The shared object is the exception.
#
# Note that each of these specializations _appends_ 'libbuild-id.so' to the
# list of target prerequisites. This causes the previous pattern rule to
# execute like so:
#
#     file test-build-id-dlopen libbuild-id.so | tac | grep -Eom1 '[[:xdigit:]]{40}' >test-build-id-dlopen.want
#
#     file test-build-id-so libbuild-id.so | tac | grep -Eom1 '[[:xdigit:]]{40}' >test-build-id-so.want
#
# This is the reason why the 'file' output is 'tac'-reversed and 'grep -m1'
# prints only the first 40-hexit sha1 digest.
test-build-id-dlopen.want: libbuild-id.so
test-build-id-so.want: libbuild-id.so

################################################################################

# In general, we can './foo' and grep its Build ID to generate 'foo.got'.
#
# Note how we are abusing '$^' (the space-delimited list of prerequisites): we
# allow the shell to expand those arguments and invoke the first with any
# others passed as positional arguments. For generic targets, this is harmless
# because '$^' is a singleton array. The 'test-build-id-dlopen' overrides this.
#
# Note how the make variable 'LD_LIBRARY_PATH' defaults to empty --- which
# means that the rule body shell variable also defaults to empty. For generic
# targets, this is harmless and ignored. The 'test-build-id-so' program
# overrides this.
#
%.got: %
	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH)" ./$^ | grep -Eo '[[:xdigit:]]{40}' >$@

# The shared object is (again) the exception.
#
# The 'test-build-id-dlopen' program needs the 'libbuild-id.so' as its first
# argument.
#
# The 'test-build-id-so' program needs the 'LD_LIBRARY_PATH=.'.
#
test-build-id-dlopen.got: libbuild-id.so
test-build-id-so.got: LD_LIBRARY_PATH=.

################################################################################

check: $(SRCS:.c=.want) $(SRCS:.c=.got)
	cmp test-build-id.want test-build-id.got
	cmp test-build-id-so.want test-build-id-so.got
	cmp test-build-id-dlopen.want test-build-id-dlopen.got
	cmp test-build-id-ld.want test-build-id-ld.got
	@echo PASS

clean:
	rm -f $(SRCS:.c=) $(SRCS:.c=.got) $(SRCS:.c=.want) *.o *.so *.ld
