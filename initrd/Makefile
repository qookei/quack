OBJ = init exec vgatty i8042d
LIB = src/sys/sys.o
OUT = $(OBJ:%=out/%)
CLEAN = $(OBJ:%=clean-%)
OUTDIR = out

initrd: $(OUT) $(OUTDIR)
	@printf "ARCHIVE\t\t%s\n" $@
	-@rm -f initrd > /dev/null 2>&1
	@tar cf initrd -C $(OUTDIR)/ $(OBJ)

$(OUTDIR)/%: src/%/ $(LIB) 
	@printf "SUBDIR\t\t%s\n" $(dir $<)
	@make -sC $(dir $<)
	@printf "COPY\t\t%s\n" $@
	@cp $<$(shell basename $@) $@

$(LIB):
	@printf "SUBDIR\t\t%s\n" $(dir $(LIB))
	@make -sC src/sys

$(OUTDIR):
	@printf "MKDIR\t\t%s\n" $@
	@mkdir $(OUTDIR)

clean-%:
	@printf "SUBDIR\t\t%s\n" src/$(@:clean-%=%)/
	@make -sC src/$(@:clean-%=%)/ clean

clean-lib:
	@printf "SUBDIR\t\t%s\n" src/sys/
	@make -sC src/sys/ clean

clean: clean-lib $(CLEAN)
	-rm $(OUT)

