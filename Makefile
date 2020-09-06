.PHONY: desktop all clean clean-switch clean-win clean-nix win nix switch 

desktop: win nix
	@echo "Built all desktop versions."

all: win nix switch
	@echo "Built all."

clean: clean-win clean-nix clean-switch
	@echo "Cleaned all."

clean-win:
	$(MAKE) -f Makefile.win clean

win:
	$(MAKE) -f Makefile.win

clean-nix:
	$(MAKE) -f Makefile.nix clean

nix:
	$(MAKE) -f Makefile.nix

clean-switch:
	$(MAKE) -f Makefile.switch clean

switch:
	$(MAKE) -f Makefile.switch
