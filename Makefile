#
# Open Greedy - an open-source version of Edromel Studio's Greedy XP
#
# Copyright (C) 2014 Arnaud TROEL
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

SELF=$(firstword $(MAKEFILE_LIST))

# no default rules
MAKEFLAGS+=-r
# no default variables
MAKEFLAGS+=-R

# build type
export B?=Linux

ifeq ($(origin SROOT),command line)
$(error SROOT should not be specified from the command line)
endif

ifeq ($(SROOT),)
SROOT=$(abspath $(CURDIR))
ifeq ($R,)
RROOT=$(SROOT)/opt
else
RROOT=$(abspath $R)
endif
export SROOT
export RROOT
all:
	@mkdir -p $(RROOT)/$D && \
		$(MAKE) -sf $(SROOT)/$(SELF) -C $(RROOT)/$D $@
clean:
	@[ ! -d $(RROOT)/$D ] || \
		$(MAKE) -sf $(SROOT)/$(SELF) -C $(RROOT)/$D $@
.NOTPARALLEL: clean
else
bins=
libs=
-include $(SROOT)/$D/Build-$B.mk
-include $(SROOT)/$D/Build.mk
ifeq ($T,)
CFLAGS:=-I$(SROOT)
ifneq ("$(EXTRA_CFLAGS)","")
CFLAGS+=$(EXTRA_CFLAGS)
endif
export CFLAGS
ifneq ("$(EXTRA_LDFLAGS)","")
LDFLAGS+=$(EXTRA_LDFLAGS)
endif
export LDFLAGS
all clean:
	@mkdir -p $(RROOT)/build && $(foreach t,$(tools),$(MAKE) -f $(SELF) T=$t -C $(RROOT)/build RROOT=$(RROOT)/build $@;)
	@$(foreach t,$(libs),$(MAKE) -f $(SELF) T=$t $@;)
	@$(foreach t,$(bins),$(MAKE) -f $(SELF) T=$t $@;)
else
ifneq ("$(MAKECMDGOALS)", "$T")
ifneq ("$(CFLAGS-$T)","")
CFLAGS+=$(CFLAGS-$T)
endif
ifneq ("$(LDFLAGS-$T)","")
LDFLAGS+=$(LDFLAGS-$T)
endif
endif
COMMA=,
V?=0
do=@([ "$(V)" -gt 0 ] && echo "$2" || echo " $1"; eval $2)
nicepath=$(subst $(RROOT)/,,$(abspath $1))
comp=$(call do,COMP $(call nicepath,$1),gcc -MMD -MT \"$@ $(@:%.o=%.d)\" $(CFLAGS) -o $1 -c $2)
arch=$(call do,ARCH $(call nicepath,$1),ar qcs -T $1 $2)
link=$(call do,LINK $(call nicepath,$1),gcc -o $1 \
     -Wl$(COMMA)--whole-archive $(filter %.a,$2) -Wl$(COMMA)--no-whole-archive \
     $(filter-out %.a,$2) $(LDFLAGS))
embed_cmd=+$(call do,DATA $(call nicepath,$1),$(RROOT)/build/embed $(patsubst %.data.o,%,$(notdir $1)) < $2 | gcc $(CFLAGS) -o "$1" -xc -c -)
sub_clean=+$(foreach t,$1,\
	  [ ! -d $(RROOT)/$D/$(dir $t) ] || \
	  $(MAKE) -C $(RROOT)/$(dir $t) -f $(SELF) D=$(dir $t) T=$(notdir $t) clean;)
VPATH=$(SROOT)/$D
$T:=$(patsubst %/,%/lib.a,$($T))
DEPS=$(patsubst %o,%d,$(filter %.o,$($T)))
-include $(DEPS)
all:
	$(foreach t,$(filter %.a,$($T)),\
		mkdir -p $(RROOT)/$(dir $t) && \
		$(MAKE) -C $(RROOT)/$(dir $t) -f $(SELF) D=$(dir $t) T=$(notdir $t) all &&) \
	$(MAKE) -f $(SELF) $T
clean:
	$(call sub_clean,$(filter %.a,$($T)))
	$(call do,CLEAN $(call nicepath,$T),rm -f $T $(filter %.o, $($T)) $(DEPS))
ifneq ("$(RROOT)", "$(SROOT)")
	rmdir $(CURDIR) 2> /dev/null || true
endif
$T: $($T)
ifneq ($(filter $T,$(tools)),)
	$(call link,$@,$^)
endif
ifneq ($(filter $T,$(libs)),)
	$(call arch,$@,$^)
endif
ifneq ($(filter $T,$(bins)),)
	$(call link,$@,$^)
endif
endif # ifeq ($T,)
%.o: %.c
	$(call comp,$@,$<)
%.data.o: % $(SROOT)/lib/embedded.h
	$(call embed_cmd,$@,$<)
endif # ifeq ($(SROOT),)
.PHONY: all clean archive
archive:
	tar cfj ../open_greedy-`date -u +%Y%m%d%H%M%S`.tar.bz2 . --exclude-vcs \
		--exclude-backups --exclude=trash --exclude=opt --exclude=dbg
