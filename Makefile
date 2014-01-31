CXXFLAGS = -Wall -g3 -O3 -static -I. -I./src
LDFLAGS =  -Wall -g3 -O3

SRCDIR:= src
OBJDIR:= obj

DIR:= $(shell mkdir -p $(OBJDIR) $(BINDIR))
SRC:= $(shell echo src/*.cpp)
OBJ:= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(notdir $(SRC)))))
DEP:= $(addprefix $(OBJDIR)/, $(addsuffix .d, $(basename $(notdir $(SRC)))))
LIB:= libzling.a

SAMPLE_SRC:= ./sample/zling.cpp
SAMPLE:= zling

target: $(LIB) $(SAMPLE)
clean:
	@ echo -n -e " cleaning..."
	@ rm -rf $(DEP) $(OBJ) $(LIB) $(SAMPLE)
	@ rmdir -p --ignore-fail-on-non-empty $(OBJDIR)
	@ echo -e " done."

.PHONY:  target
.PHONY:  clean

$(SAMPLE): $(SAMPLE_SRC) $(LIB)
	@ echo -e " making sample..."
	@ $(CXX) -o $@ $(SAMPLE_SRC) $(CXXFLAGS) -L. -lzling
	@ echo -e " done."

$(LIB): $(OBJ)
	@ echo -e " making static library..."
	@ $(AR) -r $@ $^
	@ echo -e " done."

-include $(DEP)

$(OBJDIR)/%.d: $(SRCDIR)/%.cpp
	@ echo -n -e " generating makefile dependence of $<..."
	@ $(CXX) $(CXXFLAGS) -MM $< | sed "s?\\(.*\\):?$(OBJDIR)/$(basename $(notdir $<)).o $(basename $(notdir $<)).d :?g" > $@
	@ echo -e " done."

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ echo -n -e " compiling $<..."
	@ $(CXX) $(CXXFLAGS) -c -o $@ $<
	@ echo -e " done."

