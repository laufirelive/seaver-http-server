
# 项目名称
TARGET = seaver

# 编译及其参数
CC = gcc
CFLAG = -lm -DDBG=0

# 目录
OBJDIR = obj
SRCDIR = src
INCDIR = inc

# 扩展名
EXTENSION = c

# 所有源文件
SOURCES = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.$(EXTENSION)))
# 所有依赖的目标文件
OBJECTS = $(patsubst  %.$(EXTENSION), $(OBJDIR)/%.o, $(notdir $(SOURCES)))
	

# 编译
$(TARGET) : $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(CFLAG)

# 目标文件 编译
$(OBJDIR)/%.o: $(SRCDIR)/%.$(EXTENSION)
	$(CC) -c $(CFLAG) $< -o $@ 

# 清空
.PHONY : clean
clean :
	rm -f $(TARGET) $(OBJECTS)