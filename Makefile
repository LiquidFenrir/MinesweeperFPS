.PHONY: desktop all clean clean-win clean-nix win nix

all: win nix
	@echo "Built all."

clean: clean-win clean-nix
	@echo "Cleaned all."

clean-win:
	$(MAKE) -f Makefile.win clean

win:
	$(MAKE) -f Makefile.win

clean-nix:
	$(MAKE) -f Makefile.nix clean

nix:
	$(MAKE) -f Makefile.nix
