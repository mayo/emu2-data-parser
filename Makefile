CFLAGS = -Wall

SRCS = emu2_parser.c murmur3.c xml_parser.c
OBJS := ${SRCS:.c=.o}

PYENV = pyenv
PYENV_PYTHON = $(PYENV)/bin/python
SYSTEM_PYTHON = $(or $(shell which python3), $(shell which python))

all: parse

parse: main.c $(OBJS) 
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ 

$(OBJS): %.o: %.c

emu2_parser.o: emu2_parser.c emu2_tags.h

# Ideally the there would be no submake invocation, but this seems to be the
# simplest way to trigger the dependency when it's actually needed. Including
# pyenv in emu2_tags.h dependencies causes it to be triggered even when
# emu2_tags.h does not need updating.
emu2_tags.h: emu2_tags.txt
	@echo "Generating $@"
	$(MAKE) pyenv
	@cat $< | $(PYENV_PYTHON) utils/gen.py > $@
	@echo "$@ updated, remember to commit the new version"

$(PYENV_PYTHON):
	$(SYSTEM_PYTHON) -m venv $(PYENV)
	$(PYENV_PYTHON) -m pip install --upgrade pip
	$(PYENV_PYTHON) -m pip install -r utils/requirements.txt

pyenv: $(PYENV_PYTHON)

.PHONY: pyenv

clean:
	rm -f *.o parse
	rm -rf $(PYENV)

.PHONY: clean