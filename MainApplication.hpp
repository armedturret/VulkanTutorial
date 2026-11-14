#pragma once

class MainApplication {
public:
    void run();

private:
    void initVulkan();
    void mainLoop();
    void cleanup();
};