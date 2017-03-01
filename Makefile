# Source directory
SRCDIR := src
# Object directory
OBJDIR := obj
# main.cc file
MAIN := $(SRCDIR)/main.cc
#All .cc files except main.cc in src directory
SRCS := $(filter-out $(MAIN), $(wildcard $(SRCDIR)/*.cc))
# All the corresponding .o files
OBJS := $(patsubst $(SRCDIR)/%.cc, $(OBJDIR)/%.o, $(SRCS))

CXX := clang++
# CXX := g++-5
CXXFLAGS := -O0 -std=c++11 -g -Wall

# all clean and depend generate no real target files, hence the name PHONY
.PHONY: all clean depend

# Executable being created is dramcore
all: depend dramcore

clean:
	rm -f dramcore
	rm -rf $(OBJDIR)

# obj/.depend file --> stores dependency information to facilitate incremental compilation
depend: $(OBJDIR)/.depend

#Compute dependency information for each .o file and store it in obj/.depend
# -MM --> Doesn't include library header functions in the dependency list
# -MT --> To explicitly specify the object file
# @ --> Prevents the printing the command being executed
$(OBJDIR)/.depend: $(SRCS)
	@mkdir -p $(OBJDIR)
	@rm -f $(OBJDIR)/.depend
	@$(foreach SRC, $(SRCS), $(CXX) $(CXXFLAGS) -MM -MT $(patsubst $(SRCDIR)/%.cc, $(OBJDIR)/%.o, $(SRC)) $(SRC) >> $(OBJDIR)/.depend ;)

# If the comamnd specified is not make clean, execute the makefile obj/.depend
# -include --> Don't throw an error if the file specified to be included is not present
ifneq ($(MAKECMDGOALS), clean)
-include $(OBJDIR)/.depend
endif	

# | --> dramcore should be run only after depend, However, dramcore isn't run each time depend changes
# $@ = dramcore
dramcore: $(MAIN) $(OBJS) $(SRCDIR)/*.h | depend
	$(CXX) $(CXXFLAGS) -o $@ $(MAIN) $(OBJS)
	@echo "Finished building the executable dramcore"

# | --> Execute $(OBJS) after OBJDIR is created, However, don't execute $(OBJS) each time $(OBJDIR) changes
$(OBJS): | $(OBJDIR)

# Create obj directory if it doesn't exist and don't throw an error if it already does
$(OBJDIR):
	@mkdir -p $@


# $@ = target
# $< = first element in the dependency list
$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<



