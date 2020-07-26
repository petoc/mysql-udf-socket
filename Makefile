TARGET = mysql_udf_socket
PLUGIN_DIR = $(shell mysql_config --plugindir)

all: $(TARGET).c
	gcc -Wall -s -O3 -flto $(shell mysql_config --cflags) -shared -fPIC $(TARGET).c -o $(TARGET).so
	chmod 644 $(TARGET).so

clean:
	rm -f $(TARGET).so

install:
	mkdir -p $(PLUGIN_DIR)/
	cp $(TARGET).so $(PLUGIN_DIR)/

new: clean all
