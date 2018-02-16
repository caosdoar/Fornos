#pragma once

#include <memory>

class FornosRunner;
class FornosUI_Impl;
struct GLFWwindow;

class FornosUI
{
public:
	FornosUI();
	~FornosUI();
	void init(FornosRunner *runner, GLFWwindow *window);
	void shutdown();
	void process(int windowWidth, int windowHeight);
	void render();

private:
	std::unique_ptr<FornosUI_Impl> _impl;
};