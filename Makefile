PRGNAME     = gnash.elf
CC			= gcc

RENDERER_CONFIG =  agg
HWACCEL_CONFIG = none
PIXEL_FORMAT = RGB565

SRCDIR		= ./gui/sdl/$(RENDERER_CONFIG) ./libcore/asobj/flash/geom ./libmedia/ffmpeg ./libmedia ./libcore ./librender/$(RENDERER_CONFIG) ./librender ./gui ./gui/sdl ./libcore/abc ./libcore/asobj ./libcore/asobj/flash ./libcore/asobj/flash/filters ./libcore/asobj/flash/external ./libcore/asobj/flash/display ./libcore/asobj/flash/text ./libcore/asobj/flash/net ./libcore/swf ./libcore/vm ./libcore/parser ./libsound ./libsound/sdl ./libbase ./libdevice

ifeq ($(RENDERER_CONFIG), agg)
SRCDIR		+= ./agg/src/platform/sdl ./agg/src ./agg/src/ctrl
endif

VPATH		= $(SRCDIR)
SRC_C		= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
SRC_CP		= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))
OBJ_C		= $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJ_CP		= $(notdir $(patsubst %.cpp, %.o, $(SRC_CP)))
OBJS		= $(OBJ_C) $(OBJ_CP)

CFLAGS		= -Os -Wall -Wextra
CFLAGS		+= -DLSB_FIRST  -DHAVE_CONFIG_H -DRG99

CFLAGS		+= -I./ -Ilibmedia/ffmpeg -Ilibmedia -Ilibrender/$(RENDERER_CONFIG) -Ilibrender -Igui -Igui/sdl -Ilibcore/abc -Ilibcore/asobj -Ilibcore/asobj/flash
CFLAGS		+= -Ilibcore/asobj/flash/filters -Ilibcore/asobj/flash/external -Ilibcore/asobj/flash/display -Ilibcore/asobj/flash/text -Ilibcore/asobj/flash/net -Ilibcore/swf
CFLAGS		+= -Ilibcore/vm -Ilibcore/parser -Ilibsound -Ilibsound/sdl -Ilibbase -Ilibdevice -Ilibcore -Ilibcore/asobj/flash/geom -Igui/sdl/$(RENDERER_CONFIG)


CFLAGS		+= -I/usr/include/freetype2 -I/usr/include/SDL

ifeq ($(RENDERER_CONFIG), cairo)
CFLAGS		+= -I/usr/include/cairo
else ifeq ($(RENDERER_CONFIG), agg)
CFLAGS		+= -I./agg/src/platform/sdl -I./agg/src -I./agg/src/ctrl -Iagg/include
endif

CFLAGS		+= -DGUI_SDL -DGUI_CONFIG=\"SDL\" -DRENDERER_CONFIG=\"$(RENDERER_CONFIG)\" -DHWACCEL_CONFIG=\"none\" -DCONFIG_CONFIG=\"none\" -DMEDIA_CONFIG=\"ffmpeg\" -DCXXFLAGS=\"ffmpeg\" -DPLUGINSDIR=\"./\"  -DSYSCONFDIR=\"./\" 
CFLAGS		+= -DSOUND_SDL -DUSE_MEDIA -DOPENDINGUX 

ifeq ($(RENDERER_CONFIG), cairo)
CFLAGS 		+= -DRENDERER_CAIRO
else ifeq ($(RENDERER_CONFIG), agg)
CFLAGS		+= -DRENDERER_AGG
else ifeq ($(RENDERER_CONFIG), opengl)
CFLAGS		+= -DRENDERER_OPENGL
endif

ifeq ($(PIXEL_FORMAT), RGB565)
CFLAGS 		+= -DPIXELFORMAT_RGB565
else ifeq ($(PIXEL_FORMAT), RGB32)
CFLAGS		+= -DPIXELFORMAT_ARGB32
endif

ifeq ($(PROFILE), YES)
CFLAGS 		+= -fprofile-generate=./
else ifeq ($(PROFILE), APPLY)
CFLAGS		+= -fprofile-use="./"
endif

CXXFLAGS 	= $(CFLAGS)

LDFLAGS     = -lc -lgcc -lm -lstdc++ -lfreetype -lavcodec -lavformat -lavutil -lswresample -lswscale -lgif -ljpeg -lpng -lz -lSDL -pthread
ifeq ($(RENDERER_CONFIG), cairo)
LDFLAGS		+= -lcairo
else ifeq ($(RENDERER_CONFIG), opengl)
LDFLAGS		+= -lGL -lGLU
endif

# Rules to make executable
$(PRGNAME): $(OBJS)  
	$(CC) $(CFLAGS) -o $(PRGNAME) $^ $(LDFLAGS)

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -std=gnu99 -c -o $@ $<

$(OBJ_CP) : %.o : %.cpp
	$(CXX) $(CXXFLAGS) -std=gnu++14 -c -o $@ $<

clean:
	rm -f $(PRGNAME) *.o
