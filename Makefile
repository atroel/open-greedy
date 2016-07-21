#
# Open Greedy - an open-source version of Edromel Studio's Greedy XP
#
# Copyright (C) 2014-2016 Arnaud TROEL
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

.PHONY: all clean subdirs
.NOTPARALLEL: clean

# no default rules
MAKEFLAGS+=-r

# no default variables
MAKEFLAGS+=-R

V?=0
ifeq ($V,0)
show=echo " $1 $(subst $(RROOT)/,,$2)"
MAKEFLAGS+=-s
MAKEFLAGS+=--no-print-directory
endif

ifeq (,$(SROOT)) # -------------------------------------------------------------

R?=opt
export R
export RROOT:=$(abspath $R)
export SROOT:=$(abspath $(CURDIR))

export SELF:=$(SROOT)/$(firstword $(MAKEFILE_LIST))

else # SROOT -------------------------------------------------------------------

ifeq ($(origin SROOT),command line)
$(error SROOT should not be specified from the command line)
endif

endif # SROOT ------------------------------------------------------------------

include Build.mk

CC?=gcc
LD?=ld
RM?=rm -f
RMDIR?=rmdir
MKDIR?=mkdir -p

submake=$(MAKE) -f $(SELF) $1 $2

ifeq (,$T) # -------------------------------------------------------------------

export cppflags:=$(CPPFLAGS)
export cflags:=$(CFLAGS)
export ldflags:=$(LDFLAGS)

all:
	+@$(foreach t,$(tools),\
		$(call submake,$@,T=$t RROOT=$(abspath $(RROOT)/tools)&&)) true
	+@$(foreach t,$(bins),$(call submake,$@,T=$t)&&) true

clean:
	+@$(foreach t,$(tools),\
		$(call submake,$@,T=$t RROOT=$(abspath $(RROOT)/tools);))
	+@$(foreach t,$(bins),$(call submake,$@,T=$t);)

else # T -----------------------------------------------------------------------

subdir=$(subst $(RROOT),$(SROOT),$1)

DIR:=$(subst $(SROOT),$(RROOT),$(CURDIR))
OUT:=$(abspath $(addprefix $(DIR)/,$T))
DEPS:=$(foreach t,$($T),$(addprefix $(DIR)/,$(patsubst %/,%/lib.a, $t)))
RULES:=$(foreach t,$(filter %.o,$(DEPS)),$(t:.o=.d))

cppflags+=$(cppflags-$T)
cflags+=$(cflags-$T)
ldflags+=$(ldflags-$T)

all: subdirs | $(OUT)

clean:
	+$(foreach t,$(filter %.a,$(DEPS)),[ ! -d $(dir $t) ] || \
		$(call submake,$@,-C $(call subdir,$(dir $t)) T=$(notdir $t));)
	$(call show,"CLEAN","$(OUT)")
	$(RM) $(OUT) $(DEPS) $(RULES)
	$(RMDIR) $(DIR) 2> /dev/null || true

subdirs:
	+$(MKDIR) $(DIR) $(foreach t,$(filter %.a,$(DEPS)),&&\
		$(call submake,all,-C $(call subdir,$(dir $t)) T=$(notdir $t)))

$(OUT): $(DEPS)
ifneq (,$(filter lib%.a,$T))
	$(call show,"LINK","$@")
	$(LD) -o $@ -r $^
endif
ifneq (,$(filter $T,$(tools)))
	$(call show,"TOOL","$@")
	$(CC) -o $@ $^ $(ldflags)
endif
ifneq (,$(filter $T,$(bins)))
	$(call show,"EXEC","$@")
	$(CC) -o $@ $^ $(ldflags)
endif

$(DIR)/%.o: %.c
	$(call show,"COMP","$@")
	$(CC) -MMD -MT $@ $(cppflags) $(cflags) -o $@ -c $<

$(DIR)/%.data.o: % $(SROOT)/lib/embedded.h
	$(call show,"DATA","$@")
	$(RROOT)/tools/embed $(patsubst %.data.o,%,$(notdir $@)) < $< | $(CC) $(cppflags) $(cflags) -o $@ -xc -c -

-include $(RULES)

endif # T ----------------------------------------------------------------------
