CXXFLAGS = -Wall -g3 -O3 -static -I.
LDFLAGS =  -Wall -g3 -O3

SRCDIR:= src
OBJDIR:= obj

DIR:= $(shell mkdir -p $(OBJDIR) $(BINDIR))
SRC:= $(shell echo src/*.cpp)
OBJ:= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(notdir $(SRC)))))
DEP:= $(addprefix $(OBJDIR)/, $(addsuffix .d, $(basename $(notdir $(SRC)))))
BIN:= zling

$(BIN): $(OBJ)
	@ echo -e " linking..."
	@ $(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)
	@ echo -e " done."
	@
	@ ### run test ### \
		cat /usr/bin/gcc | \
		./zling e | \
		./zling d | \
		cmp /usr/bin/gcc

-include $(DEP)

$(OBJDIR)/%.d: $(SRCDIR)/%.cpp
	@ echo -n -e " generating makefile dependence of $<..."
	@ $(CXX) $(CXXFLAGS) -MM $< | sed "s?\\(.*\\):?$(OBJDIR)/$(basename $(notdir $<)).o $(basename $(notdir $<)).d :?g" > $@
	@ echo -e " done."

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ echo -n -e " compiling $<..."
	@ $(CXX) $(CXXFLAGS) -c -o $@ $<
	@ echo -e " done."

clean:
	@ echo -n -e " cleaning..."
	@ rm -rf $(DEP) $(OBJ) $(BIN)
	@ rmdir -p --ignore-fail-on-non-empty $(OBJDIR)
	@ echo -e " done."

.IGNORE: clean
.PHONY:  clean
