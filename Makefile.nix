SOURCES     :=	source glad/src imgui
INCLUDES    :=	source glad/include imgui
DATA        :=	data
BUILD       :=	build-nix
TARGET      :=	$(notdir $(CURDIR))

WANTLIBS    :=	glfw enet dl  m

CFLAGS      :=	-O2 -Wall -Wextra -ffunction-sections -Wno-unused-parameter -Wno-unused-variable -pthread
CXXFLAGS    :=	$(CFLAGS) -std=c++17 -fno-rtti

# https://spin.atomicobject.com/2016/08/26/makefile-c-projects/
SRCS    :=	$(shell find $(SOURCES) -name *.cpp -or -name *.c)
DATAS   :=	$(shell find $(DATA) -name *.png -or -name *.glsl)
OBJS    :=	$(SRCS:%=$(BUILD)/%.o)
DATAS_H :=	$(DATAS:%=$(BUILD)/%.h)
DEPS    :=	$(OBJS:.o=.d) $(DATAS_H:.h=.d)
INC_DIRS    :=	$(INCLUDES) $(addprefix $(BUILD)/,$(DATA))
INC_FLAGS   :=	$(addprefix -I,$(INC_DIRS))
CPPFLAGS    :=	$(INC_FLAGS) -MMD -MP
LDFLAGS     :=	-Wl,--gc-sections $(addprefix -l,$(WANTLIBS)) -pthread

.PHONY:	all clean

all: $(TARGET)
	@echo Compilation complete

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) $(TARGET)
	@echo "Done!"

$(TARGET): $(DATAS_H) $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

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
