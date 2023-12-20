CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.cpp stb_image.h
	g++ $(CFLAGS) -o VulkanTest main.cpp stb_image.h $(LDFLAGS)

.PHONY: test clean
test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest