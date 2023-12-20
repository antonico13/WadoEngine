CFLAGS = -std=c++17 -O3
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.cpp stb_image.h
	g++ $(CFLAGS) -o VulkanTest main.cpp stb_image.h tiny_obj_loader.h $(LDFLAGS)

.PHONY: test clean
test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest