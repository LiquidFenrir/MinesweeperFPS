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

-include Makefile.base
