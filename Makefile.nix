SOURCES		:=	source glad/src imgui
INCLUDES	:=	source glad/include imgui
EXTRAS		:=	extra-libs
DATA		:=	data
BUILD		:=	build-nix
TARGET		:=	$(notdir $(CURDIR))

LIBS		:=	glfw enet dl pthread m
EXTRA_INCS	:=	

CFLAGS		:=	-O2 -Wall -Wextra -ffunction-sections -Wno-unused-parameter -Wno-unused-variable
CXXFLAGS	:=	$(CFLAGS) -std=c++17 -fno-rtti

# https://spin.atomicobject.com/2016/08/26/makefile-c-projects/
SRCS	:=	$(shell find $(SOURCES) -name *.cpp -or -name *.c)
DATAS	:=	$(shell find $(DATA) -name *.png -or -name *.glsl)
OBJS	:=	$(SRCS:%=$(BUILD)/%.o)
DATAS_H	:=	$(DATAS:%=$(BUILD)/%.h)
DEPS	:=	$(OBJS:.o=.d) $(DATAS_H:.h=.d)
EXTRA_IS	:=	$(foreach ex,$(EXTRA_INCS),$(EXTRAS)/$(ex)/include)
INC_DIRS	:=	$(shell find $(INCLUDES) -type d) $(addprefix $(BUILD)/,$(DATA)) $(EXTRA_IS)
INC_FLAGS	:=	$(addprefix -I,$(INC_DIRS))
CPPFLAGS	:=	$(INC_FLAGS) $(LIB_DIRS_F) -MMD -MP
LDFLAGS		:=	-Wl,--gc-sections $(addprefix -L,$(EXTRAS)/libs) $(addprefix -l,$(LIBS))

.PHONY:	all clean

all: $(TARGET)
	@echo Compilation complete

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) $(TARGET)
	@echo "Done!"

$(TARGET): $(DATAS_H) $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@ 

# c source
$(BUILD)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/%.png.h: %.png
	@mkdir -p $(dir $@)
	@python3 converter.py $@ $<

$(BUILD)/%.glsl.h: %.glsl
	@mkdir -p $(dir $@)
	@python3 converter.py $@ $<

-include $(DEPS)
