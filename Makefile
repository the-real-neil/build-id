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

CC = cc
CFLAGS = -O -fPIC -Werror -Wall
LD = ld
LDFLAGS += -ldl -Wl,--build-id=sha1
GREP_SHA1 = egrep -o '\b[0-9a-f]{40}\b'

# let gnu make use posix make variables
.TARGET = $@
.ALLSRC = $^

SRCS = \
  test-build-id-dlopen.c \
  test-build-id-ld.c \
  test-build-id-so.c \
  test-build-id.c

all: $(SRCS:.c=)

################################################################################

test-build-id: test-build-id.o build-id.o
	$(LINK.c) $(.ALLSRC) -o $(.TARGET)

test-build-id-so: test-build-id-so.o libbuild-id.so
	$(LINK.c) $(.ALLSRC) -o $(.TARGET)

test-build-id-dlopen: test-build-id-dlopen.o
	$(LINK.c) $(.ALLSRC) -o $(.TARGET)

# abuse the whitespace between .ALLSRC elements
test-build-id-ld: ld-build-id.ld test-build-id-ld.o
	$(LINK.c) -Wl,--script=$(.ALLSRC) -o $(.TARGET)

# How to generate the default linker script:
#
# ld		: `ld --verbose`
# ld.gold	: ???
# ld.bfd	: ???
# ld.lld	: ???
#
# sed expression assumes default linker script from GNU binutils ld
ld-build-id.ld:
	$(LD) --verbose | sed -r \
	-e '1,/^={50}$$/d' \
	-e '/^={50}$$/,$$d' \
	-e 's|([*][(].note.gnu.build-id[)])|__note_gnu_build_id_begin = .; \1; __note_gnu_build_id_end = .;|' \
	>$(.TARGET)

################################################################################

build-id.o: build-id.c
	$(COMPILE.c) $(.ALLSRC) -o $(.TARGET)

libbuild-id.so: build-id.o
	$(LINK.c) -shared $(.ALLSRC) -o $(.TARGET)

################################################################################

test-build-id.want: test-build-id
	file $(.ALLSRC) | $(GREP_SHA1) >$(.TARGET)

test-build-id-so.want: libbuild-id.so
	file $(.ALLSRC) | $(GREP_SHA1) >$(.TARGET)

test-build-id-dlopen.want: libbuild-id.so
	file $(.ALLSRC) | $(GREP_SHA1) >$(.TARGET)

test-build-id-ld.want: test-build-id-ld
	file $(.ALLSRC) | $(GREP_SHA1) >$(.TARGET)

################################################################################

test-build-id.got: test-build-id
	./$(.ALLSRC) | $(GREP_SHA1) >$(.TARGET)

test-build-id-so.got: test-build-id-so libbuild-id.so
	LD_LIBRARY_PATH=. ./$(.ALLSRC) | $(GREP_SHA1) >$(.TARGET)

test-build-id-dlopen.got: test-build-id-dlopen libbuild-id.so
	./$(.ALLSRC) | $(GREP_SHA1) >$(.TARGET)

test-build-id-ld.got: test-build-id-ld
	./$(.ALLSRC) | $(GREP_SHA1) >$(.TARGET)

################################################################################

check: $(SRCS:.c=.want) $(SRCS:.c=.got)
	cmp test-build-id.want test-build-id.got
	cmp test-build-id-so.want test-build-id-so.got
	cmp test-build-id-dlopen.want test-build-id-dlopen.got
	cmp test-build-id-ld.want test-build-id-ld.got
	@echo PASS

clean:
	rm -f $(SRCS:.c=) $(SRCS:.c=.got) $(SRCS:.c=.want) *.o *.so *.ld
