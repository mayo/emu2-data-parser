PYENV = pyenv
PYENV_PYTHON = $(PYENV)/bin/python
SYSTEM_PYTHON = $(or $(shell which python3), $(shell which python))

$(PYENV_PYTHON):
	$(SYSTEM_PYTHON) -m venv $(PYENV)
	$(PYENV_PYTHON) -m pip install --upgrade pip
	$(PYENV_PYTHON) -m pip install -r utils/requirements.txt

pyenv: $(PYENV_PYTHON)

.PHONY: pyenv

emu2_tags.h: %.h: %.txt $(PYENV_PYTHON)
	cat $< | $(PYENV_PYTHON) utils/gen.py > $@

clean:
	rm -rf $(PYENV)